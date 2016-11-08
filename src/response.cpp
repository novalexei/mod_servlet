/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <apr_strings.h>
#include <http_request.h>
#include <http_core.h>

#include <cctype>
#include <cstring>

#include "response.h"
#include "config.h"

namespace servlet
{

inline static bool _is_scheme_char(char c)
{
    return std::isalnum(c) || c == '+' || c == '-' || c == '.';
}

inline static bool _has_scheme(const std::string& location)
{
    bool firstChar = true;
    for (auto ch : location)
    {
        if(ch == ':') return !firstChar;
        else if(!_is_scheme_char(ch)) return false;
        firstChar = false;
    }
    return false;
}

static optional_ptr<const std::string> _to_absolute(const std::string &location, request_rec *r)
{
    if (location.empty()) return {&location};

    bool leadingSlash = location.front() == '/';

    if (location.size() > 1 && leadingSlash && location[1] == '/') /* location starts with "//" */
    {
        std::string *res = new std::string{};
        string_view scheme{ap_run_http_scheme(r)};
        res->reserve(scheme.length() + location.length() + 1);
        res->append(scheme.data(), scheme.length()).append(1, ':').append(location);
        return {res, true};
    }
    else if (leadingSlash || !_has_scheme(location))
    {
        string_view scheme{ap_run_http_scheme(r)};
        string_view host{ap_get_server_name_for_url(r)};
        apr_port_t port = ap_get_server_port(r);
        bool defaultPort = URI::get_default_port(scheme) == port;
        std::string res;
        res.reserve(scheme.length() + host.length() + location.size() + (defaultPort ? 3 : 10));

        res.append(scheme.data(), scheme.length()).append("://").append(host.data(), host.length());
        if (!defaultPort) res.append(1, ':') << port;

        if (!leadingSlash) res.append(r->parsed_uri.path).append(1, '/');
        res.append(location);
        URI normalized{std::move(res)};
        normalized.normalize_path();

        return {new std::string{normalized.string_move()}, true};
    }
    return {&location};
}

void http_response_base::add_header(const std::string &name, const std::string &value)
{
    apr_table_add(_request->headers_out, name.data(), value.data());
}
void http_response_base::set_header(const std::string &name, const std::string &value)
{
    apr_table_set(_request->headers_out, name.data(), value.data());
}
bool http_response_base::contains_header(const std::string &name) const
{
    return apr_table_get(_request->headers_out, name.data()) != nullptr;
}

static int add_value(std::vector<std::string> *values, const char *key, const char *val)
{
    values->emplace_back(val);
    return 1;
}
static int add_key_value(std::vector<std::pair<std::string, std::string>> *values, const char *key, const char *val)
{
    values->emplace_back(key, val);
    return 1;
}

string_view http_response_base::get_header(const std::string& name) const
{
    return apr_table_get(_request->headers_out, name.data());
}
long http_response_base::get_date_header(const std::string& name) const
{
    string_view view = get_header(name);
    if (view.empty()) return -1;
    return string_cast<long>(view, true);
}
void http_response_base::get_headers(const std::string& name, std::vector<std::string>& headers) const
{
    apr_table_do((int (*) (void *, const char *, const char *)) add_value,
                 (void *) &headers, _request->headers_out, name.data(), NULL);
}
void http_response_base::get_headers(std::vector<std::pair<std::string, std::string>>& headers) const
{
    apr_table_do((int (*) (void *, const char *, const char *)) add_key_value,
                 (void *) &headers, _request->headers_out, NULL);
}
string_view http_response_base::get_content_type() const
{
    return get_header("content-type");
}
void http_response_base::set_content_type(const std::string &content_type)
{
    ap_set_content_type(_request, content_type.data());
}
void http_response_base::set_content_length(std::size_t content_length)
{
    ap_set_content_length(_request, content_length);
}
void http_response_base::send_redirect(const std::string &redirectURL)
{
    set_header("Location", *_to_absolute(redirectURL, _request));
    _sc = SC_FOUND;
}

std::ostream& http_response_wrapper::get_output_stream()
{
    if (_out.has_value()) return *_out;
    std::ostream &out = _resp.get_output_stream();
    out_filter *f = filter();
    if (!f)
    {
        _out = &out;
        return *_out;
    }
    filtered_outstream *fout = new filtered_outstream{new stream_sink{out}};
    (*fout)->add_filter(f);
    _out.assign(fout, true);
    return *_out;
}

} // end of servlet namespace
