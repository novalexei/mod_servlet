/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_REQUEST_H
#define MOD_SERVLET_IMPL_REQUEST_H

#include <servlet/request.h>
#include <servlet/lib/io.h>
#include "string.h"
#include <servlet/lib/lru_map.h>

#include <httpd.h>
#include <http_protocol.h>
#include <http_core.h>

#include "multipart.h"
#include "session.h"
#include "ssl.h"

namespace servlet
{

class request_source
{
public:
    request_source(request_rec *request, std::size_t in_limit) : _request{request}, _in_limit{in_limit}
    {
        if (ap_setup_client_block(_request, REQUEST_CHUNKED_DECHUNK) != OK || !ap_should_client_block(_request))
        {
            _request = nullptr;
        }
    }
    ~request_source() noexcept { if (!_request) ap_discard_request_body(_request); }
    std::streamsize read(char *buffer, std::size_t buf_size)
    {
        if (!_request || _count >= _in_limit) return 0;
        std::streamsize read = ap_get_client_block(_request, buffer, buf_size);
        _count += read;
        return read;
    }
    std::streamsize remaining() { return _request ? _request->remaining : 0; }
private:
    request_rec *_request;
    std::size_t _in_limit;
    std::size_t _count = 0;
};

typedef basic_instream<request_source, buffer_1k, char> request_instream;

class http_request_base : public http_request
{
public:
    typedef lru_tree_map<std::string, std::shared_ptr<http_session_impl>> session_type_map;

    http_request_base(request_rec *request, const URI &uri, const std::string &context_path,
                      const std::string &srvlt_path, std::shared_ptr<session_type_map> session_map);

    ~http_request_base() noexcept override { if (_multipart_in) delete _multipart_in; else delete _in; }

    tree_any_map& get_attributes() override { return _attributes; }
    const tree_any_map& get_attributes() const override { return _attributes; }

    const std::map<std::string, std::vector<std::string>, std::less<>>& get_parameters() override;

    const std::map<string_view, string_view, std::less<>>& get_env() override;

    bool is_secure();
    std::shared_ptr<SSL_information> ssl_information() override;

    string_view get_auth_type() override
    {
        return _request->ap_auth_type ? string_view{_request->ap_auth_type} : string_view{};
    }
    const std::vector<cookie>& get_cookies() override { if (!_cookies_parsed) _parse_cookies(); return _cookies; }
    string_view get_context_path() const override { return _ctx; }
    string_view get_servlet_path() const override { return _srvlt_path; }
    const URI& get_request_uri() const override { return _uri; }
    string_view get_path_info() const override;
    string_view get_header(const std::string& name) const override;
    long get_date_header(const std::string& name) const override;

    string_view get_content_type() const override;
    long get_content_length() const override;

    void get_headers(const std::string& name, std::vector<std::string>& headers) const override;
    void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const override;

    string_view get_method() const override { return _request->method; }

    string_view get_path_translated() const override { return _request->canonical_filename; }
    string_view get_scheme() const override { return ap_run_http_scheme(_request); }
    string_view get_protocol() const override { return _request->protocol; }

    string_view get_client_addr() const override { return _request->connection->client_ip; }
    string_view get_client_host() const override
    {
        return _request->connection->client_addr->hostname ? _request->connection->client_addr->hostname :
               _request->connection->client_ip;
    }
    uint16_t    get_client_port() const override { return _request->connection->client_addr->port; }
    string_view get_remote_user() const override;

    string_view get_local_addr() const override { return _request->connection->local_ip; }
    string_view get_local_host() const override { return ap_get_local_host(_request->pool); }
    uint16_t    get_local_port() const override { return _request->connection->local_addr->port; }

    uint16_t    get_server_port() const override { return ap_get_server_port(_request); }
    string_view get_server_name() const override { return ap_get_server_name_for_url(_request); }

    void forward(const std::string &redirectURL, bool from_context_path = true) override;
    int include(const std::string &includeURL, bool from_context_path = true) override;

    http_session &get_session() override;
    bool has_session() override;
    void invalidate_session() override;

    std::istream& get_input_stream() override;
    multipart_input& get_multipart_input() override;
    bool is_multipart() const override;

private:
    const string_view& _get_content_type() const;
    void _parse_cookies();
    const std::string *_find_session_id_from_cookie();
    void _parse_params();
    void _parse_params(string_view query);

    const static std::string SESSION_COOKIE_NAME;

    request_rec *_request;
    const URI &_uri;
    string_view _ctx;
    string_view _srvlt_path;
    std::vector<cookie> _cookies;
    bool _cookies_parsed = false;
    std::shared_ptr<http_session_impl> _session;
    std::shared_ptr<session_type_map> _session_map;

    std::map<std::string, std::vector<std::string>, std::less<>> _params;
    bool _params_parsed = false;
    tree_map<string_view, string_view> _env;
    bool _env_loaded = false;

    mutable string_view _content_type;

    std::istream *_in = nullptr;
    multipart_input_impl *_multipart_in = nullptr;

    tree_any_map _attributes;
    std::shared_ptr<SSL_info> _issl;
    bool _ssl_inited = false;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_REQUEST_H
