/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_LINKED_HASH_MAP_H
#define SERVLET_LINKED_HASH_MAP_H

#include <map>
#include <unordered_map>
#include <list>
#include <chrono>
#include <mutex>
#include <experimental/any>

#include <servlet/lib/optional.h>

/**
 * @file linked_map.h
 * @brief Containes the implementation of <code>linked_map</code> class
 *        and related type definitions.
 */

namespace servlet
{

/**
 * Implementation of linked associative container.
 *
 * This class maintains the order of elements access and when iterated it
 * iterates the elements in the same order as they were accessed (least recently
 * accessed first).
 *
 * This class can be used as a base class for various LRU container implementations
 * (@see timed_lru_map). To facilitate the implementations there are two protected
 * methods:
 *
 * <ol>
 *   <li>#update - is called on every access to the element.</li>
 *   <li>#purge - is called on every modification of the container.</li>
 * </ol>
 *
 * It can be created with either <code>std::map</code> or
 * <code>std::unordered_map</code> as the underlying map (3rd
 * template parameter).
 *
 * @tparam _Key type of the key
 * @tparam _Tp type of the mapped value
 * @tparam _MT type of the base map for this class to inherit from. Currently it can
 *         be either <code>std::map</code> or <code>std::unordered_map</code>
 *
 * @see timed_lru_map
 */
template<typename _Key, typename _Tp, typename _MT>
class linked_map
{
public:
    /**
     * Container's key type
     */
    typedef _Key                        key_type;
    /**
     * Container's mapped type
     */
    typedef _Tp                         mapped_type;
    /**
     * Container's value type: <code>std::pair<const key_type&, mapped_type></code>
     */
    typedef std::pair<const _Key&, _Tp> value_type;

    /**
     * The type of this container. Defined to brievity.
     */
    typedef linked_map<_Key, _Tp, _MT>  self_type;
    /**
     * List type to maintain the order of elements.
     */
    typedef std::list<value_type>       list_type;
    /**
     * Underlying map type.
     */
    typedef _MT                         map_type;

    /**
     * Container's allocator type
     */
    typedef typename map_type::allocator_type    allocator_type;
    /**
     * <code>value_type&</code>
     */
    typedef          value_type&                 reference;
    /**
     * <code>const value_type&</code>
     */
    typedef          const value_type&           const_reference;
    /**
     * Pointer to value_type type.
     */
    typedef          value_type*                 pointer;
    /**
     * Constant pointer to value_type type.
     */
    typedef          const value_type*           const_pointer;
    /**
     * An unsigned integral type to represent the size of this container.
     */
    typedef typename map_type::size_type         size_type;
    /**
     * A signed integral type to represent distance between iterators.
     */
    typedef typename map_type::difference_type   difference_type;

    /**
     * Bidirectional iterator type.
     */
    typedef typename list_type::iterator               iterator;
    /**
     * Bidirectional constant iterator type.
     */
    typedef typename list_type::const_iterator         const_iterator;
    /**
     * Reverse iterator type.
     */
    typedef typename list_type::reverse_iterator       reverse_iterator;
    /**
     * Constant reverse iterator type.
     */
    typedef typename list_type::const_reverse_iterator const_reverse_iterator;

    /**
     * Constructs an empty container, with no elements.
     */
    linked_map() = default;
    /**
     * Copy constructor.
     *
     * Constructs a container with a copy of each of the elements in <code>m</code>.
     * @param m linked map object to copy from.
     */
    linked_map(const linked_map& m) = default;
    /**
     * Move constructor.
     *
     * Constructs a container that acquires the elements of <code>m</code> by moving them.
     * @param m linked map object to move from.
     */
    linked_map(linked_map&& m) = default;

    ~linked_map() = default;

    /**
     * The copy assignment.
     *
     * Copies all the elements from <code>m</code> into the container
     * (with <code>m</code> preserving its contents)
     * @param m Map object to copy from.
     * @return reference to self
     */
    linked_map& operator=(const linked_map& m) = default;
    /**
     * The move assignment.
     *
     * Moves the elements of <code>m</code> into the container (<code>m</code>
     * is left in an unspecified but valid state).
     * @param m Map object to move from.
     * @return reference to self
     */
    linked_map& operator=(linked_map&& m) = default;

    /**
     * Test whether container is empty
     * @return <code>true</code> if the container #size is <code>0</code>,
     *         <code>false</code> otherwise.
     */
    bool empty() const noexcept { return _map.empty(); }
    /**
     * Returns the number of elements in the container.
     * @return The number of elements in the container.
     */
    size_type size() const noexcept { return _map.size(); }

    /**
     * Tests whether value with a given key exists in this container
     * @tparam KeyType a type comparable to type of this map's key (via
     *         <code>std::less<></code>
     * @param key Key to test.
     * @return <code>true</code> if a value with a given key exists in
     *         this container, <code>false</code> otherwise.
     */
    template<typename KeyType>
    bool contains_key(const KeyType &key) const { return _map.find(key) != _map.end(); }

    /**
     * Clear content
     *
     * <p>Removes all elements from the container (which are destroyed),
     * leaving the container with a size of <code>0</code></p>
     */
    void clear()
    {
        _map.clear();
        _list.clear();
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
        auto it = _map.find(key);
        if (it == _map.end()) return optional_ref<const mapped_type>{};
        _list.splice(_list.cend(), _list, it->second);
        update(*it->second);
        return optional_ref<const mapped_type>{it->second->second};
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
        auto it = _map.find(key);
        if (it == _map.end()) return optional_ref<mapped_type>{};
        _list.splice(_list.cend(), _list, it->second);
        update(*it->second);
        return optional_ref<mapped_type>{it->second->second};
    }

    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args arguments to create the mapped value
     * @return <code>bool</code> denoting whether the previous value was replaced.
     * @see #try_put
     */
    template<class... Args>
    bool put(key_type&& key, Args &&... args)
    {
        auto it = _map.find(key);
        bool found = it != _map.end();
        if (found)
        {
            _list.splice(_list.cend(), _list, it->second);
            it->second->second = mapped_type{std::forward<Args>(args)...};
        }
        else
        {
            auto pr = _map.emplace(std::move(key), _list.begin());
            _list.emplace_back(std::piecewise_construct,
                               std::forward_as_tuple(pr.first->first),
                               std::forward_as_tuple(std::forward<Args>(args)...));
            pr.first->second = --_list.end();
        }
        purge();
        return found;
    }
    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether the previous value was replaced.
     * @see #try_put
     */
    template<class... Args>
    bool put(const key_type& key, Args &&... args)
    {
        auto it = _map.find(key);
        bool found = it != _map.end();
        if (found)
        {
            _list.splice(_list.cend(), _list, it->second);
            it->second->second = mapped_type{std::forward<Args>(args)...};
        }
        else
        {
            auto pr = _map.emplace(key, _list.begin());
            _list.emplace_back(std::piecewise_construct,
                               std::forward_as_tuple(pr.first->first),
                               std::forward_as_tuple(std::forward<Args>(args)...));
            pr.first->second = --_list.end();
        }
        purge();
        return found;
    }

    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, does nothing.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether insertion took place.
     * @see #put
     */
    template<class... Args>
    bool try_put(key_type&& key, Args &&... args)
    {
        auto it = _map.find(key);
        if (it != _map.end()) return false;
        auto pr = _map.emplace(std::move(key), _list.begin());
        _list.emplace_back(std::piecewise_construct,
                           std::forward_as_tuple(pr.first->first),
                           std::forward_as_tuple(std::forward<Args>(args)...));
        pr.first->second = --_list.end();
        purge();
        return true;
    }
    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, does nothing.
     * @tparam Args types of the arguments to be forwarded to new mapped
     *         value constructor.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether insertion took place.
     * @see #put
     */
    template<class... Args>
    bool try_put(const key_type& key, Args &&... args)
    {
        auto it = _map.find(key);
        if (it != _map.end()) return false;
        auto pr = _map.emplace(key, _list.begin());
        _list.emplace_back(std::piecewise_construct,
                           std::forward_as_tuple(pr.first->first),
                           std::forward_as_tuple(std::forward<Args>(args)...));
        pr.first->second = --_list.end();
        purge();
        return true;
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
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        _list.erase(it->second);
        _map.erase(it);
        purge();
        return true;
    }

    /**
     * Returns iterator to beginning of the container.
     *
     * <p>Returns an iterator referring to the least recently used element of
     * the container</p>
     * @return An iterator to the least recently used element of the container.
     */
    iterator begin() noexcept { return _list.begin(); }
    /**
     * Return iterator to end of the container.
     *
     * <p>Returns an iterator referring to the past-the-end element in the container</p>
     * @return An iterator to the past-the-end element in the container.
     */
    iterator end() noexcept { return _list.end(); }
    /**
     * Returns constant iterator to beginning of the container.
     *
     * <p>Returns a constant iterator referring to the least recently used element of
     * the container</p>
     * @return An const_iterator to the least recently used element of the container.
     */
    const_iterator begin() const noexcept { return _list.begin(); }
    /**
     * Return constant iterator to end of the container.
     *
     * <p>Returns a constant iterator referring to the past-the-end element in the container</p>
     * @return An const_iterator to the past-the-end element in the container.
     */
    const_iterator end() const noexcept { return _list.end(); }

    /**
     * Returns constant iterator to beginning of the container.
     *
     * <p>Returns a constant iterator referring to the least recently used element of
     * the container</p>
     * @return An const_iterator to the least recently used element of the container.
     */
    const_iterator cbegin() const noexcept { return _list.cbegin(); }
    /**
     * Return constant iterator to end of the container.
     *
     * <p>Returns a constant iterator referring to the past-the-end element in the container</p>
     * @return An const_iterator to the past-the-end element in the container.
     */
    const_iterator cend() const noexcept { return _list.cend(); }

    /**
     * Return reverse iterator to reverse beginning.
     *
     * <p>Returns a reverse iterator pointing to the last element in the
     * container (i.e., its reverse beginning). For this container it will
     * be the most recently used element.</p>
     * @return A reverse_iterator to the reverse beginning of the sequence container
     */
    reverse_iterator rbegin() noexcept { return _list.rbegin(); }
    /**
     * Return reverse iterator to reverse end.
     *
     * <p>Returns a reverse iterator pointing to the theoretical element right before
     * the first element in the map container (which is considered its reverse end)</p>
     * @return A reverse_iterator to the reverse end of the sequence container.
     */
    reverse_iterator rend() noexcept { return _list.rend(); }
    /**
     * Return constant reverse iterator to reverse beginning.
     *
     * <p>Returns a constant reverse iterator pointing to the last element in the
     * container (i.e., its reverse beginning). For this container it will
     * be the most recently used element.</p>
     * @return A const_reverse_iterator to the reverse beginning of the sequence container
     */
    const_reverse_iterator rbegin() const noexcept { return _list.rbegin(); }
    /**
     * Return constant reverse iterator to reverse end.
     *
     * <p>Returns a constant reverse iterator pointing to the theoretical element right
     * before the first element in the map container (which is considered its reverse end)</p>
     * @return A const_reverse_iterator to the reverse end of the sequence container.
     */
    const_reverse_iterator rend() const noexcept { return _list.rend(); }

    /**
     * Return constant reverse iterator to reverse beginning.
     *
     * <p>Returns a constant reverse iterator pointing to the last element in the
     * container (i.e., its reverse beginning). For this container it will
     * be the most recently used element.</p>
     * @return A const_reverse_iterator to the reverse beginning of the sequence container
     */
    const_reverse_iterator crbegin() const noexcept { return _list.crbegin(); }
    /**
     * Return constant reverse iterator to reverse end.
     *
     * <p>Returns a constant reverse iterator pointing to the theoretical element right
     * before the first element in the map container (which is considered its reverse end)</p>
     * @return A const_reverse_iterator to the reverse end of the sequence container.
     */
    const_reverse_iterator crend() const noexcept { return _list.crend(); }

protected:
    /**
     * Updates element on access.
     *
     * <p>This method is called for each element accessed by #get method and by default
     * does nothing. If the particular implementation needs to do any modifications to
     * the accessed element (e.q. update timestamp) it can do so.</p>
     *
     * @param val Reference to the accessed element.
     */
    virtual void update(value_type& val) const { }
    /**
     * Removes elements which do not confirm to the storage criteria from the container.
     *
     * <p>This method is called for each container modification by #put or #erase method
     * and by default does nothing. If the particular implementation needs to remove certain
     * elements from the container (e.q. if timestamp expired) it can do so.</p>
     */
    virtual void purge() {}

private:
    map_type _map;
    mutable list_type _list;
};

/**
 * Type definition for <code>linked_map</code> inherited from <code>std::map</code>
 */
template <typename _Key, typename _Value, typename _Compare = std::less<>,
          typename _Alloc = std::allocator<std::pair<const _Key,
                                                     typename std::list<std::pair<const _Key&, _Value>>::iterator>>>
using linked_tree_map = linked_map<_Key, _Value,
                                   std::map<_Key, typename std::list<std::pair<const _Key&, _Value>>::iterator,
                                            _Compare, _Alloc>>;

/**
 * Type definition for <code>linked_map</code> inherited from <code>std::unordered_map</code>
 */
template <typename _Key, typename _Value, typename _Hash = std::hash<_Key>,
          typename _Pred = std::equal_to<_Key>,
          typename _Alloc = std::allocator<std::pair<const _Key,
                                                     typename std::list<std::pair<const _Key&, _Value>>::iterator>>>
using linked_hash_map = linked_map<_Key, _Value,
                                   std::unordered_map<_Key, typename std::list<std::pair<const _Key&, _Value>>::iterator,
                                                      _Hash, _Pred, _Alloc>>;

} // end of servlet namespace

#endif // SERVLET_LINKED_HASH_MAP_H
