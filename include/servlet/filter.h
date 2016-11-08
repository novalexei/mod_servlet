/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_FILTER_H
#define MOD_SERVLET_FILTER_H

/**
 * @file filter.h
 * @brief Definition of http_filter class,related types and FILTER_EXPORT macro
 */

#include <servlet/request.h>
#include <servlet/response.h>
#include <servlet/context.h>

/**
 * Macro to export the filter factory method to make a servlet available
 * to <code>mod_servlet</code> container. For more details see documentation
 * for servlet::http_filter.
 */
#define FILTER_EXPORT(factoryName, className) extern "C" servlet::http_filter* factoryName() { return new className{}; }

namespace servlet
{

class filter_chain;

/**
 * A filter is an object that performs filtering tasks on either the request to
 * a resource (a servlet or static content), or on the response from a resource,
 * or both. <br>
 * <br>
 * Filters perform filtering in the <code>doFilter</code> method. Every Filter
 * has access to a FilterConfig object from which it can obtain its
 * initialization parameters, a reference to the ServletContext which it can
 * use, for example, to load resources needed for filtering tasks.
 * <p>
 * Filters are configured in the deployment descriptor of a web application
 * <p>
 * Examples that have been identified for this design are<br>
 * 1) Authentication Filters <br>
 * 2) Logging and Auditing Filters <br>
 * 3) Image conversion Filters <br>
 * 4) Data compression Filters <br>
 * 5) Encryption Filters <br>
 * 6) Tokenizing Filters <br>
 * 7) Filters that trigger resource access events <br>
 * 8) XSL/T filters <br>
 * 9) Mime-type chain Filter <br>
 *
 * <p>In order to make the filter available to <code>mod_servlet</code>
 * a factory method should be provided and exported. The easiest way to do
 * this is to use <code>FILTER_EXPORT</code> like this:</p>
 *
 * ~~~~~{.cpp}
 * class my_filter : public servlet::http_filter
 * {
 *   ...
 * };
 *
 * FILTER_EXPORT(myFilterFac, my_filter);
 * ~~~~~
 *
 * If the filter cannot be trivially created or requires special preparation
 * before it is available to <code>mod_servlet</code> container than factory method
 * should be coded manually:
 *
 * ~~~~~{.cpp}
 * class my_filter : public servlet::http_filter
 * {
 *   ...
 * };
 *
 * extern "C" servlet::http_filter* myFilterFac()
 * {
 *     my_filter* filter = nullptr;
 *     // Create and prepare the filter instance
 *     return filter;
 * }
 * ~~~~~
 */
class http_filter
{
public:
    http_filter() = default;
    virtual ~http_filter() noexcept = default;

    /**
     * A convenience method which can be overridden so that there's no need to call
     * <code>init(filter_config)</code>.
     *<p>
     * Instead of overriding #init(filter_config&), simply override this method and it will be
     * called by <code>init(filter_config config)</code>. The <code>filter_config</code>
     * object can still be retrieved via #get_filter_config.
     * @see #init(filter_config&)
     */
    virtual void init() {}

    /**
     * Called by the web container to indicate to a filter that it is being
     * placed into service. The servlet container calls the init method exactly
     * once after instantiating the filter. The init method must complete
     * successfully before the filter is asked to do any filtering work.
     * <p>
     * The web container cannot place the filter into service if the init method
     * either:
     * <ul>
     * <li>Throws an exception</li>
     * <li>Does not return within a time period defined by the web
     *     container</li>
     * </ul>
     *
     * @param cfg The configuration information associated with the
     *            filter instance being initialised
     * @see #init
     */
    virtual void init(filter_config& cfg);

    /**
     * The <code>do_filter</code> method of the http_filter is called by the
     * container each time a request/response pair is passed through the chain
     * due to a client request for a resource at the end of the chain. The
     * filter_chain passed in to this method allows the Filter to pass on the
     * request and response to the next entity in the chain.
     * <p>
     * A typical implementation of this method would follow the following
     * pattern:- <br>
     * 1. Examine the request<br>
     * 2. Optionally wrap the request object with a custom implementation to
     * filter content or headers for input filtering <br>
     * 3. Optionally wrap the response object with a custom implementation to
     * filter content or headers for output filtering <br>
     * 4. a) <strong>Either</strong> invoke the next entity in the chain using
     * the filter_chain object (<code>chain.do_filter()</code>), <br>
     * 4. b) <strong>or</strong> not pass on the request/response pair to the
     * next entity in the filter chain to block the request processing<br>
     * 5. Directly set headers on the response after invocation of the next
     * entity in the filter chain.
     *
     * @param request  The request to process
     * @param response The response associated with the request
     * @param chain    Provides access to the next filter in the chain for this
     *                 filter to pass the request and response to for further
     *                 processing
     */
    virtual void do_filter(http_request& request, http_response& response, filter_chain& chain) = 0;

    /**
     * Returns a <code>filter_config</code> object, which contains initialization and startup
     * parameters for this filter. The <code>filter_config</code> object returned is the one
     * passed to the <code>init</code> method.
     *
     * <p> Method <code>#init</code> is responsible for storing the <code>filter_config</code>
     * object so that this method can return it.
     *
     * @return the <code>filter_config</code> object that initializes this filter
     *
     * @see #init
     */
    const filter_config &get_filter_config() const { return *_cfg; }

    /**
     * Returns the name of this filter instance. The name may be provided via
     * server administration, assigned in the web application deployment
     * descriptor, or for an unregistered (and thus unnamed) filter instance it
     * will be the filter's class name.
     *
     * @return the name of the filter instance
     */
    const std::string &get_filter_name() const { return _cfg->get_filter_name(); }

    /**
     * Returns the all of the filter's initialization parameters as an <code>tree_map</code> of
     * <code>std::string</code> objects, or an empty <code>tree_map</code> if the servlet has no
     * initialization parameters. See filter_config#get_init_parameters.
     *
     * <p> This method is supplied for convenience. It gets the parameter names from
     * the servlet's <code>filter_config</code> object.
     *
     * @return a tree_map of <code>std::string</code> objects containing all of
     *                  the filter's initialization parameters
     * @see filter_config#get_init_parameters
     */
    const std::map<std::string, std::string, std::less<>>& get_init_parameters() const
    { return _cfg->get_init_parameters(); };

    /**
     * Returns an optional <code>std::string</code> containing the value of the named initialization parameter,
     * or <code>null</code> if the parameter does not exist.
     *
     * <p> This method is supplied for convenience. It gets the value of the named parameter from the
     * servlet's <code>filter_config</code> object.
     *
     * @param name a <code>string</code> specifying the name of the initialization parameter
     * @return String an optional reference to <code>std::string</code> containing the value of the
     *         initialization parameter
     * @see servlet_config#get_init_parameter
     */
    template <typename KeyType>
    optional_ref<const std::string> get_init_parameter(const KeyType& name) const
    { return _cfg->get_init_parameter(name); }

private:
    filter_config* _cfg;
};

/**
 * A filter_chain is an object provided by the servlet container to the developer
 * giving a view into the invocation chain of a filtered request for a resource.
 * Filters use the filter_chain to invoke the next filter in the chain, or if the
 * calling filter is the last filter in the chain, to invoke the resource at the
 * end of the chain.
 *
 * @see http_filter
 **/
class filter_chain
{
public:
    virtual ~filter_chain() noexcept {};

    /**
     * Causes the next filter in the chain to be invoked, or if the calling
     * filter is the last filter in the chain, causes the resource at the end of
     * the chain to be invoked.
     *
     * @param request the request to pass along the chain.
     * @param response the response to pass along the chain.
     */
    virtual void do_filter(http_request& request, http_response& response) = 0;
};

} // end of servlet namespace

#endif // MOD_SERVLET_FILTER_H
