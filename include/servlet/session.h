/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
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

class principal;

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

    /**
     * Set the authenticated principal that is associated with this session.
     * This provides an <code>authenticator</code> with a means to cache a
     * previously authenticated principal, and avoid potentially expensive
     * <code>authenticate</code> calls on every request.
     *
     * @param p The new principal, or <code>nullptr</code> if none.
     */
    void set_principal(principal* p) { _principal.reset(p); }

    /**
     * Set the authenticated principal that is associated with this session.
     *
     * This is a <code>std::shared_ptr</code> version of call
     * #set_principal(principal*)
     *
     * @param p <code>std::shared_ptr</code> to the new principal.
     * @see #set_principal(principal*)
     */
    void set_principal(std::shared_ptr<principal> p) { _principal = p; }

    /**
     * Set the authenticated principal that is associated with this session.
     *
     * This is a <code>std::unique_ptr</code> version of call
     * #set_principal(principal*)
     *
     * @param p <code>std::unique_ptr</code> to the new principal.
     * @see #set_principal(principal*)
     */
    void set_principal(std::unique_ptr<principal>&& p) { _principal = std::move(p); }

    /**
     * Return the authenticated principal that is associated with this session.
     * This provides an <code>authenticator</code> with a means to cache a
     * previously authenticated principal, and avoid potentially expensive
     * <code>authenticate</code> calls on every request.
     *
     * @return principal associated with this session or empty
     *         <code>std::shared_ptr</code>.
     */
    std::shared_ptr<principal> get_principal() const { return _principal; }

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
     * Resets <code>session_id</code> for this session.
     *
     * This method can be used if generated random <code>session_id</code> already
     * taken by other session.
     */
    virtual void reset_session_id();

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
    std::shared_ptr<principal> _principal;
};

/**
 * This interface represents the abstract notion of a principal, which
 * can be used to represent any entity, such as an individual, a
 * corporation, and a login id.
 */
class principal
{
public:
    virtual ~principal() noexcept = default;

    /**
     * Returns the name of this principal.
     *
     * @return the name of this principal.
     */
    virtual string_view get_name() noexcept = 0;
};

/**
 * Simple inplementation of <code>servlet::principal</code> with
 * a name only.
 */
class named_principal : public principal
{
public:
    /**
     * Constructs principal with a given name.
     * @param name Name
     */
    named_principal(const std::string& name) : _name{name} {}
    /**
     * Constructs principal with a given name.
     * @param name Name
     */
    named_principal(std::string&& name) : _name{std::move(name)} {}

    ~named_principal() noexcept override = default;

    string_view get_name() noexcept override { return _name; }
private:
    std::string _name;
};

} // end of servlet namespace

#endif // MOD_SERVLET_SESSION_H
