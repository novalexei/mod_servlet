/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_LOCKFREE_H
#define SERVLET_LOCKFREE_H

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>

namespace servlet
{

namespace lf = boost::lockfree;

template<typename T>
class cached_ptr_provider
{
public:
    cached_ptr_provider() = default;
    cached_ptr_provider(const cached_ptr_provider &other) = default;
    cached_ptr_provider(cached_ptr_provider &&other) = default;
    virtual ~cached_ptr_provider() noexcept = default;

    virtual T *create() = 0;
    virtual void prepare_to_cache(T *ptr_to_return) = 0;
};

template<typename T>
class ptr_cache
{
    lf::queue<T *> _queue;
    cached_ptr_provider<T> *_ptr_provider;
public:
    class ptr
    {
        T *_ptr;
        ptr_cache<T> *_cache;
    public:
        ptr() = default;
        ptr(T *_ptr, ptr_cache<T> *cache) : _ptr{_ptr}, _cache{cache} {}
        ptr(const ptr &) = delete;
        ptr(ptr &&other) : _ptr{other._ptr}, _cache{other._cache} { other.reset_ptr(); }
        ~ptr() { if (_ptr) _cache->put(_ptr); }

        ptr &operator=(const ptr &) = delete;
        ptr &operator=(ptr &&other)
        {
            _ptr = other._ptr;
            _cache = other._cache;
            other.reset_ptr();
            return *this;
        }

        inline T *operator->() { return _ptr; }
        inline T &operator*() { return *_ptr; }

        inline operator bool() const { return _ptr != nullptr; }

        friend class ptr_cache<T>;

    private:
        inline T *get() { return _ptr; }
        inline void reset_ptr() { _ptr = nullptr; }
    };

    ptr_cache(cached_ptr_provider<T> *ptr_provider, std::size_t cache_size = 128) :
            _queue(cache_size), _ptr_provider{ptr_provider} {};
    ~ptr_cache() noexcept
    {
        T *p;
        while (_queue.pop(p)) delete p;
        delete _ptr_provider;
    }

    inline void put(T *p)
    {
        if (!p) return;
        _ptr_provider->prepare_to_cache(p);
        if (!_queue.bounded_push(p)) delete p;
    }

    inline T *get()
    {
        T *p;
        return _queue.pop(p) ? p : _ptr_provider->create();
    }

    inline ptr as_ptr(T *p) { return ptr{p, this}; }
};

template<typename T>
class mpsc_queue
{
    lf::queue<T> _queue;
    std::mutex _read_mx;
    std::shared_mutex _write_mx;
    std::condition_variable _read_cv;
    std::condition_variable_any _write_cv;
    std::atomic<std::size_t> _size{0};
    std::atomic<bool> _no_write{false};
    std::size_t _stop_write_threshold;
    std::size_t _start_write_threshold;
public:
    mpsc_queue(std::size_t queue_size = 1024) : _queue{queue_size}
    {
        _stop_write_threshold = queue_size - queue_size / 10;
        _start_write_threshold = queue_size - 2 * (queue_size / 10);
    }
    ~mpsc_queue() noexcept = default;

    /* No moving or copying */
    mpsc_queue(const mpsc_queue<T> &) = delete;
    mpsc_queue(mpsc_queue<T> &&) = delete;

    void push(T t)
    {
        if (_size > _stop_write_threshold) _no_write = true;
        if (_no_write)
        {
            std::shared_lock<std::shared_mutex> lock(_write_mx);
            while (_no_write)
            {
                _read_cv.notify_one();
                _write_cv.wait(lock);
            }
        }
        ++_size;
        _queue.push(t);
        std::unique_lock<std::mutex> lock(_read_mx);
        _read_cv.notify_one();
    }

    T pop()
    {
        T ret{};
        while (!_queue.pop(ret))
        {
            std::unique_lock<std::mutex> lock(_read_mx);
            while (_queue.empty()) _read_cv.wait(lock);
        }
        --_size;
        if (_no_write && _start_write_threshold > _size)
        {
            _no_write = false;
            std::unique_lock<std::shared_mutex> lock(_write_mx);
            _write_cv.notify_all();
        }
        return ret;
    }

    std::size_t pop(T *t_arr, std::size_t att_sz)
    {
        std::size_t ret = 0;
        while (true)
        {
            for (; ret < att_sz && _queue.pop(t_arr[ret]); ++ret);
            if (ret != 0) break;
            std::unique_lock<std::mutex> lock(_read_mx);
            while (_queue.empty()) _read_cv.wait(lock);
        }
        _size -= ret;
        if (_no_write && _start_write_threshold > _size)
        {
            _no_write = false;
            std::unique_lock<std::shared_mutex> lock(_write_mx);
            _write_cv.notify_all();
        }
        return ret;
    }

    template<typename Functor>
    std::size_t consume(Functor &func)
    {
        std::size_t sz = _no_write ? _size - _start_write_threshold + 2 : 1;
        std::size_t ret = 0;
        while (ret != sz)
        {
            if (_queue.consume_one(func)) ++ret;
            else if (ret == 0)
            {
                _no_write = false;
                std::unique_lock<std::mutex> lock(_read_mx);
                while (_queue.empty()) _read_cv.wait(lock);
            }
            else break;
        }
        _size -= ret;
        if (_no_write && _start_write_threshold > _size)
        {
            _no_write = false;
            std::unique_lock<std::shared_mutex> lock(_write_mx);
            _write_cv.notify_all();
        }
        return ret;
    }

    template<typename Functor>
    void consume_all(Functor &func)
    {
        std::size_t sz;
        while ((sz = _queue.consume_all(func)) == 0)
        {
            std::unique_lock<std::mutex> lock(_read_mx);
            while (_queue.empty()) _read_cv.wait(lock);
        }
        _size -= sz;
        if (_no_write && _start_write_threshold > _size)
        {
            _no_write = false;
            _write_cv.notify_all();
        }
    }

    inline bool empty() const { return _queue.empty(); }
    inline bool is_lock_free() const { return _queue.is_lock_free(); }

    inline long approx_size() const { return _size; }
};

} // end of servlet namespace

#endif // SERVLET_LOCKFREE_H
