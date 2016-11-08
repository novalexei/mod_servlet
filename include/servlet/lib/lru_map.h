/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_TIMED_MAP_H
#define MOD_SERVLET_TIMED_MAP_H

#include <map>
#include <unordered_map>

#include <servlet/lib/linked_map.h>

/**
 * @file lru_map.h
 * @brief Containes the implementation of <code>timed_lru_map</code> class
 *        and related classes and type definitions.
 */

namespace servlet
{

/**
 * \cond HIDDEN_SYMBOLS
 */
template<typename T>
class timed_entry
{
public:
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> tp_type;

    timed_entry(const T& value) : _value{value}, _last_modified{std::chrono::system_clock::now()} {}
    timed_entry(T&& value) : _value{std::move(value)}, _last_modified{std::chrono::system_clock::now()} {}
    timed_entry(const timed_entry& other) : _value{other._value}, _last_modified{std::chrono::system_clock::now()} {}
    timed_entry(timed_entry&& other) : _value{std::move(other._value)}, _last_modified{std::chrono::system_clock::now()} {}

    ~timed_entry() = default;

    timed_entry& operator=(const timed_entry& other)
    {
        _value = other._value; _last_modified = other._last_modified;
        return *this;
    }
    timed_entry& operator=(timed_entry&& other)
    {
        _value = std::move(other._value); _last_modified = other._last_modified;
        return *this;
    }
    timed_entry& operator=(const T& value) { _value = value; update_last_modified(); return *this; }
    timed_entry& operator=(T&& value) { _value = std::move(value); update_last_modified(); return *this; }

    operator T&() & { return _value; }
    operator T&&() && { return std::move(_value); }

    T& operator*() { return _value; }
    const T& operator*() const { return _value; }

    tp_type last_modified() { return _last_modified; }
    void update_last_modified() { _last_modified = std::chrono::system_clock::now(); }

private:
    T _value;
    tp_type _last_modified;
};
/**
 * \endcond
 */

/**
 * Implementation of LRU (least recently used) timed cache.
 *
 * This class keeps track of time when elements were accessed and removes
 * them if the have not been accessed for longer than <code>timeout</code>
 *
 * This is a synchronized container.
 *
 * @tparam _Key type of the key
 * @tparam _Tp type of the mapped value
 * @tparam _MT type of the base map for this class to inherit from. Currently it can
 *         be either <code>std::map</code> or <code>std::unordered_map</code>
 * @see linked_map
 */
template<typename _Key, typename _Tp, typename _MT>
class lru_map : public linked_map<_Key, _Tp, _MT>
{
public:
    /**
     * Type of base <code>linked_map</code>
     */
    typedef linked_map<_Key, _Tp, _MT> base_type;

    /**
     * Container's key type
     */
    typedef typename base_type::key_type     key_type;
    /**
     * Container's mapped type
     */
    typedef typename base_type::mapped_type  mapped_type;
    /**
     * Container's value type: <code>std::pair<const key_type&, mapped_type></code>
     */
    typedef typename base_type::value_type   value_type;

    /**
     * Container's allocator type
     */
    typedef typename base_type::allocator_type  allocator_type;
    /**
     * <code>value_type&</code>
     */
    typedef typename base_type::reference       reference;
    /**
     * <code>const value_type&</code>
     */
    typedef typename base_type::const_reference const_reference;
    /**
     * Pointer to value_type type.
     */
    typedef typename base_type::pointer         pointer;
    /**
     * Constant pointer to value_type type.
     */
    typedef typename base_type::const_pointer   const_pointer;
    /**
     * An unsigned integral type to represent the size of this container.
     */
    typedef typename base_type::size_type       size_type;
    /**
     * A signed integral type to represent distance between iterators.
     */
    typedef typename base_type::difference_type difference_type;

    /**
     * Bidirectional iterator type.
     */
    typedef typename base_type::iterator               iterator;
    /**
     * Bidirectional constant iterator type.
     */
    typedef typename base_type::const_iterator         const_iterator;
    /**
     * Reverse iterator type.
     */
    typedef typename base_type::reverse_iterator       reverse_iterator;
    /**
     * Constant reverse iterator type.
     */
    typedef typename base_type::const_reverse_iterator const_reverse_iterator;

    /**
     * Constructs an empty container, with no elements.
     *
     * <p>The timeout argument is specified in seconds. After this number of seconds if
     * element is not accessed it will be removed from this container.</p>
     * @param timeout_sec Expiration time for elements in this container.
     */
    lru_map(std::size_t timeout_sec) : base_type{}, _timeout_sec{timeout_sec} {}
    /**
     * Copy constructor.
     *
     * Constructs a container with a copy of each of the elements in <code>other</code>.
     * @param other linked map object to copy from.
     */
    lru_map(const lru_map& other) : base_type{other}, _timeout_sec{other._timeout_sec} {}
    /**
     * Move constructor.
     *
     * Constructs a container that acquires the elements of <code>other</code> by moving them.
     * @param other linked map object to move from.
     */
    lru_map(lru_map&& other) : base_type{std::move(other)}, _timeout_sec{other._timeout_sec} {}

    /**
     * Destroys the object.
     */
    ~lru_map() = default;

    /**
     * The copy assignment.
     *
     * Copies all the elements from <code>other</code> into the container
     * (with <code>other</code> preserving its contents)
     * @param other Cache object to copy from.
     * @return reference to self
     */
    lru_map& operator=(const lru_map& other)
    {
        _timeout_sec = other._timeout_sec; base_type::operator=(other);
        return *this;
    }
    /**
     * The move assignment.
     *
     * Moves the elements of <code>other</code> into the container (<code>other</code>
     * is left in an unspecified but valid state).
     * @param other Cache object to move from.
     * @return reference to self
     */
    lru_map& operator=(lru_map&& other)
    {
        _timeout_sec = other._timeout_sec; base_type::operator=(std::move(other));
        return *this;
    }

    /**
     * Sets the timeout after which inactive elements will be purged from the
     * cache.
     * @param timeout_sec Number of seconds after which inactive element will
     *                    be removed.
     */
    void set_timeout(std::size_t timeout_sec) { _timeout_sec = timeout_sec; }

    /**
     * Tests whether value with a given key exists in this cache
     * @tparam KeyType a type comparable to type of this map's key (via
     *         <code>std::less<></code>
     * @param key Key to test.
     * @return <code>true</code> if a value with a given key exists in
     *         this cache, <code>false</code> otherwise.
     */
    template<typename KeyType>
    bool contains_key(const KeyType &key) const
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::contains_key(key);
    }

    /**
     * Clear content
     *
     * <p>Removes all elements from the cache (which are destroyed),
     * leaving the container with a size of <code>0</code></p>
     */
    void clear()
    {
        std::lock_guard<std::mutex> guard{_mutex};
        base_type::clear();
    }

    /**
     * Returns <code>optional_ref</code> object to a value with a specified type,
     * if that value exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists empty optional_ref will
     * be returned.
     * @tparam KeyType a type comparable to type of this map's key (via
     *         <code>std::less<></code>
     * @param key Key to be searched for.
     * @return <code>optional_ref</code> to the found value, or empty reference
     *         if a value with a given key doesn't exists in this container.
     * @throws std::bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename KeyType>
    optional_ref<const mapped_type> get(const KeyType& key) const
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::get(key);
    }
    /**
     * Returns <code>optional_ref</code> object to a value with a specified type,
     * if that value exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists empty optional_ref will
     * be returned.
     * @tparam KeyType a type comparable to type of this map's key (via
     *         <code>std::less<></code>
     * @param key Key to be searched for.
     * @return <code>optional_ref</code> to the found value, or empty reference
     *         if a value with a given key doesn't exists in this container.
     * @throws std::bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename KeyType>
    optional_ref<mapped_type> get(const KeyType& key)
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::get(key);
    }

    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether the insertion took place.
     */
    template<class... Args>
    bool put(key_type&& key, Args &&... args)
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::put(std::move(key), std::forward<Args>(args)...);
    }
    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether the insertion took place.
     */
    template<class... Args>
    bool put(const key_type& key, Args &&... args)
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::put(key, std::forward<Args>(args)...);
    }

    /**
     * Erase element.
     *
     * Removes from the container a single element identified by
     * a given key. Does nothing if the element with a given key is
     * not found.
     * @tparam KeyType a type comparable to type of this map's key (via
     *         <code>std::less<></code>
     * @param key Key of the element to remove.
     * @return <code>true</code> if the element was actually removed,
     *         <code>false</code> otherwise.
     */
    template<typename KeyType>
    bool erase(const KeyType &key)
    {
        std::lock_guard<std::mutex> guard{_mutex};
        return base_type::erase(key);
    }
protected:
    /**
     * Updates access timestamp of the element.
     * @param val Value for which to update the access timestamp.
     */
    void update(value_type& val) const override { val.second.update_last_modified(); }

    /**
     * Removes all the elements from the cache which has not been accessed for
     * longer than this cache timeout.
     */
    void purge() override
    {
        auto now = std::chrono::system_clock::now();
        auto b = this->begin();
        auto e = this->end();
        while (b != e &&
               std::chrono::duration_cast<std::chrono::seconds>(now-b->second.last_modified()).count() > _timeout_sec)
        {
            const key_type& key_ref = b->first;
            ++b;
            this->erase(key_ref);
        }
    }
private:
    std::size_t _timeout_sec;
    mutable std::mutex _mutex;
};

/**
 * Type definition for <code>timed_lru_map</code> inherited from <code>std::map</code>
 */
template<typename _Key, typename _Value, typename _Compare = std::less<>,
         typename _Alloc = std::allocator<std::pair<const _Key,
                                                    typename std::list<std::pair<const _Key &, _Value>>::iterator>>>
using lru_tree_map = lru_map<_Key, timed_entry<_Value>,
                             std::map<_Key, typename std::list<std::pair<const _Key &, timed_entry<_Value>>>::iterator,
                                      _Compare, _Alloc>>;

/**
 * Type definition for <code>timed_lru_map</code> inherited from <code>std::unordered_map</code>
 */
template<typename _Key, typename _Value, typename _Hash = std::hash<_Key>,
         typename _Pred = std::equal_to<_Key>,
         typename _Alloc = std::allocator<std::pair<const _Key,
                                                    typename std::list<std::pair<const _Key &, _Value>>::iterator>>>
using lru_hash_map = lru_map<_Key, timed_entry<_Value>,
                             std::unordered_map<_Key, typename std::list<std::pair<const _Key &, timed_entry<_Value>>>::iterator,
                                                _Hash, _Pred, _Alloc>>;

} // end of servlet namespace

#endif // MOD_SERVLET_TIMED_MAP_H
