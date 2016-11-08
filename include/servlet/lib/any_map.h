#ifndef MOD_SERVLET_ANY_MAP_H
#define MOD_SERVLET_ANY_MAP_H

/**
 * @file any_map.h
 * @brief Containes the implementation of <code>any_map</code> class
 *        and related type definitions.
 */

#include <map>
#include <unordered_map>
#include <experimental/any>

#include <servlet/lib/optional.h>

namespace servlet
{

using std::experimental::any;
using std::experimental::any_cast;

/**
 * Covenience class on top of associative container to facilitate work with
 * value type <code>std::any</code>.
 *
 * <p>This class implements several convenience method to make it easy
 * to use map. The methods are:</p>
 *
 * <ol>
 *   <li>#contains_key - checks if the record with a given key exists</li>
 *
 *   <li>#get - returns optional_ref of the value.</li>
 *
 *   <li>#ensure_get - returns the value reference for a given key and
 *   if the record doesn't exist new record is created and reference to
 *   the value returned</li>
 *
 *   <li>#put - inserts or replaces the value for a given key</li>
 * </ol>
 *
 * This class inherits either <code>std::map</code> or
 * <code>std::unordered_map</code> specified as template parameter.
 *
 * All the regular <code>std::map</code> or <code>std::unordered_map</code>
 * methods are inherited as is.
 *
 * In general this object can be used in folowing way:
 *
 * ~~~~~{.cpp}
 * tree_any_map m;
 * m.ensure_get<std::vector<std::string>>("vector").push_back("value");
 * std::cout << m.get<std::vector<std::string>>("vector")->size() << std::endl;
 * m.ensure_get<int>("int") = 3;
 * std::cout << m.get<int>("int") << std::endl;
 * m.put<double>("int", 3.14);
 * std::cout << m.get<double>("int") << std::endl;
 * ~~~~~
 *
 * The above program will output:
 *
 * <pre>
 * 1
 * 3
 * 3.14
 * </pre>
 *
 * @tparam _MapType base map type from which this class inherits. For now it can be
 *                  either <code>std::map</code> of <code>std::unordered_map</code>.
 *
 * @see tree_any_map
 * @see hash_any_map
 *
 * @todo reimplement new value creations when any constructors
 *       with in_place_type_t are available in the std lib.
 */
template<typename _MapType>
class any_map : public _MapType
{
public:
    /**
     * Type definition for this map implementation.
     */
    typedef _MapType                    map_type;

    /**
     * Container's key type
     */
    typedef typename map_type::key_type        key_type;
    /**
     * Container's mapped type
     */
    typedef typename map_type::mapped_type     mapped_type;
    /**
     * Container's value type: <code>std::pair<const key_type, mapped_type></code>
     */
    typedef typename map_type::value_type      value_type;
    /**
     * Container's allocator type
     */
    typedef typename map_type::allocator_type  allocator_type;
    /**
     * <code>value_type&</code>
     */
    typedef          value_type&               reference;
    /**
     * <code>const value_type&</code>
     */
    typedef          const value_type&         const_reference;
    /**
     * Pointer type to value_type.
     */
    typedef typename map_type::pointer         pointer;
    /**
     * Constant pointer type to value_type.
     */
    typedef typename map_type::const_pointer   const_pointer;
    /**
     * An unsigned integral type to represent the size of this container.
     */
    typedef typename map_type::size_type       size_type;
    /**
     * A signed integral type to represent distance between iterators.
     */
    typedef typename map_type::difference_type difference_type;

    /**
     * Bidirectional iterator type.
     */
    typedef typename map_type::iterator               iterator;
    /**
     * Bidirectional constant iterator type.
     */
    typedef typename map_type::const_iterator         const_iterator;
    /**
     * Reverse iterator type.
     */
    typedef typename map_type::reverse_iterator       reverse_iterator;
    /**
     * Constant reverse iterator type.
     */
    typedef typename map_type::const_reverse_iterator const_reverse_iterator;

    /**
     * Constructs an empty container, with no elements.
     */
    any_map() = default;
    /**
     * Forwarding constructor. It forwards all arguments to the underlying
     * map constructor.
     * @tparam Args types of the arguments to forward
     * @param args Arguments to forward to the underlying map constructor
     */
    template<typename... Args>
    any_map(Args &&... args) : map_type{std::forward<Args>(args)...} {}

    /**
     * Destroys the object.
     */
    ~any_map() = default;

    /**
     * Forwarding assignment. It forwards all arguments to the underlying
     * map assignment operator.
     * @tparam Args types of the arguments to forward
     * @param args Arguments to forward to the underlying map constructor
     * @return reference to self
     */
    template<typename... Args>
    any_map& operator=(Args &&... args) { map_type::operator=(std::forward<Args>(args)...); return *this; }

    /**
     * Tests whether value with a given key exists in this container
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @param key Key to test.
     * @return <code>true</code> if a value with a given key exists in
     *         this container, <code>false</code> otherwise.
     */
    template<typename KeyType>
    bool contains_key(const KeyType &key) const { return this->find(key) != this->end(); }

    /**
     * Returns <code>optional_ref</code> object to a value with a specified type,
     * if that value exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists empty optional_ref will
     * be returned.
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @tparam T type of the value to return
     * @param key Key to be searched for.
     * @return <code>optional_ref</code> to the found value, or empty reference
     *         if a value with a given key doesn't exists in this container.
     * @throws bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename T, typename KeyType>
    optional_ref<const T> get(const KeyType& key) const
    {
        auto it = this->find(key);
        return it == this->end() ? optional_ref<const T>{} : optional_ref<const T>{any_cast<const T&>(it->second)};
    }
    /**
     * Returns <code>optional_ref</code> object to a value with a specified type,
     * if that value exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists empty optional_ref will
     * be returned.
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @tparam T type of the value to return
     * @param key Key to be searched for.
     * @return <code>optional_ref</code> to the found value, or empty reference
     *         if a value with a given key doesn't exists in this container.
     * @throws std::bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename T, typename KeyType>
    optional_ref<T> get(const KeyType& key)
    {
        auto it = this->find(key);
        return it == this->end() ? optional_ref<T>{} : optional_ref<T>{any_cast<T&>(it->second)};
    }

    /**
     * Returns reference to a value with a specified type, if that value
     * exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists new value will be created
     * and emplaced into this container with a given key.
     * @tparam T type of the value to return
     * @tparam Args types of the arguments to construct a mapped value.
     * @param key Key to be searched for.
     * @param args argument to create the mapped value if it doesn't exist
     * @return reference to the found or created value.
     * @throws std::bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename T, typename... Args>
    T& ensure_get(key_type&& key, Args &&... args)
    {
        std::pair<iterator, bool> res = this->try_emplace(std::move(key));
        if (res.second) res.first->second = T{std::forward<Args>(args)...};
        return any_cast<T&>(res.first->second);
    }
    /**
     * Returns reference to a value with a specified type, if that value
     * exists and can be casted to the requested type.
     *
     * If the value with a given key doesn't exists new value will be created
     * and emplaced into this container with a given key.
     * @tparam T type of the value to associate with the key
     * @tparam Args types of the arguments to construct a mapped value.
     * @param key Key to be searched for.
     * @param args argument to create the mapped value if it doesn't exist
     * @return reference to the found or created value.
     * @throws std::bad_any_cast if the value is found, but couldn't be casted
     *         to the requested type
     */
    template<typename T, typename... Args>
    T& ensure_get(const key_type& key, Args &&... args)
    {
        std::pair<iterator, bool> res = this->try_emplace(key);
        if (res.second) res.first->second = T{std::forward<Args>(args)...};
        return any_cast<T&>(res.first->second);
    }

    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam T type of the value to associate with the key
     * @tparam Args types of the arguments to construct a mapped value.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether the insertion took place.
     */
    template<typename T, typename... Args>
    bool put(key_type&& key, Args &&... args)
    {
        std::pair<iterator, bool> res = this->try_emplace(std::move(key));
        res.first->second = T{std::forward<Args>(args)...};
        return res.second;
    }
    /**
     * Associates a value of specified type created with a given arguments
     * with the specified key in this map. If the map previously contained
     * a mapping for the key, the old value is replaced.
     * @tparam T type of the value to associate with the key
     * @tparam Args types of the arguments to construct a mapped value.
     * @param key key with which the specified value is to be associated
     * @param args argument to create the mapped value
     * @return <code>bool</code> denoting whether the insertion took place.
     */
    template<typename T, typename... Args>
    bool put(const key_type& key, Args &&... args)
    {
        std::pair<iterator, bool> res = this->try_emplace(key);
        res.first->second = T{std::forward<Args>(args)...};
        return res.second;
    }
};

/**
 * Type definition for <code>any_map</code> inherited from <code>std::map</code>
 */
template <typename _Key = std::string, typename _Compare = std::less<>,
          typename _Alloc = std::allocator<std::pair<const _Key, any>>>
using tree_any_value_map = any_map<std::map<_Key, any, _Compare, _Alloc>>;

/**
 * Type definition for <code>any_map</code> inherited from <code>std::map</code>
 * and with the key type <code>std::string</code>
 */
using tree_any_map = any_map<std::map<std::string, any, std::less<>>>;

/**
 * Type definition for <code>any_map</code> inherited from <code>std::unordered_map</code>
 */
template <typename _Key = std::string, typename _Hash = std::hash<_Key>,
          typename _Pred = std::equal_to<_Key>, typename _Alloc = std::allocator<std::pair<const _Key, any>>>
using hash_any_value_map = any_map<std::unordered_map<_Key, any, _Hash, _Pred, _Alloc>>;

/**
 * Type definition for <code>any_map</code> inherited from <code>std::unordered_map</code>
 * and with the key type <code>std::string</code>
 */
using hash_any_map = any_map<std::unordered_map<std::string, any>>;

} // end of servlet namespace

#endif // MOD_SERVLET_ANY_MAP_H
