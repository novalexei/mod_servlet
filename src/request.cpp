/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <http_request.h>

#include "request.h"

namespace servlet
{

constexpr std::size_t MAX_POST_DATA_VALUE_SIZE = 2048;

const std::string http_request_base::SESSION_COOKIE_NAME = "CSESSIONID";

static std::string _to_local_path(const std::string &location, bool prepend_context,
                                  const string_view &context, const URI &uri)
{
    if (location.empty()) return prepend_context ? context.to_string() : uri.path().to_string();

    if (location.front() != '/') /* Relative path */
    {
        URI relative = uri.resolve(URI{location});
        return relative.uri_view().to_string();
    }
    else if (prepend_context)
    {
        URI combined{context + location};
        combined.normalize_path();
        return combined.uri_view().to_string();
    }

    return location;
}

http_request_base::http_request_base(request_rec *request, const URI &uri, const std::string &context_path,
                                     const std::string &srvlt_path, std::shared_ptr<session_type_map> session_map) :
        _request{request}, _uri{uri}, _ctx{context_path}, _srvlt_path{srvlt_path}, _session_map{session_map}
{
    if (_srvlt_path.back() == '/') _srvlt_path = _srvlt_path.substr(0, _srvlt_path.length()-1);
}

string_view http_request_base::get_header(const std::string& name) const
{
    return apr_table_get(_request->headers_in, name.data());
}

long http_request_base::get_date_header(const std::string& name) const
{
    string_view view = get_header(name);
    if (view.empty()) return -1;
    return string_cast<long>(view, true);
}

string_view http_request_base::get_content_type() const
{
    return get_header("content-type");
}
long http_request_base::get_content_length() const
{
    return from_string<long>(get_header("content-length"), -1l);
}
static int add_value(std::vector<std::string> *values, const char *key, const char *val)
{
    values->emplace_back(val);
    return 1;
}

void http_request_base::get_headers(const std::string& name, std::vector<std::string>& headers) const
{
    apr_table_do((int (*) (void *, const char *, const char *)) add_value,
                 (void *) &headers, _request->headers_in, name.data(), NULL);
}

static int add_key_value(std::vector<std::pair<std::string, std::string>> *values, const char *key, const char *val)
{
    values->emplace_back(key, val);
    return 1;
}

void http_request_base::get_headers(std::vector<std::pair<std::string, std::string>>& headers) const
{
    apr_table_do((int (*) (void *, const char *, const char *)) add_key_value,
                 (void *) &headers, _request->headers_in, NULL);
}

string_view http_request_base::get_remote_user() const
{
    if (_session)
    {
        std::shared_ptr<principal> p = _session->get_principal();
        return p ? p->get_name() : string_view{};
    }
    return ap_get_remote_logname(_request);
}

static int extract_cookie(std::vector<cookie> *cookies, const char *key, const char *val)
{
    tokenizer t{val, ";"};
    for (auto token : t)
    {
        string_view::size_type idx = token.find('=');
        if (idx == string_view::npos)
        {
            cookies->emplace_back(trim_view(token).to_string(), std::string{});
        }
        else
        {
            cookies->emplace_back(trim_view(token.substr(0, idx)).to_string(),
                                  trim_view(token.substr(idx + 1)).to_string());
        }
    }
    return 1;
}

void http_request_base::_parse_cookies()
{
    _cookies_parsed = true;
    apr_table_do((int (*) (void *, const char *, const char *)) extract_cookie,
                 (void *) &_cookies, _request->headers_in, "Cookie", "Cookie2", NULL);
}

void http_request_base::forward(const std::string &redirectURL, bool from_context_path)
{
    ap_internal_redirect(_to_local_path(redirectURL, from_context_path, _ctx, _uri).data(), _request);
}
int http_request_base::include(const std::string &includeURL, bool from_context_path)
{
    request_rec *subr = ap_sub_req_lookup_uri(_to_local_path(includeURL, from_context_path, _ctx, _uri).data(),
                                              _request, _request->output_filters);
    int status = ap_run_sub_req(subr);
    ap_destroy_sub_req(subr);
    return status;
}

string_view http_request_base::get_path_info() const
{
    string_view path = _uri.path();
    string_view::size_type prefix_length = _ctx.size() + _srvlt_path.size();
    if (_ctx.back() == '/' && _srvlt_path.front() == '/') --prefix_length;
    return path.length() <= prefix_length ? string_view{} : path.substr(prefix_length);
}

const std::string *http_request_base::_find_session_id_from_cookie()
{
    const std::vector<cookie> &cookies = get_cookies();
    for (auto &c : cookies)
    {
        if (SESSION_COOKIE_NAME == c.get_name()) return &c.get_value();
    }
    return nullptr;
}

http_session &http_request_base::get_session()
{
    if (_session) return *_session;
    const std::string* sid = _find_session_id_from_cookie();
    string_view client_ip = get_client_addr();
    string_view user_agent = get_header("User-Agent");
    if (sid)
    {
        auto ref = _session_map->get(*sid);
        if (ref.has_value())
        {
            (*(*ref))->validate(client_ip, user_agent);
            _session = *ref;
            if (_session->get_principal()) return *_session;
            const char *user = ap_get_remote_logname(_request);
            if (user) _session->set_principal(new named_principal{user});
            return *_session;
        }
    }
    _session.reset(new http_session_impl{client_ip, user_agent});
    while (!_session_map->try_put(_session->get_id(), _session))
    {
        _session->reset_session_id();
    }
    cookie sc{SESSION_COOKIE_NAME, _session->get_id()};
    sc.set_path(_ctx.to_string());
    apr_table_add(_request->headers_out, "Set-cookie", sc.to_string().data());
    const char *user = ap_get_remote_logname(_request);
    if (user) _session->set_principal(new named_principal{user});
    return *_session;
}

bool http_request_base::has_session()
{
    if (_session) return true;
    const std::string* sid = _find_session_id_from_cookie();
    if (sid)
    {
        auto ref = _session_map->get(*sid);
        if (ref.has_value()) return true;
    }
    return false;
}

void http_request_base::invalidate_session()
{
    if (!_session)
    {
        _session_map->erase(_session->get_id());
        _session.reset();
        return;
    }
    const std::string* sid = _find_session_id_from_cookie();
    if (sid)
    {
        _session_map->erase(*sid);
        /* Delete the cookie */
        cookie sc{SESSION_COOKIE_NAME, *sid};
        sc.set_max_age(0);
        apr_table_add(_request->headers_out, "Set-cookie", sc.to_string().data());
    }
}

const string_view& http_request_base::_get_content_type() const
{
    if (!_content_type.empty()) return _content_type;
    string_view ct = get_header("content-type");
    if (ct.empty()) return _content_type;
    typename string_view::size_type idx = ct.find(';');
    if (idx == string_view::npos) _content_type = trim_view(ct);
    else _content_type = trim_view(ct.substr(0, idx));
    return _content_type;
}

std::istream& http_request_base::get_input_stream()
{
    if (_in) return *_in;
    if (_multipart_in) return *(_in = &_multipart_in->get_input_stream());
    return *(_in = new request_instream{_request, SERVLET_CONFIG.input_stream_limit});
}

multipart_input& http_request_base::get_multipart_input()
{
    if (_in)
    {
        throw io_exception{"Failed to initialize multipart input. Regular input stream has already been initialized."};
    }
    string_view ct = get_header("content-type");
    if (_get_content_type() != "multipart/form-data")
    {
        throw io_exception{"Failed to initialize multipart input. Invalid content type: " + ct};
    }
    tokenizer tok{ct, ";,"};
    for (auto token : tok)
    {
        typename string_view::size_type idx = token.find('=');
        if (idx != string_view::npos)
        {
            if (trim_view(token.substr(0, idx)) == "boundary")
            {
                string_view boundary_view = trim_view(token.substr(idx+1));
                std::string boundary;
                /* Just add leading dashes to the boundary to simplify parsing */
                boundary.reserve(boundary_view.size()+2);
                boundary.append(2, '-').append(boundary_view.data(), boundary_view.size());
                _multipart_in = new multipart_input_impl{_request, boundary, SERVLET_CONFIG.input_stream_limit,
                                                         &_params, MAX_POST_DATA_VALUE_SIZE};
                return *_multipart_in;
            }
        }
    }
    throw io_exception{"Failed to initialize multipart input"};
}

bool http_request_base::is_multipart() const
{
    return _get_content_type() == "multipart/form-data";
}

const std::map<std::string, std::vector<std::string>, std::less<>>& http_request_base::get_parameters()
{
    if (!_params_parsed) _parse_params();
    return _params;
}

static int add_env(std::map<string_view, string_view> *values, const char *key, const char *val)
{
    values->emplace(key, val);
    return 1;
}

const std::map<string_view, string_view, std::less<>>& http_request_base::get_env()
{
    if (_env_loaded) return _env;
    _env_loaded = true;
    apr_table_do((int (*) (void *, const char *, const char *)) add_env,
                 (void *) &_env, _request->subprocess_env, NULL);
    return _env;
}

bool http_request_base::is_secure()
{
    return equal_ic(get_scheme(), "https") || static_cast<bool>(ssl_information());
}

std::shared_ptr<SSL_information> http_request_base::ssl_information()
{
    if (_ssl_inited) return _issl;
    _ssl_inited = true;
    auto it = get_env().find("HTTPS");
    if (it == _env.end() || (!equal_ic(it->second, "on") && !equal_ic(it->second, "true"))) return _issl;
    _issl.reset(new SSL_info{get_env()});
    return _issl;
}

void http_request_base::_parse_params()
{
    _params_parsed = true;
    if (_request->method_number == M_GET) /* Parse from URI query */
    {
        _parse_params(_uri.query());
    }
    else if (_request->method_number == M_POST) /* Read input stream */
    {
        string_view ct = _get_content_type();
        if (ct == "multipart/form-data")
        {
            multipart_input &input = get_multipart_input();
            while (input.to_next_part()) ; /* Just read the stream, it will be parsed automatically */
        }
        else if (ct != "application/x-www-form-urlencoded") return;
        else /* otherwise read form data */
        {
            std::istream &in = get_input_stream();
            inplace_ostream out{SERVLET_CONFIG.input_stream_limit};
            out << in.rdbuf();
            if (out->characters_written() > 0) _parse_params(string_view{out->str()});
        }
    }
}

void http_request_base::_parse_params(string_view query)
{
    if (query.empty()) return;
    URI::parse_query(query, [this] (const std::string& name, const std::string& value)
    {
        this->_params.ensure_get(std::move(name)).push_back(std::move(value));
    });
    for (auto &param : _params) param.second.shrink_to_fit();
}

class multipart_input_wrapper : public multipart_input
{
public:
    multipart_input_wrapper(multipart_input& in, in_filter *filter) : _in{in}, _filter{filter} {}
    ~multipart_input_wrapper() noexcept { delete _filter; delete _fin; }

    const std::map<std::string, std::vector<std::string>, std::less<>>& get_headers() const override
    { return _in.get_headers(); }

    bool to_next_part() override
    {
        delete _fin;
        _fin = nullptr;
        return _in.to_next_part();
    }

    std::istream& get_input_stream() override
    {
        if (_fin) return *_fin;
        std::istream &in = _in.get_input_stream();
        _fin = new filtered_instream{new stream_source{in}};
        (*_fin)->add_filter(_filter, false);
        return *_fin;
    }
private:
    multipart_input& _in;
    filtered_instream *_fin = nullptr;
    in_filter *_filter;
};

std::istream& http_request_wrapper::get_input_stream()
{
    if (_in.has_value()) return *_in;
    std::istream &in = _req.get_input_stream();
    in_filter *f = filter();
    if (!f)
    {
        _in = &in;
        return *_in;
    }
    filtered_instream *fin = new filtered_instream{new stream_source{in}};
    (*fin)->add_filter(f);
    _in.assign(fin, true);
    return *_in;
}
multipart_input& http_request_wrapper::get_multipart_input()
{
    if (_multipart_in.has_value()) return *_multipart_in;
    multipart_input &input = _req.get_multipart_input();
    in_filter *f = filter();
    if (!f)
    {
        _multipart_in = &input;
        return *_multipart_in;
    }
    _multipart_in.assign(new multipart_input_wrapper{input, f}, true);
    return *_multipart_in;
}

optional_ref<const std::string> multipart_input::get_submitted_filename() const
{
    optional_ref<const std::string> cd = get_header("Content-Disposition");
    if (!cd) return {};
    if (!begins_with_ic(*cd, "form-data") && !begins_with_ic(*cd, "attachment")) return {};
    return get_header("filename");
}

} //end of servlet namespace
