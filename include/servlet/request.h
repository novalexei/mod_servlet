/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_REQUEST_H
#define SERVLET_REQUEST_H

#include <vector>
#include <memory>

#include <servlet/uri.h>
#include <servlet/cookie.h>
#include <servlet/session.h>
#include <servlet/ssl.h>
#include <servlet/lib/io.h>
#include <servlet/lib/io_filter.h>
#include <servlet/lib/logger.h>

namespace servlet
{

class multipart_input;

/**
 * Defines an object to provide client request information to a servlet. The servlet container creates
 * a <code>http_equest</code> object and passes it as an argument to the servlet's
 * <code>http_servlet#service</code> method.
 *
 * <p> A <code>http_request</code> object provides data including parameter name and values,
 * attributes,
 */
class http_request
{
public:
    virtual ~http_request() noexcept {}

    /**
     * Returns the attributes map associated with this request.
     *
     * <p>Attributes can be set two ways. The servlet container may set attributes
     * to make available custom information about a request. For example, for
     * requests made using HTTPS, the attribute
     * <code>javax.servlet.request.X509Certificate</code> can be used to
     * retrieve information on the certificate of the client. Attributes can
     * also be set programatically using the returned map.
     *
     * @return an <code>tree_any_map</code> containing the attributes of this request.
     */
    virtual tree_any_map& get_attributes() = 0;

    /**
     * Const version of call #get_attributes() const.
     *
     * @return an const <code>tree_any_map</code> containing the attributes of this
     *         request.
     * @see #get_attributes
     */
    virtual const tree_any_map& get_attributes() const = 0;

    /**
     * Returns all the parameters of this request.
     *
     * Request parameters are extra information sent with the request.
     * For HTTP servlets, parameters are contained in the query string
     * or posted form data.
     *
     * If the parameter data was sent in the request body, such as occurs with
     * an HTTP POST request, then reading the body directly via
     * #get_input_stream or #get_multipart_input can interfere with the
     * execution of this method.
     *
     * @return a <code>tree_map</code> representing this request's parameters
     * @see #get_parameter
     * @see #get_parameters(const StringType&)
     * @see #get_input_stream
     * @see #get_multipart_input
     */
    virtual const std::map<std::string, std::vector<std::string>, std::less<>>& get_parameters() = 0;

    /**
     * Returns all the environment variables accessible from this request.
     *
     * <p>Environment variables can be set by other servler modules in order
     * to provide information to other modules (like PHP or CGI). This method
     * allows to access these variables</p>
     *
     * @return a <code>tree_map</code> representing this request's environment
     *         variables
     */
    virtual const std::map<string_view, string_view, std::less<>>& get_env() = 0;

    /**
     * Returns a boolean indicating whether this request was made using a secure
     * channel, such as HTTPS.
     *
     * @return a boolean indicating if the request was made using a secure channel
     */
    virtual bool is_secure() = 0;

    /**
     * Returns information about SSL connection if it is established.
     *
     * <p>SSL information will be provided only if this request was made
     * using secure channel (method #is_secure() returns <tt>true</tt>.
     * If Otherwise <code>nullptr</code> will be returned.</p>
     *
     * @return SSL_information of this connection or <code>nullptr</code>
     * @see #is_secure
     * @see SSL_information
     */
    virtual std::shared_ptr<SSL_information> ssl_information() = 0;

    /**
     * Returns the value of a request parameter as a reference to
     * <code>std::string</code>, or <code>null</code> if the parameter does
     * not exist.
     *
     * <p>You should only use this method when you are sure the parameter has only
     * one value. If the parameter might have more than one value, use
     * #get_parameters(const StringType&).
     *
     * <p>If you use this method with a multivalued parameter, the value returned
     * is equal to the first value in the array returned by <code>#get_parameters</code>.
     *
     * @tparam StringType a type comparable to <code>std::string</code>
     * @param name a <code>StringType</code> (might be <code>std::string</code>,
     *             <code>string_view</code> or <code>const char*</code>) specifying
     *             the name of the parameter
     * @return a reference to a <code>std::string</code> representing the single value
     *         of the parameter
     * @see #get_parameters()
     * @see #get_parameters(const StringType&)
     */
    template<typename StringType>
    const optional_ref<const std::string> get_parameter(const StringType& name)
    {
        const std::map<std::string, std::vector<std::string>, std::less<>>& params = get_parameters();
        auto it = params.find(name);
        return it == params.end() || it->second.empty() ? optional_ref<const std::string>{} :
               optional_ref<const std::string>{it->second.front()};
    }

    /**
     * Returns all values of a request parameter as a reference to
     * <code>std::vector</code>, or <code>null</code> if the parameters do
     * not exist.
     *
     * @tparam StringType a type comparable to <code>std::string</code>
     * @param name a <code>StringType</code> (might be <code>std::string</code>,
     *             <code>string_view</code> or <code>const char*</code>) specifying
     *             the name of the parameter
     * @return a reference to a <code>std::vector</code> representing all the values
     *         of the parameter
     * @see #get_parameters()
     * @see #get_parameter(const StringType&)
     */
    template<typename StringType>
    const optional_ref<const std::vector<std::string>> get_parameters(const StringType& name)
    {
        const std::map<std::string, std::vector<std::string>, std::less<>>& params = get_parameters();
        auto it = params.find(name);
        return it == params.end() ? optional_ref<const std::vector<std::string>>{} :
               optional_ref<const std::vector<std::string>>{it->second};
    }


    /**
     * Returns the name of the authentication scheme used to protect the
     * servlet. All servlet containers support basic, form and client
     * certificate authentication, and may additionally support digest
     * authentication. If the servlet is not authenticated empty string is
     * returned.
     *
     * <p> Same as the value of the CGI variable AUTH_TYPE.
     *
     * @return one of the static members BASIC_AUTH, FORM_AUTH, CLIENT_CERT_AUTH,
     *         DIGEST_AUTH (suitable for == comparison) or the
     *         container-specific string indicating the authentication scheme,
     *         or empty string if the request was not authenticated.
     */
    virtual string_view get_auth_type() = 0;

    /**
     * Returns an array containing all of the <code>cookie</code> objects the
     * client sent with this request. This method returns empty vector if
     * no cookies were sent.
     *
     * @return an array of all the <code>cookies</code> included with this
     *         request, or empty vector if the request has no cookies
     */
    virtual const std::vector<cookie>& get_cookies() = 0;

    /**
     * Returns the portion of the request URI that indicates the context of the
     * request. The context path always comes first in a request URI. The path
     * starts with a "/" character but does not end with a "/" character. For
     * servlets in the default (root) context, this method returns "". The
     * container does not decode this string.
     *
     * @return a <code>string_view</code> specifying the portion of the request URI
     *         that indicates the context of the request
     */
    virtual string_view get_context_path() const = 0;

    /**
     * Returns the part of this request's URL that calls the servlet. This path
     * starts with a "/" character and includes either the servlet name or a
     * path to the servlet, but does not include any extra path information or a
     * query string. Same as the value of the CGI variable SCRIPT_NAME.
     *
     * <p>This method will return an empty string ("") if the servlet used to
     * process this request was matched using the wildcard pattern.
     *
     * @return a <code>string_view</code> containing the name or path of the servlet
     *         being called, as specified in the request URL, decoded, or an
     *         empty string if the servlet used to process the request is
     *         matched using the wildcard pattern.
     */
    virtual string_view get_servlet_path() const = 0;

    /**
     * Returns the <code>URI></code> with which this request was called.
     *
     * @return full request <code>URI</code>
     */
    virtual const URI& get_request_uri() const = 0;

    /**
     * Returns any extra path information associated with the URL the client
     * sent when it made this request. The extra path information follows the
     * servlet path but precedes the query string and will start with a "/"
     * character.
     *
     * <p> This method returns empty view if there was no extra path
     * information.
     *
     * <p>Same as the value of the CGI variable PATH_INFO.
     *
     * @return a <code>string_view</code>, decoded by the web container, specifying
     *         extra path information that comes after the servlet path but
     *         before the query string in the request URL; or empty view
     *         if the URL does not have any extra path information
     */
    virtual string_view get_path_info() const = 0;

    /**
     * Returns the value of the specified request header as a
     * <code>std::string</code>. If the request did not include a header of the
     * specified name, this method returns empty string. If there are
     * multiple headers with the same name, this method returns the first head
     * in the request. The header name is case insensitive. You can use this
     * method with any request header.
     *
     * @param name a <code>std::string</code> specifying the header name
     * @return a <code>string_view</code> containing the value of the requested
     *         header, or empty view if the request does not have a
     *         header of that name
     */
    virtual string_view get_header(const std::string& name) const = 0;

    /**
     * Returns the value of the specified request header as a <code>long</code>
     * value that represents a <code>Date</code> object. Use this method with
     * headers that contain dates, such as <code>If-Modified-Since</code>.
     *
     * <p> The date is returned as the number of milliseconds since
     * January 1, 1970 GMT. The header name is case insensitive.
     *
     * <p> If the request did not have a header of the specified name, this method
     * returns -1. If the header can't be converted to a date, the method throws
     * an <code>stack_bad_cast</code>.
     *
     * @param name a <code>std::string</code> specifying the name of the header
     * @return a <code>long</code> value representing the date specified in the
     *         header expressed as the number of milliseconds since January 1,
     *         1970 GMT, or -1 if the named header was not included with the
     *         request
     * @exception stack_bad_cast If the header value can't be converted to a date
     */
    virtual long get_date_header(const std::string& name) const = 0;

    /**
     * Returns the MIME type of the body of the request, or empty string if
     * the type is not known. For HTTP servlets, same as the value of the CGI
     * variable CONTENT_TYPE.
     *
     * @return a <code>string_view</code> containing the name of the MIME type
     *         of the request, or empty view if the type is not known
     */
    virtual string_view get_content_type() const = 0;

    /**
     * Returns the length, in bytes, of the request body and made available by
     * the input stream, or -1 if the length is not known. For HTTP servlets,
     * same as the value of the CGI variable CONTENT_LENGTH.
     *
     * @return an integer containing the length of the request body or -1 if the
     *         length is not known.
     */
    virtual long get_content_length() const = 0;

    /**
     * Fills the <code>std::vector</code> with all the header values associated with the
     * specified header name.
     *
     * @param name Header name to look up
     *
     * @param headers The values for the specified header. These are the raw values so
     *         if multiple values are specified in a single header that will be
     *         returned as a single header value.
     */
    virtual void get_headers(const std::string& name, std::vector<std::string>& headers) const = 0;

    /**
     * Fills the <code>std::vector</code> with all the request header values.
     *
     * @param headers The values for all the header values.
     */
    virtual void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const = 0;

    /**
     * Returns the name of the HTTP method with which this request was made, for
     * example, GET, POST, or PUT. Same as the value of the CGI variable
     * REQUEST_METHOD.
     *
     * @return a <code>string_view</code> specifying the name of the method with
     *         which this request was made
     */
    virtual string_view get_method() const = 0;

    /**
     * Returns any extra path information after the servlet name but before the
     * query string, and translates it to a real path. Same as the value of the
     * CGI variable PATH_TRANSLATED.
     *
     * <p> If the URL does not have any extra path information, this method returns
     * empty view or the servlet container cannot translate the virtual
     * path to a real path for any reason (such as when the web application is
     * executed from an archive). The web container does not decode this string.
     *
     * @return a <code>string_view</code> specifying the real path, or
     *         empty view if the URL does not have any extra path
     *         information
     */
    virtual string_view get_path_translated() const = 0;

    /**
     * Returns the name of the scheme used to make this request, for example,
     * <code>http</code> or <code>https</code>. Different schemes have different
     * rules for constructing URLs, as noted in RFC 1738.
     *
     * @return a <code>string_view</code> containing the name of the scheme used to
     *         make this request
     */
    virtual string_view get_scheme() const = 0;

    /**
     * Returns the name and version of the protocol the request uses in the form
     * <i>protocol/majorVersion.minorVersion</i>, for example, HTTP/1.1. For
     * HTTP servlets, the value returned is the same as the value of the CGI
     * variable <code>SERVER_PROTOCOL</code>.
     *
     * @return a <code>string_view</code> containing the protocol name and version
     *         number
     */
    virtual string_view get_protocol() const = 0;


    /**
     * Returns the Internet Protocol (IP) address of the client or last proxy
     * that sent the request. For HTTP servlets, same as the value of the CGI
     * variable <code>REMOTE_ADDR</code>.
     *
     * @return a <code>string_view</code> containing the IP address of the client
     *         that sent the request
     */
    virtual string_view get_client_addr() const = 0;

    /**
     * Returns the fully qualified name of the client or the last proxy that
     * sent the request if available. If the engine cannot or chooses not to
     * resolve the hostname, this method returns the dotted-string
     * form of the IP address. For HTTP servlets, same as the value of the CGI
     * variable <code>REMOTE_HOST</code>.
     *
     * @return a <code>string_view</code> containing the fully qualified name of the
     *         client
     */
    virtual string_view get_client_host() const = 0;

    /**
     * Returns the Internet Protocol (IP) source port of the client or last
     * proxy that sent the request.
     *
     * @return an integer specifying the port number
     */
    virtual uint16_t    get_client_port() const = 0;

    /**
     * Returns the login of the user making this request, if the user has been
     * authenticated, or empty view if the user has not been
     * authenticated. Whether the user name is sent with each subsequent request
     * depends on the browser and type of authentication. Same as the value of
     * the CGI variable REMOTE_USER.
     *
     * @return a <code>string_view</code> specifying the login of the user making
     *         this request, or empty view if the user login is not known
     */
    virtual string_view get_remote_user() const = 0;

    /**
     * Returns the Internet Protocol (IP) address of the interface on which the
     * request was received.
     *
     * @return a <code>string_view</code> containing the IP address on which the
     *         request was received.
     */
    virtual string_view get_local_addr() const = 0;

    /**
     * Returns the host name of the Internet Protocol (IP) interface on which
     * the request was received.
     *
     * @return a <code>string_view</code> containing the host name of the IP on which
     *         the request was received.
     */
    virtual string_view get_local_host() const = 0;

    /**
     * Returns the Internet Protocol (IP) port number of the interface on which
     * the request was received.
     *
     * @return an integer specifying the port number
     */
    virtual uint16_t    get_local_port() const = 0;


    /**
     * Returns the host name of the server to which the request was sent. It is
     * the value of the part before ":" in the <code>Host</code> header value,
     * if any, or the resolved server name, or the server IP address.
     *
     * @return a <code>string_view</code> containing the name of the server
     */
    virtual string_view get_server_name() const = 0;

    /**
     * Returns the port number to which the request was sent. It is the value of
     * the part after ":" in the <code>Host</code> header value, if any, or the
     * server port where the client connection was accepted on.
     *
     * @return an integer specifying the port number
     */
    virtual uint16_t    get_server_port() const = 0;

    /**
     * Redirects the request to a specified local URI within the server.
     *
     * <p>Redirect processed internally without returning to the client, which is
     * the difference of this method from http_response#send_redirect. This
     * method expects only local path, not the full URI.
     *
     * <p>If the provided URI is an absolute path it will be used as is if
     * <code>from_context_path</code> argument is <code>false</code>. Otherwise
     * current context path will be prepended to the URI.
     *
     * <p>If the provided URI is a relative path it will be resolved against
     * current request URI (see #get_request_uri).
     *
     * @param redirectURI URI to redirect the request to
     * @param from_context_path <code>true</code> if the redirectURI should be
     *                          resolved against the current context path
     * @see http_response#send_redirect
     * @see #get_request_uri
     */
    virtual void forward(const std::string &redirectURI, bool from_context_path = true) = 0;

    /**
     * Includes the response of the specified local URI into the current response.
     *
     * <p>Include processed internally and locally. The provided URI follows the
     * same rules as <code>redirectURI</code> in #forward call.
     *
     * @param includeURI URI to include into current response
     * @param from_context_path <code>true</code> if the includeURI should be
     *                          resolved against the current context path
     * @see #forward
     * @see http_response#send_redirect
     * @see #get_request_uri
     */
    virtual int include(const std::string &includeURI, bool from_context_path = true) = 0;

    /**
     * Returns the current <code>http_session</code> associated with this request
     * or, if there is no current session returns a new session.
     *
     * <p>To make sure the session is properly maintained, you must call this
     * method before the response is committed. If the container is using
     * cookies to maintain session integrity.
     *
     * <p>If the intent is to get only the session which already exists use
     * method #has_session() to ensure the session exists before calling this method.
     *
     * @return the <code>http_session</code> associated with this request.
     * @see #has_session
     */
    virtual http_session &get_session() = 0;

    /**
     * Returns <code>true</code> if the <code>http_session</code> associated with
     * this request exists.
     *
     * @return the <code>true</code> if session associated with this request exists.
     */
    virtual bool has_session() = 0;

    /**
     * Invalidates the session assosiated with this request if it exists.
     */
    virtual void invalidate_session() = 0;

    /**
     * Retrieves the body of the request as binary data using a <code>std::istream</code>.
     *
     * If this request has content type of "multipart/form-data" method #get_multipart_input
     * also can be called. If #get_multipart_input has already been called than
     * <code>get_input_stream</code> will return the input stream for the current part
     * of the multipart stream.
     *
     * @return a <code>std::istream</code> object containing the body of the request
     * @see #get_multipart_input
     */
    virtual std::istream& get_input_stream() = 0;

    /**
     * Retrieves the body of the request as binary data if the input is a multipart stream.
     *
     * This method can only retrieve input only if the current request has content type of
     * "multipart/form-data". It it not so use method #get_input_stream to read the
     * request's input, while this method will throw <code>stack_io_exception</code>.
     *
     * <p>If this method is called after calling #get_input_stream
     * <code>stack_io_exception</code> will be thrown.
     *
     * @return a <code>std::istream</code> object containing the body of the request
     * @throws stack_io_exception if this request is not a multipart or if the stream
     *                            is already retrieved with #get_input_stream
     * @see #get_input_stream
     * @see multipart_input
     */
    virtual multipart_input& get_multipart_input() = 0;

    /**
     * Returns <code>true</code> if current request is multipart.
     *
     * This method should be called before calling #get_multipart_input
     * to avoid exceptions. If this method returns <code>false</code>
     * #get_input_stream should be called instead.
     *
     * @return <code>true</code> if current request is multipart.
     * @see #get_multipart_input
     * @see #get_input_stream
     */
    virtual bool is_multipart() const = 0;
};

/**
 * This class represents a multipart input stream of a
 * <code>multipart/form-data</code> request body. Each part of this class may
 * represent either an uploaded file or form data.
 *
 * <p>This class is built on top of input stream of a request and it doesn't cache
 * any data.
 *
 * <p>The parts can be navigated only in forward direction using #to_next_part
 * method. The usual use of this class can be illustrated by the following code:
 *
 * ~~~~~~{.cpp}
 *  multipart_input in = request.get_multipart_input();
 *  while (in.to_next_part())
 *  {
 *      optional_ref<const std::string> header_value = in.get_header("header-name");
 *      if (header_value && *header_value == "expected-value")
 *      {
 *          std::istream& part_in = in.get_input_stream();
 *          // process part_in
 *          ...
 *      }
 *  }
 * ~~~~~~
 */
class multipart_input
{
public:
    virtual ~multipart_input() noexcept = default;

    /**
     * Obtain all the headers of the curent part.
     *
     * @return All the headers of the current part.
     */
    virtual const std::map<std::string, std::vector<std::string>, std::less<>>& get_headers() const = 0;

    /**
     * Obtains the value of the specified part header as a reference to a
     * <code>std::string</code>. If there are multiple headers with the
     * same name, this method returns the first header in the part.
     *
     * @tparam StringType a type comparable to <code>std::string</code>
     * @param name  Header name
     * @return      The header value or empty reference if the header is not
     *              present
     */
    template<typename StringType>
    optional_ref<const std::string> get_header(const StringType& name) const
    {
        const std::map<std::string, std::vector<std::string>, std::less<>>& headers = get_headers();
        auto it = headers.find(name);
        return it == headers.end() || it->second.empty() ? optional_ref<const std::string>{} :
               optional_ref<const std::string>{it->second.front()};
    }

    /**
     * Obtain all the values of the specified part header.
     *
     * @tparam StringType a type comparable to <code>std::string</code>
     * @param name The name of the header of interest.
     * @return All the values of the specified part header. If the part did not
     *         include any headers of the specified name, this method returns an
     *         empty <code>std::vector</code>.
     */
    template<typename StringType>
    optional_ref<const std::vector<std::string>> get_headers(const StringType& name) const
    {
        const std::map<std::string, std::vector<std::string>, std::less<>>& headers = get_headers();
        auto it = headers.find(name);
        return it == headers.end() ? optional_ref<const std::vector<std::string>>{} :
               optional_ref<const std::vector<std::string>>{it->second};
    }

    /**
     * Obtain the content type passed by the browser.
     *
     * @return The content type passed by the browser or <code>null</code> if
     *         not defined.
     */
    optional_ref<const std::string> get_content_type() const { return get_header("Content-Type"); }

    /**
     * Obtain the name of the field in the multipart form corresponding to this part.
     *
     * @return The name of the field in the multipart form corresponding to this part.
     */
    optional_ref<const std::string> get_name() const { return get_header("name"); }

    /**
     * If this part represents an uploaded file, gets the file name submitted
     * in the upload. Returns empty reference if no file name is available or if
     * this part is not a file upload.
     *
     * @return the submitted file name or empty reference.
     */
    optional_ref<const std::string> get_submitted_filename() const;

    /**
     * Obtain an <code>std::itream</code> that can be used to retrieve the
     * contents of the current part.
     *
     * @return An std::istream for the contents of the current part
     */
    virtual std::istream& get_input_stream() = 0;

    /**
     * Moves this multipart_input to the next part.
     *
     * @return <code>true</code> if the next part exists and readable,
     *         otherwise <code>false</code>
     */
    virtual bool to_next_part() = 0;
};

/**
 * Provides a convenient implementation of the http_request interface that
 * can be subclassed by developers wishing to adapt the request to a http_servlet.
 * This class implements the Wrapper or Decorator pattern. Methods default to
 * calling through to the wrapped request object.
 *
 * This wrapper also provides #filter method to filter <code>std::istream</code>
 * of the wrapped http_request.
 *
 * @see http_request
 */
class http_request_wrapper : public http_request
{
public:
    /**
     * Constructs a request object wrapping the given request.
     *
     * @param req The request to wrap
     */
    http_request_wrapper(http_request& req) : _req{req} {}

    ~http_request_wrapper() noexcept override {}

    /**
     * Returns wrapped request innstance.
     * @return reference to wrapped request instance
     */
    http_request& get_wrapped_request() { return _req; }
    /**
     * Returns wrapped request innstance.
     * @return const reference to wrapped request instance
     */
    const http_request& get_wrapped_request() const { return _req; }

    tree_any_map& get_attributes() override { return _req.get_attributes(); }
    const tree_any_map& get_attributes() const override { return _req.get_attributes(); }

    const std::map<std::string, std::vector<std::string>, std::less<>>& get_parameters() override
    { return _req.get_parameters(); }
    const std::map<string_view, string_view, std::less<>>& get_env() override { return _req.get_env(); };
    bool is_secure() override { return _req.is_secure(); }
    std::shared_ptr<SSL_information> ssl_information() override { return _req.ssl_information(); }

    string_view get_auth_type() override { return _req.get_auth_type(); }
    const std::vector<cookie>& get_cookies() override { return _req.get_cookies(); }
    string_view get_context_path() const override { return _req.get_context_path(); }
    string_view get_servlet_path() const override { return _req.get_servlet_path(); }
    const URI& get_request_uri() const override { return _req.get_request_uri(); }
    string_view get_path_info() const override { return _req.get_path_info(); }
    string_view get_header(const std::string& name) const override { return _req.get_header(name); }
    long get_date_header(const std::string& name) const override { return _req.get_date_header(name); }

    string_view get_content_type() const override { return _req.get_content_type(); }
    long get_content_length() const override { return _req.get_content_length(); }

    void get_headers(const std::string& name, std::vector<std::string>& headers) const override
    { return _req.get_headers(name, headers); }
    void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const override
    { return _req.get_headers(headers); }

    string_view get_method() const override { return _req.get_method(); }

    string_view get_path_translated() const override { return _req.get_method(); }
    string_view get_scheme() const override { return _req.get_method(); }
    string_view get_protocol() const override { return _req.get_method(); }

    string_view get_client_addr() const override { return _req.get_client_addr(); }
    string_view get_client_host() const override { return _req.get_client_host(); }
    uint16_t    get_client_port() const override { return _req.get_client_port(); }
    string_view get_remote_user() const override { return _req.get_remote_user(); }

    string_view get_local_addr() const override { return _req.get_local_addr(); }
    string_view get_local_host() const override { return _req.get_local_host(); }
    uint16_t    get_local_port() const override { return _req.get_local_port(); }

    uint16_t    get_server_port() const override { return _req.get_server_port(); }
    string_view get_server_name() const override { return _req.get_server_name(); }

    void forward(const std::string &redirectURL, bool from_context_path = true) override
    { _req.forward(redirectURL, from_context_path); }
    int include(const std::string &includeURL, bool from_context_path = true) override
    { return _req.include(includeURL, from_context_path); }

    http_session &get_session() override { return _req.get_session(); }
    bool has_session() override { return _req.has_session(); }
    void invalidate_session() override { _req.invalidate_session(); }

    bool is_multipart() const override { return _req.is_multipart(); }

    std::istream& get_input_stream() override;
    multipart_input& get_multipart_input() override;

protected:

    /**
     * Provides input filter for the http_request#get_input_stream or
     * http_request#get_multipart_input.
     *
     * If this method returns valid in_filter it will be applied to
     * <code>std::istream</code> returned by #get_input_stream method or to
     * <code>multipart_input</code> returned by #get_multipart_input method. It also
     * can return <code>nullptr</code> in which case <code>std::istream</code>
     * will be returned without any filtering applied.
     *
     * This method is called only once on the initialization of request input
     * stream. This filter will be automatically deleted on destruction of the
     * request object.
     *
     * @return an in_filter to apply to http_request input or <code>nullptr</code>
     * @see http_request#get_input_stream
     * @see http_request#get_multipart_input
     */
    virtual in_filter* filter() { return nullptr; }

private:
    http_request& _req;
    optional_ptr<std::istream> _in;
    optional_ptr<multipart_input> _multipart_in;
};

} // end of servlet namespace

#endif // SERVLET_REQUEST_H
