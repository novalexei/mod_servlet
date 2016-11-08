#ifndef MOD_SERVLET_INPL_SESSION_H
#define MOD_SERVLET_INPL_SESSION_H

#include <servlet/session.h>

namespace servlet
{

class http_session_impl : public http_session
{
public:

    http_session_impl(const string_view &client_ip, const string_view &user_agent) :
            http_session{client_ip, user_agent} {}

    void validate(const string_view &client_ip, const string_view &user_agent);
};

} // end of servlet namespace

#endif // MOD_SERVLET_INPL_SESSION_H
