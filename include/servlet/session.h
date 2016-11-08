#ifndef MOD_SERVLET_SESSION_H
#define MOD_SERVLET_SESSION_H

#include <string>
#include <chrono>
#include <experimental/string_view>

#include <servlet/lib/any_map.h>

namespace servlet
{

std::string generate_session_id();

using std::experimental::string_view;
using std::experimental::any;

/**
 * Provides a way to identify a user across more than one page request or visit
 * to a Web site and to store information about that user.
 *
 * <p>The servlet container uses this interface to create a session between an HTTP
 * client and an HTTP server. The session persists for a specified time period,
 * across more than one connection or page request from the user. A session
 * usually corresponds to one user, who may visit a site many times. The server
 * can maintain a session in many ways such as using cookies or rewriting URLs.
 *
 * <p>This interface allows servlets to
 * <ul>
 * <li>View and manipulate information about a session, such as the session
 * identifier, last accessed time
 * <li>Bind objects to sessions, allowing user information to persist across
 * multiple user connections
 * </ul>
 *
 * <p>A servlet should be able to handle cases in which the client does not
 * choose to join a session, such as when cookies are intentionally turned off.
 * Until the client joins the session, <code>is_new</code> returns
 * <code>true</code>. If the client chooses not to join the session,
 * <code>http_request#get_session</code> will return a different session on
 * each request, and <code>is_new</code> will always return <code>true</code>.
 *
 * <p>Session information is scoped only to the current web application
 * (<code>servlet_context</code>), so information stored in one context will
 * not be directly visible in another.
 */
class http_session : public tree_any_map
{
public:
    /**
     * Type to be returned with #get_creation_time and #get_last_accessed_time
     */
    typedef std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> time_type;

    /**
     * Returns a string containing the unique identifier assigned to this
     * session. The identifier is assigned by the servlet container and is
     * implementation dependent.
     *
     * @return a string specifying the identifier assigned to this session
     */
    std::string& get_id() { return _session_id; }

    /**
     * Const version of method #get_id
     *
     * @return a const string specifying the identifier assigned to this session
     * @see #get_id
     */
    const std::string& get_id() const { return _session_id; }

    /**
     * Returns the time when this session was created as recorded by
     * <code>std::chrono::system_clock</code>.
     *
     * @return a <code>time_type</code> specifying when this session was created.
     */
    time_type get_creation_time() const { return _created; }

    /**
     * Returns the last time the client sent a request associated with this
     * session, as recorded by <code>std::chrono::system_clock</code>,
     * and marked by the time the container received the request.
     *
     * <p>Actions that your application takes, such as getting or setting a
     * value associated with the session, do not affect the access time.
     *
     * @return a <code>time_type</code> representing the last time the client
     *         sent a request associated with this session
     */
    time_type get_last_accessed_time() const { return _last_accessed; }

    /**
     * Returns <code>true</code> if the client does not yet know about the
     * session or if the client chooses not to join the session. For example, if
     * the server used only cookie-based sessions, and the client had disabled
     * the use of cookies, then a session would be new on each request.
     *
     * @return <code>true</code> if the server has created a session, but the
     *         client has not yet joined
     */
    bool is_new() const { return _new; }

protected:
    /**
     * Protected constructor.
     * @param client_ip  Client IP for which this session is being created
     * @param user_agent User agent for which this session is being created
     */
    http_session(const string_view &client_ip, const string_view &user_agent);

    /**
     * Validates client IP and user agent against this session ones.
     *
     * <p>This method compares client IP and user agent of the client which requests
     * this session and if they don't match stack_security_exception is thrown</p>
     * @param client_ip Client IP to validate
     * @param user_agent User agent to validate
     * @throws stack_security_exception if client IP of user agent don't match
     */
    virtual void validate(const string_view &client_ip, const string_view &user_agent) = 0;

    /**
     * Client IP string.
     */
    std::string _client_ip;
    /**
     * User agent string.
     */
    std::string _user_agent;
    /**
     * New flag for this session.
     * @see #is_new
     */
    bool _new = true;
    /**
     * Last accessed timestamp. Updated on validation.
     * @see #get_last_accessed_time
     */
    time_type _last_accessed;

private:
    std::string _session_id;
    time_type _created;
};

} // end of servlet namespace

#endif // MOD_SERVLET_SESSION_H
