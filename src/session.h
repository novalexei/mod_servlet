/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_SESSION_H
#define MOD_SERVLET_IMPL_SESSION_H

#include <servlet/session.h>

namespace servlet
{

class http_session_impl : public http_session
{
public:

    http_session_impl(const string_view &client_ip, const string_view &user_agent) :
            http_session{client_ip, user_agent} {}

    void validate(const string_view &client_ip, const string_view &user_agent);

    void reset_session_id() override { http_session::reset_session_id(); }
};

class name_principal : public principal
{
public:
    name_principal(const std::string& name) : _name{name} {}
    name_principal(std::string&& name) : _name{std::move(name)} {}

    name_principal(const name_principal& other) = default;
    name_principal(name_principal&& other) = default;

    ~name_principal() noexcept override = default;

    name_principal& operator=(const name_principal& other) = default;
    name_principal& operator=(name_principal&& other) = default;

    string_view get_name() noexcept override { return _name; }
private:
    std::string _name;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_SESSION_H
