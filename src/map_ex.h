/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_MAP_EX_H
#define MOD_SERVLET_MAP_EX_H

#include <map>
#include <unordered_map>

namespace servlet
{

template<typename _MapType>
class _base_map : public _MapType
{
public:
    typedef _MapType map_type;

    typedef typename map_type::key_type key_type;
    typedef typename map_type::mapped_type mapped_type;
    typedef typename map_type::value_type value_type;
    typedef typename map_type::allocator_type allocator_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef typename map_type::size_type size_type;
    typedef typename map_type::difference_type difference_type;

    typedef typename map_type::iterator iterator;
    typedef typename map_type::const_iterator const_iterator;
    typedef typename map_type::reverse_iterator reverse_iterator;
    typedef typename map_type::const_reverse_iterator const_reverse_iterator;

    _base_map() = default;
    _base_map(const _base_map &) = default;
    _base_map(_base_map &&) = default;
    _base_map(const map_type &m) : _MapType{m} {}
    _base_map(map_type &&m) : _MapType{std::move(m)} {}

    ~_base_map() = default;

    _base_map &operator=(const _base_map &) = default;
    _base_map &operator=(_base_map &&) = default;
    _base_map &operator=(const map_type &m)
    {
        map_type::operator=(m);
        return *this;
    }
    _base_map &operator=(map_type &&m)
    {
        map_type::operator=(std::move(m));
        return *this;
    }

    template<typename KeyType>
    bool contains_key(const KeyType &key) const
    { return this->find(key) != this->end(); }

    template<typename KeyType>
    optional_ref<const mapped_type> get(const KeyType &key) const
    {
        const_iterator it = this->find(key);
        return it == this->end() ? optional_ref<const mapped_type>{} : optional_ref<const mapped_type>{it->second};
    }
    template<typename KeyType>
    optional_ref <mapped_type> get(const KeyType &key)
    {
        iterator it = this->find(key);
        return it == this->end() ? optional_ref < mapped_type > {} : optional_ref < mapped_type > {it->second};
    }

    template<typename... Args>
    mapped_type &ensure_get(key_type &&key, Args &&... args)
    {
        return this->try_emplace(key, std::forward<Args>(args)...).first->second;
    }
    template<typename... Args>
    mapped_type &ensure_get(const key_type &key, Args &&... args)
    {
        return this->try_emplace(key, std::forward<Args>(args)...).first->second;
    }

    template<typename... Args>
    bool put(key_type &&key, Args &&... args)
    {
        auto it = this->find(key);
        bool found = it != this->end();
        if (found) it->second = mapped_type{std::forward<Args>(args)...};
        else this->emplace(std::move(key), std::forward<Args>(args)...);
        return found;
    }
    template<typename... Args>
    bool put(const key_type &key, Args &&... args)
    {
        auto it = this->find(key);
        bool found = it != this->end();
        if (found) it->second = mapped_type{std::forward<Args>(args)...};
        else this->emplace(key, std::forward<Args>(args)...);
        return found;
    }

    template<typename KeyType>
    bool remove(const KeyType &key) { return this->erase(key) > 0; }
};

template<typename _Key, typename _Value, typename _Compare = std::less<>,
         typename _Alloc = std::allocator <std::pair<const _Key, _Value>>>
using tree_map = _base_map<std::map < _Key, _Value, _Compare, _Alloc>>;

template<typename _Key, typename _Value, typename _Hash = std::hash <_Key>,
         typename _Pred = std::equal_to <_Key>, typename _Alloc = std::allocator <std::pair<const _Key, _Value>>>
using hash_map = _base_map<std::unordered_map < _Key, _Value, _Hash, _Pred, _Alloc>>;

} // end of servlet namespace

#endif // MOD_SERVLET_MAP_EX_H
