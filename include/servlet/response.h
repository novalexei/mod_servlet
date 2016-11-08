#ifndef SERVLET_RESPONSE_H
#define SERVLET_RESPONSE_H

#include <ostream>
#include <chrono>
#include <experimental/string_view>

#include <http_protocol.h>

#include <servlet/cookie.h>
#include <servlet/lib/io.h>
#include <servlet/lib/io_filter.h>
#include <servlet/lib/optional.h>

namespace servlet
{

using std::experimental::string_view;

/**
 * Defines an object to assist a servlet in sending a response to the client.
 * The servlet container creates a <code>http_response</code> object and
 * passes it as an argument to the servlet's <code>service</code> method.
 *
 * <p> To send binary data in a response, use the <code>std::ostream</code> returned by
 * #get_output_stream
 */
class http_response
{
public:
    virtual ~http_response() noexcept {}

    /**
     * Adds the specified cookie to the response. This method can be called
     * multiple times to set more than one cookie.
     *
     * @param c the Cookie to return to the client
     */
    virtual void add_cookie(const cookie& c) = 0;

    /**
     * Adds a response header with the given name and value. This method allows
     * response headers to have multiple values.
     *
     * @param name the name of the header
     * @param value the additional header value If it contains octet string, it
     *            should be encoded according to RFC 2047 (http://www.ietf.org/rfc/rfc2047.txt)
     * @see #set_header
     */
    virtual void add_header(const std::string &name, const std::string &value) = 0;

    /**
     * Adds a response header with the given name and date-value. The date is
     * specified as <code>std::chrono::time_point</code>. This method allows
     * response headers to have multiple values.
     *
     * @tparam Clock clock type of the date
     * @tparam Dur duration type of the date
     * @param name the name of the header to set
     * @param date the additional date value
     * @see #set_date_header
     */
    template<typename Clock, typename Dur>
    void add_date_header(const std::string &name, const std::chrono::time_point<Clock, Dur> &date)
    {
        add_date_header(name, std::chrono::duration_cast<std::chrono::seconds>(date.time_since_epoch()).count());
    }

    /**
     * Adds a response header with the given name and date-value. The date is
     * specified in terms of milliseconds since the epoch. This method allows
     * response headers to have multiple values.
     *
     * @param name the name of the header to set
     * @param timeSec the additional date value
     * @see #set_date_header
     */
    virtual void add_date_header(const std::string &name, long timeSec) = 0;

    /**
     * Sets a response header with the given name and value. If the header had
     * already been set, the new value overwrites the previous one. The
     * <code>#contains_header</code> method can be used to test for the presence
     * of a header before setting its value.
     *
     * @param name the name of the header
     * @param value the header value If it contains octet string, it should be
     *            encoded according to RFC 2047 (http://www.ietf.org/rfc/rfc2047.txt)
     * @see #contains_header
     * @see #add_header
     */
    virtual void set_header(const std::string &name, const std::string &value) = 0;

    /**
     * Sets a response header with the given name and date-value. The date is
     * specified as <code>std::chrono::time_point</code>. If the header had
     * already been set, the new value overwrites the previous one. The
     * <code>#contains_header</code> method can be used to test for the presence
     * of a header before setting its value.
     *
     * @tparam Clock clock type of the date
     * @tparam Dur duration type of the date
     * @param name the name of the header to set
     * @param date the assigned date value
     * @see #contains_header
     * @see #add_date_header
     */
    template<typename Clock, typename Dur>
    void set_date_header(const std::string &name, const std::chrono::time_point<Clock, Dur> &date)
    {
        set_date_header(name, std::chrono::duration_cast<std::chrono::seconds>(date.time_since_epoch()).count());
    }
    /**
     * Sets a response header with the given name and date-value. The date is
     * specified in terms of milliseconds since the epoch. If the header had
     * already been set, the new value overwrites the previous one. The
     * <code>#contains_header</code> method can be used to test for the presence
     * of a header before setting its value.
     *
     * @param name the name of the header to set
     * @param date the assigned date value
     * @see #contains_header
     * @see #add_date_header
     */
    virtual void set_date_header(const std::string &name, long date) = 0;

    /**
     * Returns a boolean indicating whether the named response header has
     * already been set.
     *
     * @param name the header name
     * @return <code>true</code> if the named response header has already been
     *         set; <code>false</code> otherwise
     */
    virtual bool contains_header(const std::string &name) const = 0;

    /**
     * Return the value for the specified header, or empty string if this
     * header has not been set.  If more than one value was added for this
     * name, only the first is returned; use
     * get_headers(const std::string&, std::vector<std::string>&)
     * to retrieve all of them.
     *
     * @param name Header name to look up
     *
     * @return The first value for the specified header. This is the raw value
     *         so if multiple values are specified in the first header then they
     *         will be returned as a single header value .
     */
    virtual string_view get_header(const std::string& name) const = 0;

    /**
     * Return the date value for the specified header, or <code>-1</code> if this
     * header has not been set.  If date value cannot be parsed from the header
     * value <code>stack_bad_cast</code> exception will be thrown.
     * to retrieve all of them.
     *
     * @param name Header name to look up
     *
     * @return The first value for the specified header converted into long.
     */
    virtual long get_date_header(const std::string& name) const = 0;

    /**
     * Fills the <code>std::vector</code> with all the header values associated with the
     * specified header name.
     *
     * @param name Header name to look up
     * @param headers The values for the specified header. These are the raw values so
     *         if multiple values are specified in a single header that will be
     *         returned as a single header value.
     */
    virtual void get_headers(const std::string& name, std::vector<std::string>& headers) const = 0;

    /**
     * Fills the <code>std::vector</code> with all the response header values.
     *
     * @param headers The values for the headers.
     */
    virtual void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const = 0;

    /**
     * Returns the content type used for the MIME body sent in this response.
     * The content type proper must have been specified using
     * #set_content_type before the response is committed. If no content
     * type has been specified, this method returns null.
     *
     * @return a <code>string_view</code> specifying the content type, for example,
     *         <code>text/html; charset=UTF-8</code>, or null
     */
    virtual string_view get_content_type() const = 0;

    /**
     * Sets the content type of the response being sent to the client, if the
     * response has not been committed yet. The given content type may include a
     * character encoding specification, for example,
     * <code>text/html;charset=UTF-8</code>.
     *
     * <p> This method may be called repeatedly to change content type and character
     * encoding. This method has no effect if called after the response has been
     * committed. It does not set the response's character encoding if it is
     * called after <code>get_output_stream</code> has been called or after the response
     * has been committed.
     *
     * @param type a <code>std::string</code> specifying the MIME type of the content
     * @see #get_output_stream
     */
    virtual void set_content_type(const std::string &type) = 0;


    /**
     * Sets the length of the content body in the response In HTTP servlets,
     * this method sets the HTTP Content-Length header.
     *
     * @param len an <code>std::size_t</code> specifying the length of the
     *            content being returned to the client; sets the
     *            Content-Length header
     */
    virtual void set_content_length(std::size_t len) = 0;

    /**
     * Sends a temporary redirect response to the client using the specified
     * redirect location URL. This method can accept relative URLs; the servlet
     * container must convert the relative URL to an absolute URL before sending
     * the response to the client. If the location is relative without a leading
     * '/' the container interprets it as relative to the current request URI.
     * If the location is relative with a leading '/' the container interprets
     * it as relative to the servlet container root.
     *
     * <p>If the response has already been committed, this method throws an
     * IllegalStateException. After using this method, the response should be
     * considered to be committed and should not be written to.
     *
     * @param redirectURL the redirect location URL
     */
    virtual void send_redirect(const std::string &redirectURL) = 0;

    /**
     * Sets the status code for this response. This method is used to set the
     * return status code when there is no error (for example, for the status
     * codes SC_OK or SC_MOVED_TEMPORARILY).
     *
     * <p> The container clears the buffer and sets the Location header, preserving
     * cookies and other headers.
     *
     * @param sc the status code
     */
    virtual void set_status(int sc) = 0;

    /**
     * Get the HTTP status code for this Response.
     *
     * @return The HTTP status code for this Response
     */
    virtual int get_status() const = 0;

    /**
     * Returns a <code>std::ostream</code> suitable for writing binary data in
     * the response. The servlet container does not encode the binary data.
     *
     * @return a <code>std::ostream</code> for writing binary data
     */
    virtual std::ostream& get_output_stream() = 0;

    /*
     * Server status codes; see RFC 2068.
     */

    /**
     * Status code (100) indicating the client can continue.
     */
    static constexpr int SC_CONTINUE                        = 100;

    /**
     * Status code (101) indicating the server is switching protocols according
     * to Upgrade header.
     */
    static constexpr int SC_SWITCHING_PROTOCOLS             = 101;


    /**
     * Status code (200) indicating the request succeeded normally.
     */
    static constexpr int SC_OK                              = 200;

    /**
     * Status code (201) indicating the request succeeded and created a new
     * resource on the server.
     */
    static constexpr int SC_CREATED                         = 201;

    /**
     * Status code (202) indicating that a request was accepted for processing,
     * but was not completed.
     */
    static constexpr int SC_ACCEPTED                        = 202;

    /**
     * Status code (203) indicating that the meta information presented by the
     * client did not originate from the server.
     */
    static constexpr int SC_NON_AUTHORITATIVE_INFORMATION   = 203;

    /**
     * Status code (204) indicating that the request succeeded but that there
     * was no new information to return.
     */
    static constexpr int SC_NO_CONTENT                      = 204;

    /**
     * Status code (205) indicating that the agent <em>SHOULD</em> reset the
     * document view which caused the request to be sent.
     */
    static constexpr int SC_RESET_CONTENT                   = 205;

    /**
     * Status code (206) indicating that the server has fulfilled the partial
     * GET request for the resource.
     */
    static constexpr int SC_PARTIAL_CONTENT                 = 206;


    /**
     * Status code (300) indicating that the requested resource corresponds to
     * any one of a set of representations, each with its own specific location.
     */
    static constexpr int SC_MULTIPLE_CHOICES                = 300;

    /**
     * Status code (301) indicating that the resource has permanently moved to a new
     * location, and that future references should use a new URI with their requests.
     */
    static constexpr int SC_MOVED_PERMANENTLY               = 301;

    /**
     * Status code (302) indicating that the resource has temporarily moved to
     * another location, but that future references should still use the
     * original URI to access the resource. This definition is being retained
     * for backwards compatibility. SC_FOUND is now the preferred definition.
     */
    static constexpr int SC_MOVED_TEMPORARILY               = 302;

    /**
     * Status code (302) indicating that the resource reside temporarily under a
     * different URI. Since the redirection might be altered on occasion, the
     * client should continue to use the Request-URI for future requests.
     * (HTTP/1.1) To represent the status code (302), it is recommended to
     * use this variable.
     */
    static constexpr int SC_FOUND                           = 302;

    /**
     * Status code (303) indicating that the response to the request can be
     * found under a different URI.
     */
    static constexpr int SC_SEE_OTHER                       = 303;

    /**
     * Status code (304) indicating that a conditional GET operation found that
     * the resource was available and not modified.
     */
    static constexpr int SC_NOT_MODIFIED                    = 304;

    /**
     * Status code (305) indicating that the requested resource <em>MUST</em> be
     * accessed through the proxy given by the <code><em>Location</em></code> field.
     */
    static constexpr int SC_USE_PROXY                       = 305;

    /**
     * Status code (307) indicating that the requested resource resides
     * temporarily under a different URI. The temporary URI <em>SHOULD</em> be
     * given by the <code><em>Location</em></code> field in the response.
     */
    static constexpr int SC_TEMPORARY_REDIRECT              = 307;


    /**
     * Status code (400) indicating the request sent by the client was
     * syntactically incorrect.
     */
    static constexpr int SC_BAD_REQUEST                     = 400;

    /**
     * Status code (401) indicating that the request requires HTTP
     * authentication.
     */
    static constexpr int SC_UNAUTHORIZED                    = 401;

    /**
     * Status code (402) reserved for future use.
     */
    static constexpr int SC_PAYMENT_REQUIRED                = 402;

    /**
     * Status code (403) indicating the server understood the request but
     * refused to fulfill it.
     */
    static constexpr int SC_FORBIDDEN                       = 403;

    /**
     * Status code (404) indicating that the requested resource is not
     * available.
     */
    static constexpr int SC_NOT_FOUND                       = 404;

    /**
     * Status code (405) indicating that the method specified in the
     * <code><em>Request-Line</em></code> is not allowed for the resource
     * identified by the <code><em>Request-URI</em></code>.
     */
    static constexpr int SC_METHOD_NOT_ALLOWED              = 405;

    /**
     * Status code (406) indicating that the resource identified by the
     * request is only capable of generating response entities which have
     * content characteristics not acceptable according to the accept
     * headers sent in the request.
     */
    static constexpr int SC_NOT_ACCEPTABLE                  = 406;

    /**
     * Status code (407) indicating that the client <em>MUST</em> first
     * authenticate itself with the proxy.
     */
    static constexpr int SC_PROXY_AUTHENTICATION_REQUIRED   = 407;

    /**
     * Status code (408) indicating that the client did not produce a request
     * within the time that the server was prepared to wait.
     */
    static constexpr int SC_REQUEST_TIMEOUT                 = 408;

    /**
     * Status code (409) indicating that the request could not be completed due
     * to a conflict with the current state of the resource.
     */
    static constexpr int SC_CONFLICT                        = 409;

    /**
     * Status code (410) indicating that the resource is no longer available at
     * the server and no forwarding address is known. This condition
     * <em>SHOULD</em> be considered permanent.
     */
    static constexpr int SC_GONE                            = 410;

    /**
     * Status code (411) indicating that the request cannot be handled without a
     * defined <code><em>Content-Length</em></code>.
     */
    static constexpr int SC_LENGTH_REQUIRED                 = 411;

    /**
     * Status code (412) indicating that the precondition given in one or more
     * of the request-header fields evaluated to false when it was tested on the
     * server.
     */
    static constexpr int SC_PRECONDITION_FAILED             = 412;

    /**
     * Status code (413) indicating that the server is refusing to process the
     * request because the request entity is larger than the server is willing
     * or able to process.
     */
    static constexpr int SC_REQUEST_ENTITY_TOO_LARGE        = 413;

    /**
     * Status code (414) indicating that the server is refusing to service the
     * request because the <code><em>Request-URI</em></code> is longer than the
     * server is willing to interpret.
     */
    static constexpr int SC_REQUEST_URI_TOO_LONG            = 414;

    /**
     * Status code (415) indicating that the server is refusing to service the
     * request because the entity of the request is in a format not supported by
     * the requested resource for the requested method.
     */
    static constexpr int SC_UNSUPPORTED_MEDIA_TYPE          = 415;

    /**
     * Status code (416) indicating that the server cannot serve the requested
     * byte range.
     */
    static constexpr int SC_REQUESTED_RANGE_NOT_SATISFIABLE = 416;

    /**
     * Status code (417) indicating that the server could not meet the
     * expectation given in the Expect request header.
     */
    static constexpr int SC_EXPECTATION_FAILED              = 417;


    /**
     * Status code (500) indicating an error inside the HTTP server which
     * prevented it from fulfilling the request.
     */
    static constexpr int SC_INTERNAL_SERVER_ERROR           = 500;

    /**
     * Status code (501) indicating the HTTP server does not support the
     * functionality needed to fulfill the request.
     */
    static constexpr int SC_NOT_IMPLEMENTED                 = 501;

    /**
     * Status code (502) indicating that the HTTP server received an invalid
     * response from a server it consulted when acting as a proxy or gateway.
     */
    static constexpr int SC_BAD_GATEWAY                     = 502;

    /**
     * Status code (503) indicating that the HTTP server is temporarily
     * overloaded, and unable to handle the request.
     */
    static constexpr int SC_SERVICE_UNAVAILABLE             = 503;

    /**
     * Status code (504) indicating that the server did not receive a timely
     * response from the upstream server while acting as a gateway or proxy.
     */
    static constexpr int SC_GATEWAY_TIMEOUT                 = 504;

    /**
     * Status code (505) indicating that the server does not support or refuses
     * to support the HTTP protocol version that was used in the request message.
     */
    static constexpr int SC_HTTP_VERSION_NOT_SUPPORTED      = 505;
};

/**
 * Provides a convenient implementation of the http_response interface
 * that can be subclassed by developers wishing to adapt the response from a
 * http_servlet. This class implements the Wrapper or Decorator pattern.
 * Methods default to calling through to the wrapped response object.
 *
 * This wrapper also provides #filter method to filter <code>std::ostream</code>
 * of the wrapped http_response.
 */
class http_response_wrapper : public http_response
{
public:

    /**
     * Constructs a response adaptor wrapping the given response.
     *
     * @param resp The response to be wrapped
     */
    http_response_wrapper(http_response&resp) : _resp{resp} {}
    /**
     * Overridden destructor
     */
    ~http_response_wrapper() override {}

    void add_cookie(const cookie& c) override { _resp.add_cookie(c); }
    void add_header(const std::string &name, const std::string &value) override { _resp.add_header(name, value); }
    void add_date_header(const std::string &name, long timeSec) override { _resp.add_date_header(name, timeSec); }
    void set_header(const std::string &name, const std::string &value) override { _resp.set_header(name, value); }
    void set_date_header(const std::string &name, long timeSec) override { _resp.set_date_header(name, timeSec); }
    bool contains_header(const std::string &name) const override { return _resp.contains_header(name); }

    string_view get_header(const std::string& name) const override { return _resp.get_header(name); }
    long get_date_header(const std::string& name) const override { return _resp.get_date_header(name); }
    void get_headers(const std::string& name, std::vector<std::string>& headers) const override
    { _resp.get_headers(name, headers); }
    void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const override
    { _resp.get_headers(headers); }

    string_view get_content_type() const override { return _resp.get_content_type(); }
    void set_content_type(const std::string &content_type) override { _resp.set_content_type(content_type); }
    void set_content_length(std::size_t content_length) override { _resp.set_content_length(content_length); }

    void send_redirect(const std::string &redirectURL) override { _resp.send_redirect(redirectURL); }

    void set_status(int sc) override { _resp.set_status(sc); }
    int get_status() const override { return _resp.get_status(); }

    std::ostream& get_output_stream() override;
protected:

    /**
     * Provides output filter for the http_response#get_output_stream.
     *
     * If this method returns valid out_filter it will be applied to
     * <code>std::ostream</code> returned by #get_output_stream method. It also
     * can return <code>nullptr</code> in which case <code>std::ostream</code>
     * will be returned without any filtering applied.
     *
     * @return an out_filter to apply to <code>std::ostream</code> or <code>nullptr</code>
     * @see http_response#get_output_stream
     */
    virtual out_filter *filter() { return nullptr; }

private:
    http_response& _resp;
    optional_ptr<std::ostream> _out;
};

} // end of servlet namespace

#endif // SERVLET_RESPONSE_H
