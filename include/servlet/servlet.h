#ifndef SERVLET_SERVLET_H
#define SERVLET_SERVLET_H

/**
 * @file servlet.h
 * @brief Definition of http_servlet class and SERVLET_EXPORT macro
 */

#include <string>

#include <servlet/request.h>
#include <servlet/response.h>
#include <servlet/context.h>

/**
 * Macro to export the servlet factory method to make a servlet available
 * to <code>mod_servlet</code> container. For more details see documentation
 * for servlet::http_servlet.
 */
#define SERVLET_EXPORT(factoryName, className) extern "C" servlet::http_servlet* factoryName() { return new className{}; }

namespace servlet
{

/**
 * Provides an abstract class to be subclassed to create
 * an HTTP servlet suitable for a Web site. A subclass of
 * <code>http_servlet</code> must override at least
 * one method, usually one of these:
 *
 * <ul>
 * <li> <code>do_get</code>, if the servlet supports HTTP GET requests
 * <li> <code>do_post</code>, for HTTP POST requests
 * <li> <code>do_put</code>, for HTTP PUT requests
 * <li> <code>do_delete</code>, for HTTP DELETE requests
 * <li> <code>init</code>, to manage resources that are held for the life of the servlet
 * <li> <code>get_servlet_config</code>, which the servlet uses to provide information about itself
 * </ul>
 *
 * <p>There's almost no reason to override the <code>service</code> * method.
 * <code>service</code> handles standard HTTP requests by dispatching them to the handler methods
 * for each HTTP request type (the <code>do_</code><i>method</i> methods listed above).</p>
 *
 * <p>Likewise, there's almost no reason to override the
 * <code>do_options</code> and <code>do_trace</code> methods.</p>
 *
 * <p>Servlets typically run on multithreaded servers,
 * so be aware that a servlet must handle concurrent
 * requests and be careful to synchronize access to shared resources.
 * Shared resources include in-memory data such as
 * instance or class variables and external objects
 * such as files, database connections, and network
 * connections.</p>
 *
 * <p>In order to make the servlet available to <code>mod_servlet</code>
 * a factory method should be provided and exported. The easiest way to do
 * this is to use <code>SERVLET_EXPORT</code> like this:</p>
 *
 * ~~~~~{.cpp}
 * class my_servlet : public servlet::http_servlet
 * {
 *   ...
 * };
 *
 * SERVLET_EXPORT(myServletFac, my_servlet);
 * ~~~~~
 *
 * If the servlet cannot be trivially created or requires special preparation
 * before it is available to <code>mod_servlet</code> container than factory method
 * should be coded manually:
 *
 * ~~~~~{.cpp}
 * class my_servlet : public servlet::http_servlet
 * {
 *   ...
 * };
 *
 * extern "C" servlet::http_servlet* myServletFac()
 * {
 *     my_servlet* servlet = nullptr;
 *     // Create and prepare the servlet instance
 *     return servlet;
 * }
 * ~~~~~
 */
class http_servlet
{
public:
    http_servlet() = default;
    virtual ~http_servlet() noexcept = default;

    /**
     * Called by the servlet container to indicate to a servlet that the servlet is being placed into service.
     *
     * <p> The servlet container calls the <code>init</code> method exactly once after instantiating the
     * servlet. The <code>init</code> method must complete successfully before the servlet can receive any requests.
     *
     * <p> The servlet container cannot place the servlet into service if the <code>init</code> method
     * <ol>
     * <li>Throws a <code>servlet_exception</code>
     * <li>Does not return within a time period defined by the Web server
     * </ol>
     *
     * @param config a <code>servlet_config</code> object containing the servlet's
     *            configuration and initialization parameters
     *
     * @see #get_servlet_config
     */
    virtual void init(servlet_config& config);

    /**
     * A convenience method which can be overridden so that there's no need to call <code>init(config)</code>.
     *
     * <p> Instead of overriding #init(servlet_config&), simply override this method and it will be
     * called by <code>init(config)</code>. The <code>servlet_config</code> object can still
     * be retrieved via #get_servlet_config().
     */
    virtual void init() {}

    /**
     * Receives standard HTTP requests from the public <code>service</code> method and dispatches
     * them to the <code>do_</code><i>method</i> methods defined in this class.There's no
     * need to override this method.
     *
     * @param req the http_request object that contains the request the client made of the servlet
     *
     * @param resp the http_response object that contains the response the servlet returns to the client
     */
    virtual void service(http_request& req, http_response& resp);

    /**
     * Returns a <code>servlet_config</code> object, which contains initialization and startup
     * parameters for this servlet. The <code>servlet_config</code> object returned is the one
     * passed to the <code>init</code> method.
     *
     * <p> Method <code>#init</code> is responsible for storing the <code>servlet_config</code>
     * object so that this method can return it.
     *
     * @return the <code>servlet_config</code> object that initializes this servlet
     *
     * @see #init
     */
    const servlet_config &get_servlet_config() const { return *_cfg; }

    /**
     * Returns the name of this servlet instance. The name may be provided via
     * server administration, assigned in the web application deployment
     * descriptor, or for an unregistered (and thus unnamed) servlet instance it
     * will be the servlet's class name.
     *
     * @return the name of the servlet instance
     */
    const std::string &get_servlet_name() const { return _cfg->get_servlet_name(); }

    /**
     * Returns the all of the servlet's initialization parameters as an <code>tree_map</code> of
     * <code>std::string</code> objects, or an empty <code>tree_map</code> if the servlet has no
     * initialization parameters. See servlet_config#get_init_parameters.
     *
     * <p> This method is supplied for convenience. It gets the parameter names from
     * the servlet's <code>servlet_config</code> object.
     *
     * @return tree_map a tree_map of <code>std::string</code> objects containing all of
     *                  the servlet's initialization parameters
     * @see servlet_config#get_init_parameters
     */
    const std::map<std::string, std::string, std::less<>>& get_init_parameters() const
    { return _cfg->get_init_parameters(); };

    /**
     * Returns an optional <code>std::string</code> containing the value of the named initialization parameter,
     * or <code>null</code> if the parameter does not exist.
     *
     * <p> This method is supplied for convenience. It gets the value of the named parameter from the
     * servlet's <code>servlet_config</code> object.
     *
     * @tparam KeyType a type comparable to <code>std::string</code>
     * @param name a <code>string</code> specifying the name of the initialization parameter
     * @return String an optional reference to <code>std::string</code> containing the value of the
     *         initialization parameter
     * @see servlet_config#get_init_parameter
     */
    template <typename KeyType>
    optional_ref<const std::string> get_init_parameter(const KeyType& name) const
    { return _cfg->get_init_parameter(name); }

protected:
    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a GET request.
     *
     * <p>Overriding this method to support a GET request also automatically supports an HTTP HEAD request.
     * A HEAD request is a GET request that returns no body in the response, only the request header fields.
     *
     * <p>When overriding this method, read the request data, write the response headers, get the response's
     * writer or output stream object, and finally, write the response data. It's best to include content
     * type and encoding. When using a <code>std::ostream</code> object to return the response,
     * set the content type before accessing the <code>std::ostream</code> object.
     *
     * <p>The servlet container must write the headers before committing the response, because in HTTP
     * the headers must be sent before the response body.
     *
     * <p>Where possible, set the Content-Length header
     * (with the http_response::set_content_length(std::size_t) method), to allow the servlet container
     * to use a persistent connection to return its response to the client, improving performance.
     * The content length is automatically set if the entire response fits inside the response buffer.
     *
     * <p>When using HTTP 1.1 chunked encoding (which means that the response has a Transfer-Encoding header),
     * do not set the Content-Length header.
     *
     * <p>The GET method should be safe, that is, without any side effects for which users are held responsible.
     * For example, most form queries have no side effects. If a client request is intended to change stored data,
     * the request should use some other HTTP method.
     *
     * <p>The GET method should also be idempotent, meaning that it can be safely repeated. Sometimes making a
     * method safe also makes it idempotent. For example, repeating queries is both safe and idempotent, but
     * buying a product online or modifying data is neither safe nor idempotent.
     *
     * <p>If the request is incorrectly formatted, <code>do_get</code> returns an HTTP "Bad Request" message.
     *
     * @param req an http_request object that contains the request the client has made of the servlet
     *
     * @param resp an http_response object that contains the response the servlet sends to the client
     *
     * @see servlet::http_response#set_content_type
     */
    virtual void do_get(http_request& req, http_response& resp);

    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a POST request.
     *
     * The HTTP POST method allows the client to send data of unlimited length to the Web server a single time
     * and is useful when posting information such as credit card numbers.
     *
     * <p>When overriding this method, read the request data, write the response headers, get the response's
     * writer or output stream object, and finally, write the response data. It's best to include content
     * type and encoding. When using a <code>std::ostream</code> object to return the response, set the
     * content type before accessing the <code>std::ostream</code> object.
     *
     * <p>The servlet container must write the headers before committing the response, because in HTTP
     * the headers must be sent before the response body.
     *
     * <p>Where possible, set the Content-Length header (with the
     * http_response#set_content_length method), to allow the servlet container
     * to use a persistent connection to return its response to the client, improving performance.
     * The content length is automatically set if the entire response fits inside the response buffer.
     *
     * <p>When using HTTP 1.1 chunked encoding (which means that the response has a Transfer-Encoding header),
     * do not set the Content-Length header.
     *
     * <p>This method does not need to be either safe or idempotent. Operations requested through POST
     * can have side effects for which the user can be held accountable, for example,
     * updating stored data or buying items online.
     *
     * <p>If the HTTP POST request is incorrectly formatted, <code>do_post</code> returns an HTTP
     * "Bad Request" message.
     *
     *
     * @param req an http_request object that contains the request the client has made of the servlet
     *
     * @param resp an http_response object that contains the response the servlet sends to the client
     *
     * @see servlet::http_response#set_content_type
     */
    virtual void do_post(http_request& req, http_response& resp);

    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a PUT request.
     *
     * The PUT operation allows a client to place a file on the server and is similar to sending a file by FTP.
     *
     * <p>When overriding this method, leave intact any content headers sent with the request (including
     * Content-Length, Content-Type, Content-Transfer-Encoding, Content-Encoding, Content-Base,
     * Content-Language, Content-Location, Content-MD5, and Content-Range). If your method cannot
     * handle a content header, it must issue an error message (HTTP 501 - Not Implemented) and discard
     * the request. For more information on HTTP 1.1, see RFC 2616 <a href="http://www.ietf.org/rfc/rfc2616.txt"></a>.
     *
     * <p>This method does not need to be either safe or idempotent. Operations that <code>do_put</code>
     * performs can have side effects for which the user can be held accountable. When using
     * this method, it may be useful to save a copy of the affected URL in temporary storage.
     *
     * <p>If the HTTP PUT request is incorrectly formatted, <code>do_put</code> returns an HTTP "Bad Request" message.
     *
     * @param req the http_request object that contains the request the client made of the servlet
     *
     * @param resp the http_response object that contains the response the servlet returns to the client
     */
    virtual void do_put(http_request& req, http_response& resp);

    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a DELETE request.
     *
     * The DELETE operation allows a client to remove a document or Web page from the server.
     *
     * <p>This method does not need to be either safe or idempotent. Operations requested through
     * DELETE can have side effects for which users can be held accountable. When using
     * this method, it may be useful to save a copy of the affected URL in temporary storage.
     *
     * <p>If the HTTP DELETE request is incorrectly formatted, <code>do_delete</code> returns
     * an HTTP "Bad Request" message.
     *
     * @param req the http_request object that contains the request the client made of the servlet
     *
     *
     * @param resp the http_response object that contains the response the servlet returns to the client
     */
    virtual void do_delete(http_request& req, http_response& resp);
    /**
     * <p>Receives an HTTP HEAD request from the protected <code>service</code> method and handles the
     * request.
     * The client sends a HEAD request when it wants to see only the headers of a response, such as
     * Content-Type or Content-Length. The HTTP HEAD method counts the output bytes in the response
     * to set the Content-Length header accurately.
     *
     * <p>If you override this method, you can avoid computing the response body and just set the response headers
     * directly to improve performance. Make sure that the <code>do_head</code> method you write is both safe
     * and idempotent (that is, protects itself from being called multiple times for one HTTP HEAD request).
     *
     * <p>If the HTTP HEAD request is incorrectly formatted, <code>do_head</code> returns an HTTP "Bad Request"
     * message.
     *
     * @param req the http_request object that is passed to the servlet
     *
     * @param resp the http_response object that the servlet uses to return the headers to the client
     */
    virtual void do_head(http_request& req, http_response& resp);

    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a TRACE request.
     *
     * A TRACE returns the headers sent with the TRACE request to the client, so that they can be used in
     * debugging. There's no need to override this method.
     *
     * @param req the http_request object that contains the request the client made of the servlet
     *
     * @param resp the http_response object that contains the response the servlet returns to the client
     */
    virtual void do_trace(http_request& req, http_response& resp);

    /**
     * Called by the server (via the <code>service</code> method) to allow a servlet to handle a OPTIONS request.
     *
     * The OPTIONS request determines which HTTP methods the server supports and returns an appropriate
     * header. For example, if a servlet overrides <code>do_get</code>, this method returns the
     * following header:
     *
     * <p><code>Allow: GET, HEAD, TRACE, OPTIONS</code>
     *
     * <p>There's no need to override this method unless the servlet implements new HTTP methods, beyond those
     * implemented by HTTP 1.1.
     *
     * @param req the http_request object that contains the request the client made of the servlet
     *
     * @param resp the http_response object that contains the response the servlet returns to the client
     */
    virtual void do_options(http_request& req, http_response& resp);

    /**
     * Returns the time the <code>http_request</code> object was last modified,
     * in milliseconds since midnight January 1, 1970 GMT.
     * If the time is unknown, this method returns a negative number (the default).
     *
     * <p>Servlets that support HTTP GET requests and can quickly determine
     * their last modification time should override this method.
     * This makes browser and proxy caches work more effectively,
     * reducing the load on server and network resources.
     *
     * @param req the http_request object that is sent to the servlet
     *
     * @return  a <code>long</code> integer specifying the time the <code>http_request</code>
     *              object was last modified, in milliseconds since midnight, January 1, 1970 GMT, or
     *              -1 if the time is not known
     */
    virtual long get_last_modified(http_request& req);

    /**
     * Constant to indicate that GET method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int GET_ALLOWED     = 1;
    /**
     * Constant to indicate that POST method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int POST_ALLOWED    = 1 << 1;
    /**
     * Constant to indicate that PUT method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int PUT_ALLOWED     = 1 << 2;
    /**
     * Constant to indicate that DELETE method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int DELETE_ALLOWED  = 1 << 3;
    /**
     * Constant to indicate that HEAD method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int HEAD_ALLOWED    = 1 << 4;
    /**
     * Constant to indicate that TRACE method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int TRACE_ALLOWED   = 1 << 5;
    /**
     * Constant to indicate that OPTIONS method is allowed.
     * @see #get_allowed_methods
     */
    constexpr static int OPTIONS_ALLOWED = 1 << 6;

    /**
     * \brief This method is needed by do_options, which adds all allowed by the servlet methods.
     *
     * This method is needed by do_options, which adds all allowed by the servlet methods. By default it
     * reports all methods to be allowed.
     *
     * There is no such method in Java Servlet API, because getOptions there reports only those methods allowed
     * which were overridden in the running servlet class. In C++ unfortunately it is impossible to find
     * which virtual methods were overridden, as comparison ov virtual method pointers are not supported.
     * There is a proposal to C++ stardatization commitee to allow virtual function pointers comparison
     * (see document P0191R1 <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0191r1.pdf"></a>)
     *
     * @return int mask of all allowed to this servlet methods.
     *
     * @see get_options()
     */
    virtual int get_allowed_methods();

private:
    void _maybe_set_last_modified(http_response &resp, long lastModifiedSec);

    static const std::string METHOD_DELETE;
    static const std::string METHOD_HEAD;
    static const std::string METHOD_GET;
    static const std::string METHOD_OPTIONS;
    static const std::string METHOD_POST;
    static const std::string METHOD_PUT;
    static const std::string METHOD_TRACE;

    static const std::string HEADER_IFMODSINCE;
    static const std::string HEADER_LASTMOD;

    servlet_config* _cfg;
};

}

#endif // SERVLET_SERVLET_H
