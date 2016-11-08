/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_COOKIE_H
#define MOD_SERVLET_COOKIE_H

#include <string>

namespace servlet
{

/**
 * Creates a cookie, a small amount of information sent by a servlet to a Web
 * browser, saved by the browser, and later sent back to the server. A cookie's
 * value can uniquely identify a client, so cookies are commonly used for
 * session management.
 * <p>
 * A cookie has a name, a single value, and optional attributes such as a
 * comment, path and domain qualifiers, a maximum age, and a version number.
 * Some Web browsers have bugs in how they handle the optional attributes, so
 * use them sparingly to improve the interoperability of your servlets.
 * <p>
 * The servlet sends cookies to the browser by using the
 * http_response#add_cookie method, which adds fields to HTTP
 * response headers to send cookies to the browser, one at a time. The browser
 * is expected to support 20 cookies for each Web server, 300 cookies total, and
 * may limit cookie size to 4 KB each.
 * <p>
 * The browser returns cookies to the servlet by adding fields to HTTP request
 * headers. Cookies can be retrieved from a request by using the
 * http_request#get_cookies method. Several cookies might have the
 * same name but different path attributes.
 * <p>
 * Cookies affect the caching of the Web pages that use them. HTTP 1.0 does not
 * cache pages that use cookies created with this class. This class does not
 * support the cache control defined with HTTP 1.1.
 * <p>
 * This class supports both the Version 0 (by Netscape) and Version 1 (by RFC
 * 2109) cookie specifications. By default, cookies are created using Version 0
 * to ensure the best interoperability.
 */
class cookie
{
public:
    /**
     * Constructs a cookie with a specified name and value.
     * <p>
     * The name must conform to RFC 2109. That means it can contain only ASCII
     * alphanumeric characters and cannot contain commas, semicolons, or white
     * space or begin with a $ character. The cookie's name cannot be changed
     * after creation.
     * <p>
     * The value can be anything the server chooses to send. Its value is
     * probably of interest only to the server. The cookie's value can be
     * changed after creation with the <code>setValue</code> method.
     * <p>
     * By default, cookies are created according to the Netscape cookie
     * specification. The version can be changed with the
     * <code>set_version</code> method.
     *
     * @param name a <code>std::string</code> specifying the name of the cookie
     * @param value a <code>std::string</code> specifying the value of the cookie
     * @see #set_value
     * @see #set_version
     */
    cookie(const std::string &name, const std::string &value) : _name{name}, _value{value} {}

    /**
     * Constructs a cookie with a specified name and value.
     *
     * <p>Same as #cookie(const std::string&, const std::string&), except the
     * arguments are moved.</p>
     *
     * @param name a <code>std::string</code> specifying the name of the cookie
     * @param value a <code>std::string</code> specifying the value of the cookie
     * @see #cookie(const std::string&, const std::string&)
     */
    cookie(std::string &&name, std::string &&value) : _name{std::move(name)}, _value{std::move(value)} {}

    /**
     * Constructs a cookie with a specified name and value.
     *
     * <p>Same as #cookie(const std::string&, const std::string&), except name
     * argument is moved.</p>
     *
     * @param name a <code>std::string</code> specifying the name of the cookie
     * @param value a <code>std::string</code> specifying the value of the cookie
     * @see #cookie(const std::string&, const std::string&)
     */
    cookie(std::string &&name, const std::string &value) : _name{std::move(name)}, _value{value} {}

    /**
     * Constructs a cookie with a specified name and value.
     *
     * <p>Same as #cookie(const std::string&, const std::string&), except value
     * argument is moved.</p>
     *
     * @param name a <code>std::string</code> specifying the name of the cookie
     * @param value a <code>std::string</code> specifying the value of the cookie
     * @see #cookie(const std::string&, const std::string&)
     */
    cookie(const std::string &name, std::string &&value) : _name{name}, _value{std::move(value)} {}

    /**
     * Copy constructor
     * @param c Object to copy from
     */
    cookie(const cookie& c)  = default;

    /**
     * Move constructor
     * @param c Object to move from
     */
    cookie(cookie&& c)  = default;

    /**
     * Copy assignment operator
     * @param c Object to copy from
     */
    cookie& operator=(const cookie& c) = default;

    /**
     * Move assignment operator
     * @param c Object to move from
     */
    cookie& operator=(cookie&& c) = default;

    /**
     * Destructor
     */
    ~cookie() noexcept = default;

    /**
     * Returns the name of the cookie. The name cannot be changed after
     * creation.
     *
     * @return a <code>std::string</code> specifying the cookie's name
     */
    const std::string &get_name() const { return _name; }

    /**
     * Returns the value of the cookie.
     *
     * @return a <code>std::string</code> containing the cookie's present value
     * @see #set_value
     * @see cookie
     */
    const std::string &get_value() const { return _value; }

    /**
     * Returns the comment describing the purpose of this cookie, or
     * empty string if the cookie has no comment.
     *
     * @return a <code>String</code> containing the comment, or
     *         <code>null</code> if none
     * @see #set_comment
     */
    const std::string &get_comment() const { return _comment; }

    /**
     * Returns the domain name set for this cookie. The form of the domain name
     * is set by RFC 2109.
     *
     * @return a <code>std::string</code> containing the domain name
     * @see #set_domain
     */
    const std::string &get_domain() const { return _domain; }

    /**
     * Returns the maximum age of the cookie, specified in seconds, By default,
     * <code>-1</code> indicating the cookie will persist until browser
     * shutdown.
     *
     * @return a <code>long</code> specifying the maximum age of the cookie in
     *         seconds; if negative, means the cookie persists until browser shutdown
     * @see #set_max_age
     */
    long get_max_age() const { return _max_age; }

    /**
     * Returns the path on the server to which the browser returns this cookie.
     * The cookie is visible to all subpaths on the server.
     *
     * @return a <code>std::string</code> specifying a path that contains a servlet
     *         name, for example, <i>/catalog</i>
     * @see #set_path
     */
    const std::string &get_path() const { return _path; }

    /**
     * Returns <code>true</code> if the browser is sending cookies only over a
     * secure protocol, or <code>false</code> if the browser can send cookies
     * using any protocol.
     *
     * @return <code>true</code> if the browser uses a secure protocol;
     *         otherwise, <code>true</code>
     * @see #set_secure
     */
    bool is_secure() const { return _secure; }

    /**
     * Returns the version of the protocol this cookie complies with. Version 1
     * complies with RFC 2109, and version 0 complies with the original cookie
     * specification drafted by Netscape. Cookies provided by a browser use and
     * identify the browser's cookie version.
     *
     * @return 0 if the cookie complies with the original Netscape specification;
     *         1 if the cookie complies with RFC 2109
     * @see #set_version
     */
    int get_version() const { return _version; }

    /**
     * Gets the flag that controls if this cookie will be hidden from scripts on
     * the client side.
     *
     * @return  <code>true</code> if the cookie is hidden from scripts, else
     *          <code>false</code>
     */
    bool is_http_only() const { return _http_only; }

    /**
     * Assigns a new value to a cookie after the cookie is created. If you use a
     * binary value, you may want to use BASE64 encoding.
     * <p>
     * With Version 0 cookies, values should not contain white space, brackets,
     * parentheses, equals signs, commas, double quotes, slashes, question
     * marks, at signs, colons, and semicolons. Empty values may not behave the
     * same way on all browsers.
     *
     * @param value a <code>std::string</code> specifying the new value
     * @see #get_value
     * @see cookie
     */
    void set_value(const std::string &value) { _value = value; }

    /**
     * Assigns a new value to a cookie after the cookie is created.
     *
     * <p>Same as #set_value(const std::string&) except the argument is
     * moved</p>
     *
     * @param value a <code>std::string</code> specifying the new value
     * @see #set_value(const std::string&)
     */
    void set_value(std::string &&value) { _value = std::move(value); }

    /**
     * Specifies a comment that describes a cookie's purpose. The comment is
     * useful if the browser presents the cookie to the user. Comments are not
     * supported by Netscape Version 0 cookies.
     *
     * @param comment a <code>std::string</code> specifying the comment to
     *                display to the user
     * @see #get_comment
     */
    void set_comment(const std::string &comment) { _comment = comment; }

    /**
     * Specifies a comment that describes a cookie's purpose.
     *
     * <p>Same as #set_comment(const std::string&) except the argument is
     * moved</p>
     *
     * @param comment a <code>std::string</code> specifying the comment to
     *                display to the user
     * @see #set_comment(const std::string&)
     */
    void set_comment(std::string &&comment) { _comment = std::move(comment); }

    /**
     * Specifies the domain within which this cookie should be presented.
     * <p>
     * The form of the domain name is specified by RFC 2109. A domain name
     * begins with a dot (<code>.foo.com</code>) and means that the cookie is
     * visible to servers in a specified Domain Name System (DNS) zone (for
     * example, <code>www.foo.com</code>, but not <code>a.b.foo.com</code>). By
     * default, cookies are only returned to the server that sent them.
     *
     * @param domain a <code>std::string</code> containing the domain name within
     *               which this cookie is visible; form is according to RFC 2109
     * @see #get_domain
     */
    void set_domain(const std::string &domain) { _domain = domain; }

    /**
     * Specifies the domain within which this cookie should be presented.
     *
     * <p>Same as #set_domain(const std::string&) except the argument is
     * moved</p>
     *
     * @param domain a <code>std::string</code> containing the domain name within
     *               which this cookie is visible; form is according to RFC 2109
     * @see #set_domain(const std::string&)
     */
    void set_domain(std::string &&domain) { _domain = std::move(domain); }

    /**
     * Sets the maximum age of the cookie in seconds.
     * <p>
     * A positive value indicates that the cookie will expire after that many
     * seconds have passed. Note that the value is the <i>maximum</i> age when
     * the cookie will expire, not the cookie's current age.
     * <p>
     * A negative value means that the cookie is not stored persistently and
     * will be deleted when the Web browser exits. A zero value causes the
     * cookie to be deleted.
     *
     * @param max_age an integer specifying the maximum age of the cookie in
     *                seconds; if negative, means the cookie is not stored;
     *                if zero, deletes the cookie
     * @see #get_max_age
     */
    void set_max_age(long max_age) { _max_age = max_age; }

    /**
     * Specifies a path for the cookie to which the client should return the
     * cookie.
     * <p>
     * The cookie is visible to all the pages in the directory you specify, and
     * all the pages in that directory's subdirectories. A cookie's path must
     * include the servlet that set the cookie, for example, <i>/catalog</i>,
     * which makes the cookie visible to all directories on the server under
     * <i>/catalog</i>.
     * <p>
     * Consult RFC 2109 (available on the Internet) for more information on
     * setting path names for cookies.
     *
     * @param path a <code>std::string</code> specifying a path
     * @see #get_path
     */
    void set_path(const std::string &path) { _path = path; }

    /**
     * Specifies a path for the cookie to which the client should return the
     * cookie.
     *
     * <p>Same as #set_path(const std::string&) except the argument is
     * moved</p>
     *
     * @param path a <code>std::string</code> specifying a path
     * @see #set_path(const std::string&)
     */
    void set_path(std::string &&path) { _path = std::move(path); }

    /**
     * Indicates to the browser whether the cookie should only be sent using a
     * secure protocol, such as HTTPS or SSL.
     * <p>
     * The default value is <code>false</code>.
     *
     * @param secure if <code>true</code>, sends the cookie from the browser
     *               to the server only when using a secure protocol; if
     *               <code>false</code>, sent on any protocol
     * @see #is_secure
     */
    void set_secure(bool secure) { _secure = secure; }

    /**
     * Sets the version of the cookie protocol this cookie complies with.
     * Version 0 complies with the original Netscape cookie specification.
     * Version 1 complies with RFC 2109.
     * <p>
     * Since RFC 2109 is still somewhat new, consider version 1 as experimental;
     * do not use it yet on production sites.
     *
     * @param version 0 if the cookie should comply with the original Netscape
     *                specification; 1 if the cookie should comply with RFC 2109
     * @see #get_version
     */
    void set_version(int version) { _version = version; }

    /**
     * Sets the flag that controls if this cookie will be hidden from scripts on
     * the client side.
     *
     * @param http_only The new value of the flag
     */
    void set_http_only(bool http_only) { _http_only = http_only; }

    /**
     * Converts this cookie into its string representation.
     *
     * @return a <code>std::string</code> representation of this cookie.
     */
    std::string to_string() const;

private:
    std::string _name;       /* NAME= ... "$Name" style is reserved */
    std::string _value;      /* value of NAME */

    /* Attributes encoded in the header's cookie fields. */
    std::string _comment;    /* ;Comment=VALUE ... describes cookie's use */
    /* ;Discard ... implied by maxAge < 0 */
    std::string _domain;     /* ;Domain=VALUE ... domain that sees cookie */
    long _max_age = -1;      /* ;Max-Age=VALUE ... cookies auto-expire */
    std::string _path;       /* ;Path=VALUE ... URLs that see the cookie */
    bool _secure = false;    /* ;Secure ... e.g. use SSL */
    int _version = 0;        /* ;Version=1 ... means RFC 2109++ style */
    bool _http_only = false; /* Not in cookie specs, but supported by browsers */
};

} // end of servlet namespace

#endif // MOD_SERVLET_COOKIE_H
