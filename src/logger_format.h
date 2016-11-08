/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_LOGGER_FORMAT_H
#define MOD_SERVLET_LOGGER_FORMAT_H

#include <experimental/filesystem>

#include <servlet/lib/logger.h>
#include "time.h"
#include "lockfree.h"

namespace servlet { namespace logging {

using servlet::operator<<;

namespace fs = std::experimental::filesystem;

inline static string_view _trim(const std::string &str)
{
    return trim_view(trim_view(trim_view(str), "\""), "'");
}

class string_ptr_provider : public cached_ptr_provider<inplace_ostream>
{
public:
    inplace_ostream* create() override { return new inplace_ostream{}; }
    void prepare_to_cache(inplace_ostream* ptr_to_return) override { (*ptr_to_return)->reset(); }
};

extern ptr_cache<inplace_ostream> INPLACE_STRING_STREAM_CACHE;

class console_log_output : public log_output
{
public:
    ~console_log_output() noexcept = default;

    void write_string(string_view str) override { std::cout.write(str.data(), str.length()); }
    void flush() override { std::cout.flush(); }
    void load_config(std::map<std::string, std::string, std::less<>>& props,
                     const std::string& conf_prefix, const std::string &base_dir) {}
};

struct console_log_output_factory : log_output_factory
{
    ~console_log_output_factory() noexcept override {}
    console_log_output* new_log_output() override { return new console_log_output{}; }
};

class file_log_output : public log_output
{
    std::ofstream _out;
public:
    file_log_output() {}
    ~file_log_output() noexcept override {}

    void write_string(string_view str) override { _out.write(str.data(), str.length()); }
    void flush() override { _out.flush(); }
    void load_config(std::map<std::string, std::string, std::less<>>& props,
                     const std::string& conf_prefix, const std::string &base_dir) override
    {
        auto it = props.find(conf_prefix.empty() ? "log.file" : conf_prefix + "log.file");
        std::string log_file;
        if (it != props.end() && !it->second.empty()) log_file = std::move(_trim(it->second).to_string());
        if (log_file.empty()) log_file = "app.log";
        _out.open(log_file, std::ios_base::out | std::ios_base::app);
    }
};

struct file_log_output_factory : log_output_factory
{
    virtual ~file_log_output_factory() noexcept {}
    virtual file_log_output* new_log_output() { return new file_log_output{}; }
};

class file_name_constructor
{
public:
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> _tp_type;
    virtual ~file_name_constructor() noexcept {}
    virtual std::string get_name(_tp_type tp) = 0;
    virtual std::string get_name(int next_num) = 0;
    virtual std::string get_name(_tp_type tp, int next_num) = 0;
};

class size_rotation_file_log_output : public log_output
{
public:
    size_rotation_file_log_output() {}
    ~size_rotation_file_log_output() noexcept { delete _fn_ctor; }

    void write_string(string_view str) override { _check_file(); _out.write(str.data(), str.length()); }
    void flush() override { _check_file(); _out.flush(); }
    void load_config(std::map<std::string, std::string, std::less<>>& props,
                     const std::string& conf_prefix, const std::string &base_dir) override;
private:
    void _check_file();

    std::size_t _max_size = log_registry::DEFAULT_FILE_ROTATION_SIZE;
    file_name_constructor* _fn_ctor = nullptr;
    int _cur_number = 1;
    counted_ofstream _out;
};

class date_rotation_file_log_output : public log_output
{
public:
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> _tp_type;

    date_rotation_file_log_output() : _ts{std::chrono::system_clock::now()}, _tomorrow{tomorrow(_ts)} {}
    ~date_rotation_file_log_output() noexcept { delete _fn_ctor; }

    void write_string(string_view str) override { _check_file(); _out.write(str.data(), str.length()); }
    void flush() override { _check_file(); _out.flush(); }
    void load_config(std::map<std::string, std::string, std::less<>>& props,
                     const std::string& conf_prefix, const std::string &base_dir) override;
private:
    void _check_file();
    file_name_constructor* _fn_ctor = nullptr;
    _tp_type _ts;
    _tp_type _tomorrow;
    std::ofstream _out;
};

class date_size_rotation_file_log_output : public log_output
{
public:
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> _tp_type;

    date_size_rotation_file_log_output() : _ts{std::chrono::system_clock::now()}, _tomorrow{tomorrow(_ts)} {}
    ~date_size_rotation_file_log_output() noexcept { delete _fn_ctor; }

    void write_string(string_view str) override { _check_file(); _out.write(str.data(), str.length()); }
    void flush() override { _check_file(); _out.flush(); }
    void load_config(std::map<std::string, std::string, std::less<>>& props,
                     const std::string& conf_prefix, const std::string &base_dir) override;

private:
    void _check_file();

    file_name_constructor* _fn_ctor = nullptr;
    std::size_t _max_size = log_registry::DEFAULT_FILE_ROTATION_SIZE;
    int _cur_number = 1;
    _tp_type _ts;
    _tp_type _tomorrow;
    counted_ofstream _out;
};

struct size_rotation_file_log_output_factory : log_output_factory
{
    virtual ~size_rotation_file_log_output_factory() noexcept {}
    virtual size_rotation_file_log_output* new_log_output() { return new size_rotation_file_log_output{}; }
};

struct date_rotation_file_log_output_factory : log_output_factory
{
    virtual ~date_rotation_file_log_output_factory() noexcept {}
    virtual date_rotation_file_log_output* new_log_output() { return new date_rotation_file_log_output{}; }
};

struct date_size_rotation_file_log_output_factory : log_output_factory
{
    virtual ~date_size_rotation_file_log_output_factory() noexcept {}
    virtual date_size_rotation_file_log_output* new_log_output() { return new date_size_rotation_file_log_output{}; }
};

class single_thread_locked_stream : public locked_stream
{
    std::shared_ptr<log_output> _log_output;
public:
    single_thread_locked_stream(std::shared_ptr<log_output> output) : _log_output(output) {}
    ~single_thread_locked_stream() noexcept {}

    inplace_ostream* get_buffer(const std::locale& loc) override
    {
        inplace_ostream *out = INPLACE_STRING_STREAM_CACHE.get();
        if (out->getloc().name() != loc.name()) out->imbue(loc);
        return out;
    }
    void return_buffer(inplace_ostream* buf)
    {
        _log_output->write_string((*buf)->view());
        if ((*buf)->was_flushed()) _log_output->flush();
        INPLACE_STRING_STREAM_CACHE.put(buf);
    }
};

class sync_locked_stream : public locked_stream
{
    std::shared_ptr<log_output> _log_output;
    std::mutex _mutex;
public:
    sync_locked_stream(std::shared_ptr<log_output> output) : _log_output(output), _mutex{} {}

    sync_locked_stream(const sync_locked_stream &other) = default;
    sync_locked_stream(sync_locked_stream &&other) = default;

    ~sync_locked_stream() noexcept override {}

    inplace_ostream* get_buffer(const std::locale& loc) override
    {
        inplace_ostream *out = INPLACE_STRING_STREAM_CACHE.get();
        if (out->getloc().name() != loc.name()) out->imbue(loc);
        return out;
    }
    void return_buffer(inplace_ostream* buf)
    {
        {
            string_view view = (*buf)->view();
            bool need_flush = (*buf)->was_flushed();
            std::lock_guard<std::mutex> lock{_mutex};
            _log_output->write_string(view);
            if (need_flush) _log_output->flush();
        }
        INPLACE_STRING_STREAM_CACHE.put(buf);
    }
};

class inplace_ostream_cache_returner
{
public:
    inplace_ostream_cache_returner(inplace_ostream* ptr) : _ptr(ptr) {}
    ~inplace_ostream_cache_returner() noexcept { INPLACE_STRING_STREAM_CACHE.put(_ptr); }
private:
    inplace_ostream* _ptr;
};

class _log_messages_consumer
{
private:
    std::shared_ptr<log_output> _log_output;
    std::shared_ptr<mpsc_queue<inplace_ostream*>> _queue;
    bool _running = true;
public:
    _log_messages_consumer(std::shared_ptr<log_output> output, std::shared_ptr<mpsc_queue<inplace_ostream*>> &queue) :
            _log_output{output}, _queue{queue} {}
    ~_log_messages_consumer() noexcept { }

    void operator()() { while (_running) _queue->consume(*this); }
    void operator()(inplace_ostream *str);
};

class async_locked_stream : public locked_stream
{
    std::shared_ptr<log_output> _log_output;
    std::shared_ptr<mpsc_queue<inplace_ostream*>> _async_queue;
    std::thread _consumer_thread;
public:
    async_locked_stream(std::shared_ptr<log_output> output, std::size_t queue_size = 1024) :
            _log_output{output},
            _async_queue{std::shared_ptr<mpsc_queue<inplace_ostream*>>{new mpsc_queue<inplace_ostream*>{queue_size}}},
            _consumer_thread{_log_messages_consumer{_log_output, _async_queue}} {}

    async_locked_stream(const async_locked_stream &other) = delete;
    ~async_locked_stream() noexcept override;

    inplace_ostream* get_buffer(const std::locale& loc) override
    {
        inplace_ostream *out = INPLACE_STRING_STREAM_CACHE.get();
        if (out->getloc().name() != loc.name()) out->imbue(loc);
        return out;
    }
    void return_buffer(inplace_ostream* buf) { _async_queue->push(buf); }
};

class rotating_file_name_constructor : public file_name_constructor
{
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> _tp_type;
    typedef std::vector<std::string>              vector_type;
    typedef typename vector_type::difference_type idx_type;

    vector_type _name_parts;
    idx_type _y_idx;
    idx_type _Y_idx;
    idx_type _m_idx;
    idx_type _d_idx;
    idx_type _n_idx;
    int _n_width;

    void _set_number(int next_num);
    void _set_date_part(idx_type idx, int width, int value);
    void _set_date(_tp_type tp);
    std::string _compose_name() const;
public:
    rotating_file_name_constructor(const vector_type& name_parts, idx_type y_idx, idx_type Y_idx,
                                   idx_type m_idx, idx_type d_idx, idx_type n_idx, int n_width) :
            _name_parts(name_parts), _y_idx(y_idx), _Y_idx(Y_idx),
            _m_idx(m_idx), _d_idx(d_idx), _n_idx(n_idx), _n_width(n_width) {}
    rotating_file_name_constructor(vector_type&& name_parts, idx_type y_idx, idx_type Y_idx,
                                   idx_type m_idx, idx_type d_idx, idx_type n_idx, int n_width) :
            _name_parts(std::move(name_parts)), _y_idx(y_idx),
            _Y_idx(Y_idx), _m_idx(m_idx), _d_idx(d_idx), _n_idx(n_idx), _n_width(n_width) {}

    ~rotating_file_name_constructor() noexcept override {}

    inline std::string get_name(_tp_type tp) override
    {
        _set_date(tp);
        return _compose_name();
    }
    inline std::string get_name(int next_num) override
    {
        _set_number(next_num);
        return _compose_name();
    }
    inline std::string get_name(_tp_type tp, int next_num) override
    {
        _set_date(tp);
        _set_number(next_num);
        return _compose_name();
    }
};

enum class PREFIX_VALUE
{
    LEVEL,
    TIMESTAMP,
    NAME,
    THREAD_ID
};

class simple_prefix_printer : public prefix_printer
{
    std::string _prefix;
    time_point_format *_tstamp_fmt = nullptr;
    std::vector<std::pair<PREFIX_VALUE, std::string>> _prefix_format{{PREFIX_VALUE::TIMESTAMP, " | "},
                                                                     {PREFIX_VALUE::THREAD_ID, " | "},
                                                                     {PREFIX_VALUE::LEVEL,     " | "},
                                                                     {PREFIX_VALUE::NAME,      " | "}};
    static char const *LEVEL_STR[];
public:
    simple_prefix_printer() {}
    ~simple_prefix_printer() noexcept override { if (_tstamp_fmt) delete _tstamp_fmt; }

    void print_prefix(LEVEL level, const std::string &name, std::ostream &out) const override;

    void load_config(std::map<std::string, std::string, std::less<>>& props, const std::string& conf_prefix) override;
};

struct simple_prefix_printer_factory : prefix_printer_factory
{
    virtual ~simple_prefix_printer_factory() noexcept {}
    virtual simple_prefix_printer* new_prefix_printer() override { return new simple_prefix_printer{}; }
};

} // end of servlet::logging namespace

} // end of servlet namespace

#endif // MOD_SERVLET_LOGGER_FORMAT_H
