/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_PATTERN_MAP_H
#define SERVLET_PATTERN_MAP_H

#include <ostream>
#include <iterator>
#include <atomic>
#include <algorithm>
#include <string>
#include <vector>
#include <experimental/string_view>
#include <iostream>
#include <servlet/lib/optional.h>

namespace servlet
{

using std::experimental::basic_string_view;
using std::experimental::string_view;

template <typename T>
class tree_visitor
{
public:
    virtual ~tree_visitor() noexcept {}
    virtual void in(T& value) = 0;
    virtual void out() = 0;
};

template<typename ValueType>
struct pattern_map_pair
{
    pattern_map_pair(const std::string& pattern, bool ex, const ValueType &val) :
            uri_pattern{pattern}, exact{ex}, value{val} {}
    pattern_map_pair(std::string&& pattern, bool ex, const ValueType &val) :
            uri_pattern{std::move(pattern)}, exact{ex}, value{val} {}

    template<class... Args>
    pattern_map_pair(const std::string& pattern, bool ex, Args &&... args) :
            uri_pattern{pattern}, exact{ex}, value{std::forward<Args>(args)...} {}
    template<class... Args>
    pattern_map_pair(std::string&& pattern, bool ex, Args &&... args) :
            uri_pattern{std::move(pattern)}, exact{ex}, value{std::forward<Args>(args)...} {}

    ~pattern_map_pair() { for (auto item : detalizations) delete item; }

    void add_detalization(pattern_map_pair<ValueType> *pair);
    void finalize();

    void traverse(tree_visitor<ValueType>& visitor);

    std::string uri_pattern;
    bool exact;
    ValueType value;
    std::vector<pattern_map_pair*> detalizations;
};

/**
 * pattern_map is a map which uses patterns for keys. The purpose of this class is to store and search
 * associations of URI patterns to value. At this point two URI pattern types are supported:
 * - exact - URI should match key exactly;
 * - open_ended - URI should start with key.
 */
template<typename ValueType>
class pattern_map
{
public:
    typedef std::string key_type;
    typedef ValueType value_type;
    typedef pattern_map_pair<ValueType> pair_type;

    typedef std::vector<pair_type*> storage_type;

    pattern_map() {}
    virtual ~pattern_map() { delete _catch_all; for (auto item : _storage) delete item; }

    void add(const key_type &pattern, const value_type &value) { _add(new pair_type{pattern, value}); }
    void add(key_type &&pattern, const value_type &value) { _add(new pair_type{std::move(pattern), value}); }

    template<class... Args>
    bool add(const key_type &pattern, bool exact, Args &&... args)
    { return _add(new pair_type{pattern, exact, std::forward<Args>(args)...}); }
    template<class... Args>
    bool add(key_type &&pattern, bool exact, Args &&... args)
    { return _add(new pair_type{std::move(pattern), exact, std::forward<Args>(args)...}); }

    void finalize();
    bool is_finalized() const { return _finalized; }
    void traverse(tree_visitor<ValueType>& visitor);

    std::size_t size() const { return _storage.size(); }

    optional_ref<value_type> get(const std::string& uri);
    optional_ref<value_type> get(string_view uri);

    pair_type* get_pair(const std::string& uri);
    pair_type* get_pair(string_view uri);

    pair_type* get_pair_shallow(const std::string& uri);
    pair_type* get_pair_shallow(string_view uri);

    typename storage_type::iterator begin() { return _storage.begin(); }
    typename storage_type::iterator end() { return _storage.end(); }
    typename storage_type::const_iterator begin() const { return _storage.begin(); }
    typename storage_type::const_iterator end() const { return _storage.end(); }
    typename storage_type::const_iterator cbegin() const { return _storage.cbegin(); }
    typename storage_type::const_iterator cend() const { return _storage.cend(); }

private:
    bool _add(pair_type* new_pair);

    pair_type* _catch_all = nullptr;
    storage_type _storage;
    bool _finalized = false;
};

template<typename ValueType>
inline bool pattern_map_pair_cmp(const pattern_map_pair<ValueType> *pair1, const pattern_map_pair<ValueType> *pair2)
{
    return pair1->uri_pattern < pair2->uri_pattern;
}

/* Compares two strings in reverse order. We assume that str1.length() > str2.length() */
template<typename StringType1, typename StringType2>
inline bool is_detalization(const StringType1 &str1, const StringType2 &str2)
{
    return std::equal(str1.rbegin(), str1.rend(), string_view{str2.data(), str1.size()}.rbegin());
}



template<typename T, typename Enable = void>
struct is_mergeable : std::false_type { };
template<typename T>
struct is_mergeable<T, decltype(std::declval<T>().merge(std::declval<T&>()))> : std::true_type { };

template<typename T, typename Enable = void>
struct is_ptr_mergeable : std::false_type { };
template<typename T>
struct is_ptr_mergeable<T, decltype(std::declval<T>()->merge(std::declval<T&>()))> : std::true_type { };

template<typename T, typename std::enable_if_t<(is_mergeable<T>()), int> = 0>
inline void merge(T& t1, T& t2) { t1.merge(t2); }
template<typename T, typename std::enable_if_t<(is_ptr_mergeable<T>()), int> = 0>
inline void merge(T& t1, T& t2) { t1->merge(t2); }
template<typename T, typename std::enable_if_t<(!(is_mergeable<T>() || is_ptr_mergeable<T>())), int> = 0>
inline void merge(T& t1, T& t2) {}

template<typename ValueType>
bool _add_pair(pattern_map_pair<ValueType> *new_pair, std::vector<pattern_map_pair<ValueType>*> &pairs)
{
    auto e = pairs.end();
    auto cur = e;
    for (auto b = pairs.begin(); b != e; ++b)
    {
        if (cur == e && (*b)->uri_pattern.length() <= new_pair->uri_pattern.length())
        {
            if (!(*b)->exact)
            {
                if (is_detalization((*b)->uri_pattern, new_pair->uri_pattern))
                {
                    if ((*b)->uri_pattern.length() == new_pair->uri_pattern.length() && !new_pair->exact)
                    {
                        merge((*b)->value, new_pair->value);
                        return false;
                    }
                    (*b)->add_detalization(new_pair);
                    return true;
                }
            }
            else if ((*b)->uri_pattern == new_pair->uri_pattern)
            {
                if (new_pair->exact)
                {
                    merge(new_pair->value, (*b)->value);
                    return false;
                }
                new_pair->add_detalization((*b));
                if (cur == e)
                {
                    *b = new_pair;
                    cur = b;
                    continue;
                }
            }
        }
        else if (new_pair->uri_pattern.length() <= (*b)->uri_pattern.length())
        {
            if (!new_pair->exact && is_detalization(new_pair->uri_pattern, (*b)->uri_pattern))
            {
                if (new_pair->uri_pattern.length() == (*b)->uri_pattern.length() && !(*b)->exact)
                {
                    merge(new_pair->value, (*b)->value);
                    return false;
                }
                new_pair->add_detalization((*b));
                if (cur == e)
                {
                    *b = new_pair;
                    cur = b;
                    continue;
                }
            }
        }
        if (cur != e && b != ++cur) *cur = *b;
    }
    if (cur == e) { pairs.push_back(new_pair); return true; }
    else { for (int i = e-cur; i > 1; --i) pairs.pop_back(); return true; }
}

template<typename ValueType>
void pattern_map_pair<ValueType>::add_detalization(pattern_map_pair<ValueType> *pair)
{
    _add_pair(pair, detalizations);
}

template<typename ValueType>
void pattern_map_pair<ValueType>::finalize()
{
    std::sort(detalizations.begin(), detalizations.end(), pattern_map_pair_cmp<ValueType>);
    if (!detalizations.empty()) for (auto pr : detalizations) pr->finalize();
    detalizations.shrink_to_fit();
}

template<typename ValueType>
void pattern_map_pair<ValueType>::traverse(tree_visitor<ValueType>& visitor)
{
    visitor.in(value);
    for (pattern_map_pair<ValueType> *pr : detalizations) pr->traverse(visitor);
    visitor.out();
}

template<typename ValueType>
bool pattern_map<ValueType>::_add(pair_type* new_pair)
{
    if (new_pair->uri_pattern == "/" && !new_pair->exact)
    {
        if (_catch_all) return false;
        _catch_all = new_pair;
        return true;
    }
    else return _add_pair(new_pair, _storage);
}

template<typename ValueType>
void pattern_map<ValueType>::finalize()
{
    std::sort(_storage.begin(), _storage.end(), pattern_map_pair_cmp<value_type>);
    for (auto pr : _storage) pr->finalize();
    _storage.shrink_to_fit();
    _finalized = true;
}

template<typename ValueType>
void pattern_map<ValueType>::traverse(tree_visitor<ValueType>& visitor)
{
    if (_catch_all) visitor.in(_catch_all->value);
    for (pair_type *pr : _storage) pr->traverse(visitor);
    if (_catch_all) visitor.out();
}

template<typename It> inline std::reverse_iterator<It> __rev(It it) { return std::reverse_iterator<It>{it}; }

template<typename StringType>
inline bool _is_pattern(const std::string& pattern, const StringType& uri, bool exact)
{
    if (exact) return pattern.size() == uri.size() && std::equal(pattern.rbegin(), pattern.rend(), uri.rbegin());
    return pattern.size() <= uri.size() &&
           std::equal(__rev(pattern.end()), __rev(pattern.begin()), __rev(uri.begin() + pattern.size()));
}

template <typename ValueType, typename StringType>
inline bool _cmp_to_pair(pattern_map_pair<ValueType>* pr, const StringType& uri)
{
    if (pr->uri_pattern.size() >= uri.size()) return pr->uri_pattern < uri;
    return pr->exact ? true : uri.compare(0, pr->uri_pattern.size(), pr->uri_pattern) > 0;
}

template<typename ValueType, typename StringType>
pattern_map_pair<ValueType>* _find(const StringType &uri, std::vector<pattern_map_pair<ValueType>*> &pairs, bool shallow)
{
    auto pit = std::lower_bound(pairs.begin(), pairs.end(), uri, _cmp_to_pair<ValueType, StringType>);
    if (pit == pairs.end() || !_is_pattern((*pit)->uri_pattern, uri, (*pit)->exact)) return nullptr;
    if (shallow || (*pit)->detalizations.empty()) return *pit;
    auto dpit = _find(uri, (*pit)->detalizations, shallow);
    return dpit ? dpit : *pit;
}

template<typename ValueType>
pattern_map_pair<ValueType>* pattern_map<ValueType>::get_pair(const std::string &uri)
{
    pair_type* pr = _find(uri, _storage, false);
    return pr ? pr : _catch_all;
}
template<typename ValueType>
pattern_map_pair<ValueType>* pattern_map<ValueType>::get_pair(string_view uri)
{
    pair_type* pr = _find(uri, _storage, false);
    return pr ? pr : _catch_all;
}

template<typename ValueType>
pattern_map_pair<ValueType>* pattern_map<ValueType>::get_pair_shallow(const std::string& uri)
{
    pair_type* pr = _find(uri, _storage, false);
    return pr ? pr : _catch_all;
}
template<typename ValueType>
pattern_map_pair<ValueType>* pattern_map<ValueType>::get_pair_shallow(string_view uri)
{
    pair_type* pr = _find(uri, _storage, false);
    return pr ? pr : _catch_all;
}

template<typename ValueType>
optional_ref<ValueType> pattern_map<ValueType>::get(const std::string &uri)
{
    pair_type* pr = get_pair(uri);
    return pr ? optional_ref<value_type>{pr->value} : optional_ref<value_type>{};
}
template<typename ValueType>
optional_ref<ValueType> pattern_map<ValueType>::get(string_view uri)
{
    pair_type* pr = _find(uri, _storage, false);
    return pr ? optional_ref<value_type>{pr->value} : optional_ref<value_type>{};
}

} // end of servlet namespace

#endif // SERVLET_PATTERN_MAP_H
