#ifndef MOD_SERVLET_URI_NEW_H
#define MOD_SERVLET_URI_NEW_H

/**
 * @file uri.h Contains definition of URI class and related structures and operations.
 * @brief Definition of URI class and related functions
 */

#include <string>
#include <iostream>
#include <iterator>
#include <vector>
#include <functional>
#include <experimental/string_view>

#include <servlet/lib/exception.h>

namespace servlet
{

using std::experimental::string_view;

/**
 * Exception thrown while parsing URI if syntax error encountered.
 */
struct uri_syntax_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Exception thrown when it is impossible to build URI from parts.
 */
struct uri_builder_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/**
 * Represents a Uniform Resource Identifier (URI) reference.
 *
 * <p>This class conforms to a URI as defined by RFC 3986, RFC 3987 and
 * RFC 2732, including scoped IDs. It provides member functions for
 * normalizing, comparing and resolving URIs.</p>
 *
 * <p>This class provides constructors for creating URI instances from
 * their components or by parsing their string forms, methods for accessing the
 * various components of an instance, and methods for normalizing, resolving,
 * and relativizing URI instances. Instances of this class are mutable.</p>
 *
 * <h4> URI syntax and components </h4>
 *
 * At the highest level a URI reference (hereinafter simply "URI") in string
 * form has the syntax
 *
 * <blockquote>
 * [<i>scheme</i><tt><b>:</b></tt>]<i>scheme-specific-part</i>[<tt><b>#</b></tt><i>fragment</i>]
 * </blockquote>
 *
 * where square brackets [...] delineate optional components and the characters
 * <tt><b>:</b></tt> and <tt><b>#</b></tt> stand for themselves.
 *
 * <p> An <i>absolute</i> URI specifies a scheme; a URI that is not absolute is
 * said to be <i>relative</i>.  URIs are also classified according to whether
 * they are <i>opaque</i> or <i>hierarchical</i>.
 *
 * <p> An <i>opaque</i> URI is an absolute URI whose scheme-specific part does
 * not begin with a slash character (<tt>'/'</tt>).  Opaque URIs are not
 * subject to further parsing.  Some examples of opaque URIs are:
 *
 * <blockquote><table cellpadding=0 cellspacing=0 summary="layout">
 * <tr><td><tt>mailto:java-net@java.sun.com</tt><td></tr>
 * <tr><td><tt>news:comp.lang.java</tt><td></tr>
 * </table></blockquote>
 *
 * <p> A <i>hierarchical</i> URI is either an absolute URI whose
 * scheme-specific part begins with a slash character, or a relative URI, that
 * is, a URI that does not specify a scheme.  Some examples of hierarchical
 * URIs are:
 *
 * <blockquote>
 * <tt>http://java.sun.com/j2se/1.3/</tt><br>
 * <tt>docs/guide/collections/designfaq.html#28</tt><br>
 * <tt>../../../demo/jfc/SwingSet2/src/SwingSet2.java</tt><br>
 * <tt>file:///~/calendar</tt>
 * </blockquote>
 *
 * <p> A hierarchical URI is subject to further parsing according to the syntax
 *
 * <blockquote>
 * [<i>scheme</i><tt><b>:</b></tt>][<tt><b>//</b></tt><i>authority</i>][<i>path</i>][<tt><b>?</b></tt><i>query</i>][<tt><b>#</b></tt><i>fragment</i>]
 * </blockquote>
 *
 * where the characters <tt><b>:</b></tt>, <tt><b>/</b></tt>,
 * <tt><b>?</b></tt>, and <tt><b>#</b></tt> stand for themselves.  The
 * scheme-specific part of a hierarchical URI consists of the characters
 * between the scheme and fragment components.
 *
 * <p> The authority component of a hierarchical URI is, if specified, either
 * <i>server-based</i> or <i>registry-based</i>.  A server-based authority
 * parses according to the familiar syntax
 *
 * <blockquote>
 * [<i>user-info</i><tt><b>\@</b></tt>]<i>host</i>[<tt><b>:</b></tt><i>port</i>]
 * </blockquote>
 *
 * where the characters <tt><b>\@</b></tt> and <tt><b>:</b></tt> stand for
 * themselves.  Nearly all URI schemes currently in use are server-based.  An
 * authority component that does not parse in this way is considered to be
 * registry-based.
 *
 * <p> The path component of a hierarchical URI is itself said to be absolute
 * if it begins with a slash character (<tt>'/'</tt>); otherwise it is
 * relative.  The path of a hierarchical URI that is either absolute or
 * specifies an authority is always absolute.
 *
 * <p> All told, then, a URI instance has the following nine components:
 *
 * <blockquote><table summary="Describes the components of a URI:scheme,scheme-specific-part,authority,user-info,host,port,path,query,fragment">
 * <tr><th><i>Component</i></th><th><i>Type</i></th></tr>
 * <tr><td>scheme</td><td><tt>string_view</tt></td></tr>
 * <tr><td>scheme-specific-part&nbsp;&nbsp;&nbsp;&nbsp;</td><td><tt>string_view</tt></td></tr>
 * <tr><td>authority</td><td><tt>string_view</tt></td></tr>
 * <tr><td>user-info</td><td><tt>string_view</tt></td></tr>
 * <tr><td>host</td><td><tt>string_view</tt></td></tr>
 * <tr><td>port</td><td><tt>uint16_t</tt></td></tr>
 * <tr><td>path</td><td><tt>string_view</tt></td></tr>
 * <tr><td>query</td><td><tt>string_view</tt></td></tr>
 * <tr><td>fragment</td><td><tt>string_view</tt></td></tr>
 * </table></blockquote>
 *
 * In a given instance any particular component is either <i>undefined</i> or
 * <i>defined</i> with a distinct value.  Undefined string components are
 * represented by <tt>empty view</tt>, while undefined integer components are
 * represented by <tt>0</tt>.
 *
 * <p> Whether a particular component is or is not defined in an instance
 * depends upon the type of the URI being represented.  An absolute URI has a
 * scheme component.  An opaque URI has a scheme, a scheme-specific part, and
 * possibly a fragment, but has no other components.  A hierarchical URI always
 * has a path (though it may be empty) and a scheme-specific-part (which at
 * least contains the path), and may have any of the other components.  If the
 * authority component is present and is server-based then the host component
 * will be defined and the user-information and port components may be defined.
 *
 *
 * <h4> Operations on URI instances </h4>
 *
 * The key operations supported by this class are those of
 * <i>normalization</i>, <i>resolution</i>, and <i>relativization</i>.
 *
 * <p> <i>Normalization</i> is the process of removing unnecessary <tt>"."</tt>
 * and <tt>".."</tt> segments from the path component of a hierarchical URI.
 * Each <tt>"."</tt> segment is simply removed.  A <tt>".."</tt> segment is
 * removed only if it is preceded by a non-<tt>".."</tt> segment.
 * Normalization has no effect upon opaque URIs.
 *
 * <p> <i>Resolution</i> is the process of resolving one URI against another,
 * <i>base</i> URI.  The resulting URI is constructed from components of both
 * URIs in the manner specified by RFC&nbsp;2396, taking components from the
 * base URI for those not specified in the original.  For hierarchical URIs,
 * the path of the original is resolved against the path of the base and then
 * normalized.  The result, for example, of resolving
 *
 * <blockquote>
 * <tt>docs/guide/collections/designfaq.html#28&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</tt>(1)
 * </blockquote>
 *
 * against the base URI <tt>http://java.sun.com/j2se/1.3/</tt> is the result
 * URI
 *
 * <blockquote>
 * <tt>http://java.sun.com/j2se/1.3/docs/guide/collections/designfaq.html#28</tt>
 * </blockquote>
 *
 * Resolving the relative URI
 *
 * <blockquote>
 * <tt>../../../demo/jfc/SwingSet2/src/SwingSet2.java&nbsp;&nbsp;&nbsp;&nbsp;</tt>(2)
 * </blockquote>
 *
 * against this result yields, in turn,
 *
 * <blockquote>
 * <tt>http://java.sun.com/j2se/1.3/demo/jfc/SwingSet2/src/SwingSet2.java</tt>
 * </blockquote>
 *
 * Resolution of both absolute and relative URIs, and of both absolute and
 * relative paths in the case of hierarchical URIs, is supported.  Resolving
 * the URI <tt>file:///~calendar</tt> against any other URI simply yields the
 * original URI, since it is absolute.  Resolving the relative URI (2) above
 * against the relative base URI (1) yields the normalized, but still relative,
 * URI
 *
 * <blockquote>
 * <tt>demo/jfc/SwingSet2/src/SwingSet2.java</tt>
 * </blockquote>
 *
 * <p> <i>Relativization</i>, finally, is the inverse of resolution: For any
 * two normalized URIs <i>u</i> and&nbsp;<i>v</i>,
 *
 * <blockquote>
 *   <i>u</i><tt>.relativize(</tt><i>u</i><tt>.resolve(</tt><i>v</i><tt>)) == </tt><i>v</i><tt></tt>&nbsp;&nbsp;and<br>
 *   <i>u</i><tt>.resolve(</tt><i>u</i><tt>.relativize(</tt><i>v</i><tt>)) == </tt><i>v</i><tt></tt>&nbsp;&nbsp;.<br>
 * </blockquote>
 *
 * This operation is often useful when constructing a document containing URIs
 * that must be made relative to the base URI of the document wherever
 * possible.  For example, relativizing the URI
 *
 * <blockquote>
 * <tt>http://java.sun.com/j2se/1.3/docs/guide/index.html</tt>
 * </blockquote>
 *
 * against the base URI
 *
 * <blockquote>
 * <tt>http://java.sun.com/j2se/1.3</tt>
 * </blockquote>
 *
 * yields the relative URI <tt>docs/guide/index.html</tt>.
 *
 * <h4> URIs, URLs, and URNs </h4>
 *
 * A URI is a uniform resource <i>identifier</i> while a URL is a uniform
 * resource <i>locator</i>.  Hence every URL is a URI, abstractly speaking, but
 * not every URI is a URL.  This is because there is another subcategory of
 * URIs, uniform resource <i>names</i> (URNs), which name resources but do not
 * specify how to locate them.  The <tt>mailto</tt>, <tt>news</tt>, and
 * <tt>isbn</tt> URIs shown above are examples of URNs.
 *
 * <p> An instance of this class represents a URI reference in the syntactic
 * sense defined by RFC&nbsp;2396.  A URI may be either absolute or relative.
 * A URI string is parsed according to the generic syntax without regard to the
 * scheme, if any, that it specifies.  No lookup of the host, if any, is
 * performed, and no scheme-dependent stream handler is constructed.  Equality,
 * hashing, and comparison are defined strictly in terms of the character
 * content of the instance.  In other words, a URI instance is little more than
 * a structured string that supports the syntactic, scheme-independent
 * operations of comparison, normalization, resolution, and relativization.
 *
 *
 * @see <a href="http://ietf.org/rfc/rfc2279.txt"><i>RFC&nbsp;2279: UTF-8, a
 * transformation format of ISO 10646</i></a>,
 * <br><a href="http://www.ietf.org/rfc/rfc2373.txt"><i>RFC&nbsp;2373: IPv6
 * Addressing Architecture</i></a>, <br>
 * <a href="http://www.ietf.org/rfc/rfc2396.txt"><i>RFC&nbsp;2396: Uniform
 * Resource Identifiers (URI): Generic Syntax</i></a>, <br>
 * <a href="http://www.ietf.org/rfc/rfc2732.txt"><i>RFC&nbsp;2732: Format for
 * Literal IPv6 Addresses in URLs</i></a>
 */
class URI
{
public:
    /**
     * The URI string_type.
     */
    typedef std::string string_type;

    /**
     * The URI string_view.
     */
    typedef std::experimental::string_view string_view;

    /**
     * The URI const_iterator type.
     */
    typedef string_view::const_iterator const_iterator;

    /**
     * Default constructor. Creates empty URI.
     */
    URI() : _uri{}, _uri_view{_uri}, _scheme{_uri}, _user_info{_uri}, _host{_uri},
            _port{_uri}, _path{_uri}, _query{_uri}, _fragment{_uri} {}

    /**
     * \fn URI(const string_type&)
     * Constructs a URI by parsing the given string.
     *
     * <p>This constructor parses the given string as specified by the
     * grammar in <a
     * href="http://www.ietf.org/rfc/rfc2396.txt">RFC&nbsp;2396</a>,
     * Appendix&nbsp;A, <b><i>except for the following deviations:</i></b> </p>
     *
     * <ul type=disc>
     *
     *   <li><p> An empty authority component is permitted as long as it is
     *   followed by a non-empty path, a query component, or a fragment
     *   component.  This allows the parsing of URIs such as
     *   <tt>"file:///foo/bar"</tt>, which seems to be the intent of
     *   RFC&nbsp;2396 although the grammar does not permit it.  If the
     *   authority component is empty then the user-information, host, and port
     *   components are undefined. </p></li>
     *
     *   <li><p> Empty relative paths are permitted; this seems to be the
     *   intent of RFC&nbsp;2396 although the grammar does not permit it.  The
     *   primary consequence of this deviation is that a standalone fragment
     *   such as <tt>"#foo"</tt> parses as a relative URI with an empty path
     *   and the given fragment, and can be usefully <a
     *   href="#resolve-frag">resolved</a> against a base URI.
     *
     *   <li><p> IPv4 addresses are not validated. </p></li>
     *
     *   <li><p> IPv6 addresses are permitted for the host component.  An IPv6
     *   address must be enclosed in square brackets (<tt>'['</tt> and
     *   <tt>']'</tt>) as specified by <a
     *   href="http://www.ietf.org/rfc/rfc2732.txt">RFC&nbsp;2732</a>. Other
     *   than this constraint IPv6 addresses are not validated. </p></li>
     *
     *   <li><p> Characters in the <i>other</i> category are permitted wherever
     *   RFC&nbsp;2396 permits <i>escaped</i> octets, that is, in the
     *   user-information, path, query, and fragment components, as well as in
     *   the authority component if the authority is registry-based.  This
     *   allows URIs to contain Unicode characters beyond those in the US-ASCII
     *   character set. </p></li>
     *
     * </ul>
     *
     * @param  uri_str The string to be parsed into a URI
     *
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     */
    explicit URI(const string_type& uri_str) : _uri{uri_str} { _initialize(); }

    /**
     * Constructs a URI by parsing the given string.
     *
     * <p>URI string is parsed as described in URI(const string_type&)</p>
     *
     * @param  uri_str The string to be parsed into a URI
     *
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    explicit URI(string_type&& uri_str) : _uri{std::move(uri_str)} { _initialize(); }

    /**
     * Constructs a URI by parsing the given string.
     *
     * <p>URI string is parsed as described in URI(const string_type&)</p>
     *
     * @param  uri_str The string to be parsed into a URI
     *
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    explicit URI(string_view uri_str) : _uri{uri_str.to_string()} { _initialize(); }

    /**
     * Constructs a URI by parsing the given string.
     *
     * <p>URI string is parsed as described in URI(const string_type&)</p>
     *
     * @param  uri_str The string to be parsed into a URI
     *
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    explicit URI(const char* uri_str) : _uri{uri_str} { _initialize(); }

    /**
     * Constructs a URI by parsing the given string.
     *
     * <p>URI string is constructed from first, last iterators and the
     * string is parsed further as described in URI(const string_type&)</p>
     *
     * @tparam InputIter input iterator type from which URI string will be created
     * @param  first Begin iterator to URI string
     * @param  last  End iterator to URI string
     *
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    template <class InputIter>
    URI(InputIter first, InputIter last) : _uri{string_type(first, last)} { _initialize(); }

    /**
     * Constructs a hierarchical URI from the given components.
     *
     * <p> If a scheme is given then the path, if also given, must either be
     * empty or begin with a slash character (<tt>'/'</tt>). If the path begins
     * not with slash, than slash will be prepended to it to facilitate relative
     * paths. Otherwise initial component of the new URI may be left undefined
     * by passing empty view for the corresponding parameter or, in the case of
     * the <tt>port</tt> parameter, by passing <tt>0</tt> or negative value.
     *
     * <p> This constructor first builds a URI string from the given components
     * according to the rules specified in <a
     * href="http://www.ietf.org/rfc/rfc2396.txt">RFC&nbsp;2396</a>,
     * section&nbsp;5.2, step&nbsp;7: </p>
     *
     * <ol>
     *
     *   <li><p> Initially, the result string is empty. </p></li>
     *
     *   <li><p> If a scheme is given then it is appended to the result,
     *   followed by a colon character (<tt>':'</tt>).  </p></li>
     *
     *   <li><p> If user information, a host, or a port are given then the
     *   string <tt>"//"</tt> is appended.  </p></li>
     *
     *   <li><p> If user information is given then it is appended, followed by
     *   a commercial-at character (<tt>'@'</tt>).  </p></li>
     *
     *   <li><p> If a host is given then it is appended. </p></li>
     *
     *   <li><p> If a port number is given then a colon character
     *   (<tt>':'</tt>) is appended, followed by the port number in decimal.
     *   </p></li>
     *
     *   <li><p> If a path is given then it is appended. </p></li>
     *
     *   <li><p> If a query is given then a question-mark character
     *   (<tt>'?'</tt>) is appended, followed by the query. </p></li>
     *
     *   <li><p> Finally, if a fragment is given then a hash character
     *   (<tt>'#'</tt>) is appended, followed by the fragment.   </p></li>
     *
     * </ol>
     *
     * <p> No further parsing for the resulting URI string is attempted. </p>
     *
     * @param   scheme    Scheme name
     * @param   userInfo  User name and authorization information
     * @param   host      Host name
     * @param   port      Port number
     * @param   path      Path
     * @param   query     Query
     * @param   fragment  Fragment
     */
    URI(string_view scheme, string_view userInfo, string_view host, uint16_t port,
        string_view path, string_view query, string_view fragment);

    /**
     * Constructs a hierarchical URI from the given components.
     *
     * <p> This constructor follows the same rules as
     * #URI(string_view scheme, string_view userInfo, string_view host, uint16_t port, string_view path, string_view query, string_view fragment)
     * but with empty userInfo and fragment.
     *
     * @param   scheme    Scheme name
     * @param   host      Host name
     * @param   port      Port number
     * @param   path      Path
     * @param   query     Query
     *
     * @see #URI(string_view scheme, string_view userInfo, string_view host, uint16_t port, string_view path, string_view query, string_view fragment)
     */
    URI(string_view scheme, string_view host, uint16_t port, string_view path, string_view query) :
            URI{scheme, "", host, port, path, query, ""} {}

    /**
     * Copy constructor.
     * @param   other    URI to copy from
     */
    URI(const URI &other) : _uri{other._uri}, _uri_view{_uri}, _normalized{other._normalized} { _move_parts(other); }

    /**
     * Move constructor.
     * @param   other    URI to move from
     */
    URI(URI &&other) noexcept : _uri{std::move(other._uri)}, _uri_view{_uri} { _move_parts(other); }

    /**
     * Destructor.
     */
    ~URI() noexcept {}

    /**
     * Copy assignment operator.
     * @param   other    URI to copy from
     */
    URI &operator=(const URI& other);

    /**
     * Move assignment operator.
     * @param   other    URI to move from
     */
    URI &operator=(URI&& other);

    /**
     * String assignment operator.
     *
     * <p>Assigns the string representation of URI to this URI object. The
     * string is parsed following the rules to string constructor
     * URI(const string_type&)</p>
     *
     * @param uri_str The string to be parsed into a URI
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    URI &operator=(const string_type& uri_str) { _uri = uri_str; _initialize(); return *this; }

    /**
     * Move string assignment operator.
     *
     * <p>Assigns the string representation of URI to this URI object. The
     * string is parsed following the rules to string constructor
     * URI(const string_type&)</p>
     *
     * @param uri_str The string to be parsed into a URI
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    URI &operator=(string_type&& uri_str) { _uri = std::move(uri_str); _initialize(); return *this; }

    /**
     * String view assignment operator.
     *
     * <p>Assigns the string representation of URI to this URI object. The
     * string is parsed following the rules to string constructor
     * URI(const string_type&)</p>
     *
     * @param uri_str The string to be parsed into a URI
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    URI &operator=(string_view uri_str) { _uri = uri_str.to_string(), _initialize(); return *this; }

    /**
     * C-string assignment operator.
     *
     * <p>Assigns the string representation of URI to this URI object. The
     * string is parsed following the rules to string constructor
     * URI(const string_type&)</p>
     *
     * @param uri_str The string to be parsed into a URI
     * @throws  uri_syntax_error
     *          If the given string violates RFC&nbsp;2396, as augmented
     *          by the above deviations
     * @see URI(const string_type&)
     */
    URI &operator=(const char* uri_str) { _uri = uri_str; _initialize(); return *this; }

    /**
     * Swaps one uri object with another.
     * @param other The other uri object.
     */
    void swap(URI &other) noexcept;

    /**
     * Returns an iterator at the first element in the underlying URI view.
     * @return A begin iterator of underlying string view of the URI.
     */
    const_iterator begin() const noexcept { return _uri_view.begin(); }

    /**
     * Returns an iterator at the last+1 element in the underlying URI view.
     * @return A end iterator of underlying string view of the URI.
     */
    const_iterator end() const noexcept { return _uri_view.end(); }

    /**
     * Returns the scheme component of this URI.
     * @return  The scheme component of this URI, or empty view
     *          if the scheme is undefined
     */
    string_view scheme() const noexcept { return _scheme; }

    /**
     * Sets scheme component to this URI.
     *
     * <p>New scheme component replaces the old one. If new scheme
     * component is empty, the current scheme component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param scheme New scheme component to this URI
     */
    void set_scheme(string_view scheme);

    /**
     * Returns the userInfo component of this URI.
     * @return  The userInfo component of this URI, or empty view
     *          if the userInfo is undefined
     */
    string_view user_info() const noexcept { return _user_info; }

    /**
     * Sets user_info component to this URI.
     *
     * <p>New user_info component replaces the old one. If new user_info
     * component is empty, the current user_info component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param user_info New user_info component to this URI
     */
    void set_user_info(string_view user_info);

    /**
     * Returns the host component of this URI.
     * @return  The host component of this URI, or empty view
     *          if the host is undefined
     */
    string_view host() const noexcept { return _host; }

    /**
     * Sets host component to this URI.
     *
     * <p>New host component replaces the old one. If new host
     * component is empty, the current host component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param host New host component to this URI
     */
    void set_host(string_view host);

    /**
     * Returns the port view component of this URI.
     * @return  The port view component of this URI, or empty view
     *          if the port is undefined
     */
    string_view port_view() const noexcept { return _port; }

    /**
     * Returns the port component of this URI.
     * @return  The port component of this URI, or <tt>0</tt>
     *          if the port is undefined
     */
    uint16_t port() const noexcept { return _port_i; }

    /**
     * Sets port component to this URI.
     *
     * <p>New port component replaces the old one. If new port
     * component is empty, the current port component will be deleted
     * and considered to be undefined</p>
     *
     * <p>This method validates port, by reading uint16_t representation.
     * No forther parsing is performed in this method.</p>
     *
     * @param port New port component to this URI
     */
    void set_port(string_view port);

    /**
     * Sets port component to this URI.
     *
     * <p>New port component replaces the old one. If new port
     * component is <tt>0</tt> or negative, the current port component
     * will be deleted and considered to be undefined</p>
     *
     * <p>No forther parsing is performed in this method.</p>
     *
     * @param port New port component to this URI
     */
    void set_port(uint16_t port);

    /**
     * Returns the path view component of this URI.
     * @return  The path view component of this URI, or empty view
     *          if the path is undefined
     */
    string_view path() const noexcept { return _path; }

    /**
     * Sets path component to this URI.
     *
     * <p>New path component replaces the old one. If new path
     * component is empty, the current path component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param path New path component to this URI
     * @throws  uri_syntax_error
     *          If the given path component is not empty and does not
     *          begin with slash character <tt>"/"</tt>
     */
    void set_path(string_view path);

    /**
     * Returns the query view component of this URI.
     * @return  The query view component of this URI, or empty view
     *          if the query is undefined
     */
    string_view query() const noexcept { return _query; }

    /**
     * Sets query component to this URI.
     *
     * <p>New query component replaces the old one. If new query
     * component is empty, the current path component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param query New query component to this URI
     */
    void set_query(string_view query);

    /**
     * Adds name-value pair to the current query component of this URI.
     *
     * <p>If query component already exists, this method will append
     * <code>"&name=value"</code> string, otherwise new query component
     * will be created as <code>"name=value"</code></p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param name Name of new name-value pair of query component of this URI
     * @param value Value of new name-value pair of query component of this URI
     */
    void add_to_query(string_view name, string_view value);

    /**
     * Parses this URI's query into name-value pairs and feeds them to a given consumer.
     *
     * This function is equivalent of calling:
     *
     * ~~~~~{.cpp}
     * URI::parse_uri(query(), consumer);
     * ~~~~~
     * @param consumer <code>std::function</code> to feed name-value pairs to.
     * @see parse_query(string_view , std::function<void(const std::string&, const std::string&)>)
     */
    void parse_query(std::function<void(const std::string&, const std::string&)> consumer)
    { parse_query(_query, consumer); }

    /**
     * Parses a given query string into name-value pairs and feeds them to a given consumer.
     *
     *
     * Before adding the name-value pairs to the consumer the are decodes from
     * all the persent encoded characters.
     *
     * The consumer here is used for genericity not to restrict type of map object
     * passed to the method. For example:
     *
     * ~~~~~{.cpp}
     * custom_map name_value_map;
     * URI::parse_query(query, [&name_value_map] (const std::string& name, const std::string& value)
     *     {
     *         name_value_map.emplace(std::move(name), std::move(value));
     *     });
     * ~~~~~
     * @param query query string view
     * @param consumer <code>std::function</code> to feed name-value pairs to.
     * @see parse_query(std::function<void(const std::string&, const std::string&)>)
     */
    static void parse_query(string_view query, std::function<void(const std::string&, const std::string&)> consumer);

    /**
     * Returns the fragment view component of this URI.
     * @return  The fragment view component of this URI, or empty view
     *          if the fragment is undefined
     */
    string_view fragment() const noexcept { return _fragment; }

    /**
     * Sets fragment component to this URI.
     *
     * <p>New fragment component replaces the old one. If new fragment
     * component is empty, the current fragment component will be deleted
     * and considered to be undefined</p>
     *
     * <p>No parsing or validation is performed in this method.</p>
     *
     * @param fragment New fragment component to this URI
     */
    void set_fragment(string_view fragment);

    /**
     * Tests whether this URI has a valid authority.
     * @return <code>true</code> if the URI has an authority,
     *         <code>false</code> otherwise.
     */
    bool has_authority() const noexcept;

    /**
     * Returns the raw authority component of this URI.
     *
     * <p> The authority component of a URI, if defined, only contains the
     * commercial-at character (<tt>'@'</tt>) and characters in the
     * <i>unreserved</i>, <i>punct</i>, <i>escaped</i>, and <i>other</i>
     * categories.  If the authority is server-based then it is further
     * constrained to have valid user-information, host, and port
     * components. </p>
     *
     * @return  The raw authority component of this URI,
     *          or <tt>null</tt> if the authority is undefined
     */
    string_view authority() const noexcept;

    /**
     * Returns the string view representation of this URI.
     * @return  The string view representation of this URI, or empty view
     *          if the URI doesn't have any components undefined
     */
    string_view uri_view() const noexcept { return _uri_view; }

    /**
     * Returns the URI as a std::string object.
     * @returns A URI string.
     */
    const std::string& string() const noexcept { return _uri; }

    /**
     * Returns the URI as a std::string&& object.
     *
     * <p>This  method can be used to move the string representation
     * out of this URI if this object is not intended to be used further</p>
     * @returns A moved URI string.
     */
    std::string&& string_move() { return std::move(_uri); }

    /**
     * Returns the content of this URI as a US-ASCII string.
     *
     * <p> If this URI does not contain any characters in the <i>other</i>
     * category then an invocation of this method will return the same value as
     * an invocation of the #string method.  Otherwise
     * this method works as if by invoking that method and then <a
     * href="#encode">encoding</a> the result.</p>
     *
     * @return  The string form of this URI, encoded as needed so that
     *          it only contains characters in the US-ASCII charset
     */
    std::string to_ASCII_string() const;

    /**
     * Tells whether or not this URI is empty.
     *
     * <p> A URI is absolute if, and only if, it has no components. </p>
     *
     * @return  <tt>true</tt> if, and only if, this URI has no components defined
     */
    bool empty() const noexcept { return _uri.empty(); }

    /**
     * Tells whether or not this URI is absolute.
     *
     * <p> A URI is absolute if, and only if, it has a scheme component. </p>
     *
     * @return  <tt>true</tt> if, and only if, this URI is absolute
     */
    bool is_absolute() const noexcept { return !_scheme.empty(); }

    /**
     * Tells whether or not this URI is opaque.
     *
     * <p> A URI is opaque if, and only if, it is absolute and its
     * scheme-specific part does not begin with a slash character ('/').
     * An opaque URI has a scheme, a scheme-specific part, and possibly
     * a fragment; all other components are undefined. </p>
     *
     * @return  <tt>true</tt> if, and only if, this URI is opaque
     */
    bool is_opaque() const noexcept; // { return (is_absolute() && !has_authority()); }

    /**
     * Normalizes this URI's path.
     *
     * <p>The path is normalized in a manner consistent with <a
     * href="http://www.ietf.org/rfc/rfc2396.txt">RFC&nbsp;2396</a>,
     * section&nbsp;5.2, step&nbsp;6, sub-steps&nbsp;c through&nbsp;f; that is:
     * </p>
     *
     * <ol>
     *
     *   <li><p> All <tt>"."</tt> segments are removed. </p></li>
     *
     *   <li><p> If a <tt>".."</tt> segment is preceded by a non-<tt>".."</tt>
     *   segment then both of these segments are removed.  This step is
     *   repeated until it is no longer applicable. </p></li>
     *
     * </ol>
     *
     * <p> A normalized path will begin with one or more <tt>".."</tt> segments
     * if there were insufficient non-<tt>".."</tt> segments preceding them to
     * allow their removal. Otherwise, a normalized path will not contain any
     * <tt>"."</tt> or <tt>".."</tt> segments. </p>
     */
    void normalize_path();

    /**
     * Normalizes this URI.
     *
     * <p>The normalization includes:</p>
     *
     * <ol>
     *
     *   <li><p>All alphabetic characters in the scheme component
     *   are converted to lower-case</p></li>
     *
     *   <li><p>All persent encoded characters are decoded</p></li>
     *
     *   <li><p>Path component is normalized as described in
     *   #normalize_path</p></li>
     *
     * </ol>
     *
     * @see #normalize_path
     */
    void normalize();

    /**
     * Create normalized copy of this URI.
     *
     * <p>Copy of this URI is created and #normalize method is called on
     * the copy</p>
     *
     * @return Normalized URI instance
     * @see #normalize
     */
    URI create_normalized() const;

    /**
     * Create copy of this URI with normalized path.
     *
     * <p>Copy of this URI is created and #normalize_path method is called
     * on the copy</p>
     *
     * @return Normalized URI instance
     * @see #normalize_path
     */
    URI create_with_normalized_path() const;

    /**
     * Relativizes the given URI against this URI.
     *
     * <p> The relativization of the given URI against this URI is computed as
     * follows: </p>
     *
     * <ol>
     *
     *   <li><p> If either this URI or the given URI are opaque, or if the
     *   scheme and authority components of the two URIs are not identical, or
     *   if the path of this URI is not a prefix of the path of the given URI,
     *   then the given URI is returned. </p></li>
     *
     *   <li><p> Otherwise a new relative hierarchical URI is constructed with
     *   query and fragment components taken from the given URI and with a path
     *   component computed by removing this URI's path from the beginning of
     *   the given URI's path. </p></li>
     *
     * </ol>
     *
     * @param  uri  The URI to be relativized against this URI
     * @return The resulting URI
     */
    URI relativize(const URI &uri) const;

    /**
     * Resolves the given URI against this URI.
     *
     * <p> If the given URI is already absolute, or if this URI is opaque, then
     * the given URI is returned.
     *
     * <p><If the given URI's fragment component is defined, its path component
     * is empty, and its scheme, authority, and query components are undefined,
     * then a URI with the given fragment but with all other components equal
     * to those of this URI is returned. This allows a URI representing a
     * standalone fragment reference, such as <tt>"#foo"</tt>, to be usefully
     * resolved against a base URI.
     *
     * <p> Otherwise this method constructs a new hierarchical URI in a manner
     * consistent with <a
     * href="http://www.ietf.org/rfc/rfc2396.txt">RFC&nbsp;2396</a>,
     * section&nbsp;5.2; that is: </p>
     *
     * <ol>
     *
     *   <li><p> A new URI is constructed with this URI's scheme and the given
     *   URI's query and fragment components. </p></li>
     *
     *   <li><p> If the given URI has an authority component then the new URI's
     *   authority and path are taken from the given URI. </p></li>
     *
     *   <li><p> Otherwise the new URI's authority component is copied from
     *   this URI, and its path is computed as follows: </p></li>
     *
     *   <ol type=a>
     *
     *     <li><p> If the given URI's path is absolute then the new URI's path
     *     is taken from the given URI. </p></li>
     *
     *     <li><p> Otherwise the given URI's path is relative, and so the new
     *     URI's path is computed by resolving the path of the given URI
     *     against the path of this URI.  This is done by concatenating all but
     *     the last segment of this URI's path, if any, with the given URI's
     *     path and then normalizing the result as if by invoking the {@link
     *     #normalize() normalize} method. </p></li>
     *
     *   </ol>
     *
     * </ol>
     *
     * <p> The result of this method is absolute if, and only if, either this
     * URI is absolute or the given URI is absolute.  </p>
     *
     * @param  uri  The URI to be resolved against this URI
     * @return The resulting URI
     */
    URI resolve(const URI &uri) const;

    /**
     * Compares this URI against another.
     *
     * <p>Before comparing the URIs are normalized. If this is not a
     * desired behaviour comparison operators should be used to compare
     * URIs without normalization.</p>
     * @param other The URI to compare to this.
     * @returns <code>0</code> if the URIs are considered equal,
     *          negative value if this is less than other and
     *          and positive value if this is greater than other.
     * @see operator==(const URI&, const URI&)
     * @see operator!=(const URI&, const URI&)
     * @see operator<(const URI&, const URI&)
     * @see operator>(const URI&, const URI&)
     * @see operator<=(const URI&, const URI&)
     * @see operator>=(const URI&, const URI&)
     */
    int compare(const URI &other) const noexcept;

    /**
     * Returns default port for given scheme or <tt>0</tt> if the default
     * port could not be found.
     *
     * @param scheme The scheme for which to find the default port
     *
     * @return default port for a given scheme or <tt>0</tt> if the
     *         default port is not found.
     */
    static uint16_t get_default_port(string_view scheme);

    /**
     * Decode all the persent encoded characters in the string.
     * @param str Strign view to decode.
     * @return Decoded string.
     */
    static std::string decode(string_view str);

private:
    void _initialize();
    void _initialize(string_view scheme, string_view user_info, string_view host, string_view port,
                     string_view path, string_view query, string_view fragment);

    void _parse(string_view::const_iterator &it, string_view::const_iterator last);
    bool _set_host_and_port(string_view::const_iterator first,
                            string_view::const_iterator last,
                            string_view::const_iterator last_colon);

    void _move_parts(const URI& other);
    void _resize_parts(std::size_t offset, int_fast16_t resize_bytes);
    void _decode_encoded_unreserved_chars();

    string_type _uri;
    string_view _uri_view;
    string_view _scheme;
    string_view _user_info;
    string_view _host;
    string_view _port;
    uint16_t    _port_i = 0;
    string_view _path;
    string_view _query;
    string_view _fragment;
    bool _normalized = false;

    static const std::vector<std::pair<std::string, uint16_t>> DEFAULT_PORTS;
};

/**
 * Output streaming operator overload for URI class objects.
 * @param out Output stream
 * @param uri URI object
 * @return Output stream
 */
template<typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out, const URI& uri)
{
    return out << uri.uri_view();
}

/**
 * \fn operator==(const URI&, const URI&)
 * Overloaded equality operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator==(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() == uri2.uri_view(); }

/**
 * \fn operator==(const URI&, const URI::string_type&)
 * Helper comparison method: uri1.string() == uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator==(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() == uri2; }

/**
 * Helper comparison method: uri1 == uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator==(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 == uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() == uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator==(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() == uri2; }

/**
 * Helper comparison method: uri1 == uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator==(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 == uri2.uri_view(); }

/**
 * Overloaded not equality operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator!=(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() != uri2.uri_view(); }

/**
 * Helper comparison method: uri1.string() != uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator!=(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() != uri2; }

/**
 * Helper comparison method: uri1 != uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator!=(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 != uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() != uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator!=(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() != uri2; }

/**
 * Helper comparison method: uri1 != uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator!=(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 != uri2.uri_view(); }

/**
 * Overloaded less-than operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() <  uri2.uri_view(); }

/**
 * Helper comparison method: uri1.string() < uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() < uri2; }

/**
 * Helper comparison method: uri1 < uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 < uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() < uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() < uri2; }

/**
 * Helper comparison method: uri1 < uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 < uri2.uri_view(); }

/**
 * Overloaded greater-than operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() >  uri2.uri_view(); }

/**
 * Helper comparison method: uri1.string() > uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() > uri2; }

/**
 * Helper comparison method: uri1 > uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 > uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() > uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() > uri2; }

/**
 * Helper comparison method: uri1 > uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 > uri2.uri_view(); }

/**
 * Overloaded less-or-equals operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<=(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() <= uri2.uri_view(); }

/**
 * Helper comparison method: uri1.string() <= uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<=(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() <= uri2; }

/**
 * Helper comparison method: uri1 <= uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<=(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 <= uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() <= uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<=(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() <= uri2; }

/**
 * Helper comparison method: uri1 <= uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator<=(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 <= uri2.uri_view(); }

/**
 * Overloaded greater-or-equals operator for URI objects.
 *
 * <p>It compares string object representations and doesn't do any preparation.
 * In order to compare normalized objects use #URI::compare</p>
 * @param uri1 URI to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>=(const URI& uri1, const URI& uri2) noexcept { return uri1.uri_view() >= uri2.uri_view(); }

/**
 * Helper comparison method: uri1.string() >= uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by std::string to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>=(const URI& uri1, const URI::string_type& uri2) noexcept { return uri1.string() >= uri2; }

/**
 * Helper comparison method: uri1 >= uri2.string();
 * @param uri1 URI represented by std::string to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>=(const URI::string_type& uri1, const URI& uri2) noexcept { return uri1 >= uri2.string(); }

/**
 * Helper comparison method: uri1.uri_view() >= uri2;
 * @param uri1 URI to compare
 * @param uri2 URI represented by URI::string_view to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>=(const URI& uri1, const URI::string_view& uri2) noexcept { return uri1.uri_view() >= uri2; }

/**
 * Helper comparison method: uri1 >= uri2.uri_view();
 * @param uri1 URI represented by URI::string_view to compare
 * @param uri2 URI to compare
 * @return the result of the comparison
 * @see #URI::compare
 */
inline bool operator>=(const URI::string_view& uri1, const URI& uri2) noexcept { return uri1 >= uri2.uri_view(); }

/**
 * Overloaded swap function of URI class objects.
 * @param lhs URI to swap
 * @param rhs URI to swap
 */
inline void swap(URI &lhs, URI &rhs) noexcept { lhs.swap(rhs); }

} // end of servlet namespace

#endif // MOD_SERVLET_URI_NEW_H
