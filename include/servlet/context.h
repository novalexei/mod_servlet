/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_CONTEXT_H
#define MOD_SERVLET_CONTEXT_H

#include <experimental/any>
#include <experimental/string_view>

#include <servlet/lib/any_map.h>
#include <servlet/lib/optional.h>

namespace servlet
{

using std::experimental::string_view;

/**
 * Defines a set of methods that a servlet uses to communicate with its servlet
 * container.
 * <p>
 * There is one context per "web application". (A "web application" is a
 * collection of servlets and content installed under a specific subset of the
 * server's URL namespace such as <code>/catalog</code> and possibly installed
 * via a <code>.war</code> file.)
 * <p>
 * In the case of a web application marked "distributed" in its deployment
 * descriptor. In this situation, the context cannot be used as a location to
 * share global information (because the information won't be truly global).
 * Use an external resource like a database instead.
 * <p>
 * The <code>servlet_context</code> object is contained within the
 * servlet_config object, which the Web server provides the servlet when
 * the servlet is initialized.
 *
 * @see http_servlet#get_servlet_config
 * @see http_filter#get_filter_config
 * @see servlet_config#get_servlet_context
 */
class servlet_context
{
public:
    /**
     * Type definition for initial parameters map.
     */
    typedef std::map<std::string, std::string, std::less<>> init_params_map;

    /**
     * virtual destructor
     */
    virtual ~servlet_context() noexcept = default;

    /**
     * Return the main path associated with this context.
     *
     * @return The main context path
     */
    const std::string& get_context_path() const { return _ctx_path; }

    /**
     * Returns filesystem path of current web application
     * @return The webapp filesystem path.
     */
    const std::string& get_webapp_path() const { return _webapp_path; }

    /**
     * Returns the servlet container attribute with the given name, or
     * empty reference if there is no attribute by that name. An attribute
     * allows a servlet container to give the servlet additional information not
     * already provided by this interface. See your server documentation for
     * information about its attributes. Full attributes map can be
     * retrieved using <code>get_attributes</code>.
     * <p>
     * The attribute is returned as a reference to <code>any</code>.
     * @tparam T type of the value to return
     * @tparam KeyType a type comparable to <code>std::string</code>
     *
     * @param key a <code>std::string</code> specifying the name of the attribute
     * @return a reference to <code>any</code> containing the value of the attribute,
     *         or empty reference if no attribute exists matching the given name
     * @throws any_bad_cast if the stored type cannot be casted to the requested type.
     * @see #get_attributes
     */
    template <typename T, typename KeyType>
    optional_ref<const T> get_attribute(const KeyType& key) const { return _attr_map.get<T>(key); }

    /**
     * Finds attribute by key and casts it to specified ValueType.
     * @tparam T type of the value to return
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @param key Key to find attribute value by.
     * @return optional reference to attribute corresponding to key
     * @throws any_bad_cast if the stored type cannot be casted to the requested type.
     */
    template <typename KeyType, typename ValueType>
    optional_ref<const ValueType> get_attribute(const KeyType& key) const
    {
        optional_ref<const any> found = get_attribute(key);
        if (!found.has_value()) return optional_ref<const ValueType>{};
        return optional_ref<const ValueType>{any_cast<const ValueType>(found.value())};
    }

    /**
     * Binds an object to a given attribute name in this servlet context. If the
     * name specified is already used for an attribute, this method will replace
     * the attribute with the new to the new attribute.
     *
     * @tparam ValueType type of the value to set.
     *
     * @param key a <code>std::string</code> specifying the name of the attribute
     * @param value a value of the arbitrary type which will be stored in
     *              <code>any</code> object representing the attribute
     * @return <code>true</code> if value in the map was replaced.
     */
    template <typename ValueType>
    bool set_attribute(const std::string &key, ValueType &&value) { return _attr_map.put(key, std::forward(value)); }

    /**
     * Move version of #set_attribute(const std::string&, ValueType&&)
     *
     * @tparam ValueType type of the value to set.
     * @param key a map key by which to store element
     * @param value value of the element to store
     * @return <code>true</code> if value in the map was replaced.
     * @see #set_attribute(const std::string&, ValueType&&)
     */
    template <typename ValueType>
    bool set_attribute(std::string &&key, ValueType &&value)
    { return _attr_map.put(std::move(key), std::forward(value)); }

    /**
     * Removes the attribute with the given name from the servlet context. After
     * removal, subsequent calls to #get_attribute to retrieve the attribute's
     * value will return an empty reference.
     *
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @param key a <code>string</code> specifying the name of the attribute to
     *            be removed
     */
    template <typename KeyType>
    bool remove_attribute(const KeyType& key) { return _attr_map.erase(key) > 0; }

    /**
     * Returns an <code>tree_map</code> containing all the attributes
     * available within this servlet context.
     *
     * @return a <code>tree_map</code> of attributes
     * @see #get_attribute
     */
    tree_any_map& get_attributes() { return _attr_map; }

    /**
     * Const version of #get_attributes method.
     *
     * @return a const <code>tree_map</code> of attributes
     */
    const tree_any_map& get_attributes() const { return _attr_map; }

    /**
     * Returns a reference to a <code>std::string</code> containing the value of
     * the named context-wide initialization parameter, or an empty reference if
     * the parameter does not exist.
     * <p>
     * This method can make available configuration information useful to an
     * entire "web application". For example, it can provide a webmaster's email
     * address or the name of a system that holds critical data.
     *
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @param key a <code>std::string</code> containing the name of the parameter
     *            whose value is requested
     * @return a reference to a <code>std::string</code> containing the value of
     *         the initialization parameter
     * @see servlet_config#get_init_parameter
     */
    template <typename KeyType>
    optional_ref<const std::string> get_init_parameter(const KeyType& key) const
    {
        auto it = _init_params_map.find(key);
        return it == _init_params_map.end() ? optional_ref<const std::string>{} :
               optional_ref<const std::string>{it->second};
    }

    /**
     * Returns the context's initialization parameters map of
     * <code>std::string</code> objects. The returned map can be empty if the
     * context has no initialization parameters.
     *
     * @return an <code>tree_map</code> of the context's initialization parameters
     * @see servlet_config#get_init_parameters
     * @see servlet_config#get_init_parameter
     */
    const init_params_map& get_init_parameters() const { return _init_params_map; }

    /**
     * Returns the MIME type of the specified file, or <code>null</code> if the
     * MIME type is not known. The MIME type is determined by the configuration
     * of the servlet container, and may be specified in a web application
     * deployment descriptor. Common MIME types are <code>"text/html"</code> and
     * <code>"image/gif"</code>.
     *
     * @param file_name a <code>std::string</code> specifying the name of a file
     * @return a <code>std::string</code> specifying the file's MIME type
     */
    virtual optional_ref<const std::string> get_mime_type(string_view file_name) const = 0;

protected:
    /**
     * Protected constructor to be used from derrived classes.
     * @param ctx_path Context path
     * @param webapp_path Filesystem path to the web application
     * @param init_params Initial parameters map
     */
    servlet_context(const std::string& ctx_path, const std::string& webapp_path, init_params_map &&init_params) :
            _ctx_path{ctx_path}, _webapp_path{webapp_path}, _init_params_map{std::move(init_params)} {}

    /**
     * Context path
     */
    const std::string& _ctx_path;
    /**
     * Attributes map (to be filled by inheriting class).
     */
    tree_any_map _attr_map;
    /**
     * Initial parameters map
     */
    init_params_map _init_params_map;

    /**
     * Filesystem path of current webapp
     */
    const std::string& _webapp_path;
};

/**
 * A servlet configuration object used by a servlet container to pass
 * information to a servlet during initialization.
 */
class servlet_config
{
public:
    /**
     * Creates new servlet_config object with a given servlet name.
     * @param servlet_name the name of the servlet.
     */
    servlet_config(const std::string &servlet_name) : _servlet_name{servlet_name} {}

    /**
     * Move version of the object constructor.
     * @param servlet_name the name of the servlet
     * @see #servlet_config(const std::string &)
     */
    servlet_config(std::string &&servlet_name) : _servlet_name{std::move(servlet_name)} {}

    /**
     * Destructor
     */
    virtual ~servlet_config() noexcept = default;

    /**
     * Returns the name of this servlet instance. The name may be provided via
     * server administration, assigned in the web application deployment
     * descriptor, or for an unregistered (and thus unnamed) servlet instance it
     * will be the servlet's class name.
     *
     * @return the name of the servlet instance
     */
    const std::string& get_servlet_name() const { return _servlet_name; }

    /**
     * Returns a reference to the servlet_context in which the caller is
     * executing.
     *
     * @return a servlet_context object, used by the caller to interact
     *         with its servlet container
     * @see servlet_context
     */
    virtual const servlet_context& get_servlet_context() const = 0;

    /**
     * Returns a <code>std::string</code> containing the value of the named
     * initialization parameter, or empty_reference if the parameter does not
     * exist.
     *
     * @param key a <code>string</code> specifying the name of the
     *            initialization parameter
     * @return a reference to a <code>std::string</code> containing the value
     *         of the initialization parameter
     */
    template <typename KeyType>
    optional_ref<const std::string> get_init_parameter(const KeyType& key) const
    { return get_servlet_context().get_init_parameter(key); }

    /**
     * Returns all the servlet's initialization parameters as a
     * <code>tree_map</code>.
     *
     * @return a <code>tree_map</code> of all the servlet's initialization parameters
     */
    const std::map<std::string, std::string, std::less<>>& get_init_parameters() const
    { return get_servlet_context().get_init_parameters(); }

protected:
    /**
     * Servlet name
     */
    const std::string _servlet_name;
};

/**
 * A filter configuration object used by a servlet container to pass information
 * to a filter during initialization.
 *
 * @see http_filter
 */
class filter_config
{
public:
    /**
     * Creates new filter_config object with a given filter name.
     * @param filter_name The name of the filter.
     */
    filter_config(const std::string &filter_name) : _filter_name{filter_name} {}

    /**
     * Move version of the object constructor.
     * @param filter_name the name of the filter
     * @see #filter_config(const std::string &)
     */
    filter_config(std::string &&filter_name) : _filter_name{std::move(filter_name)} {}

    /**
     * Destructor
     */
    virtual ~filter_config() noexcept = default;

    /**
     * Get the name of the filter.
     *
     * @return The filter-name of this filter as defined in the deployment
     *         descriptor.
     */
    const std::string& get_filter_name() const { return _filter_name; }

    /**
     * Returns a reference to the servlet_context in which the caller is
     * executing.
     *
     * @return servlet_context object, used by the caller to interact
     *         with its servlet container
     *
     * @see servlet_context
     */
    virtual servlet_context& get_servlet_context() = 0;

    /**
     * Returns a <code>std::string</code> containing the value of the named
     * initialization parameter, or empty_reference if the parameter does not
     * exist.
     *
     * @param key a <code>string</code> specifying the name of the
     *            initialization parameter
     * @return a reference to a <code>std::string</code> containing the value
     *         of the initialization parameter
     */
    template <typename KeyType>
    optional_ref<const std::string> get_init_parameter(const KeyType& key)
    { return get_servlet_context().get_init_parameter(key); }

    /**
     * Returns all the filter's initialization parameters as a
     * <code>tree_map</code>.
     *
     * @return a <code>tree_map</code> of all the filter's initialization parameters
     */
    const std::map<std::string, std::string, std::less<>>& get_init_parameters()
    { return get_servlet_context().get_init_parameters(); }

protected:
    /**
     * Filter name
     */
    const std::string _filter_name;
};

} // end of servlet namespace

#endif // MOD_SERVLET_CONTEXT_H
