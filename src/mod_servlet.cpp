/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <exception>

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include <http_core.h>

#include "config.h"

#include <servlet/uri.h>
#include <servlet/servlet.h>
#include <apr_strings.h>
#include "pattern_map.h"
#include "dispatcher.h"

using namespace servlet;

APLOG_USE_MODULE(servlet); /* For logging */

static servlet::webapp_dispatcher WEBAPP_DISPATCHER;

inline static const char* p(const char* s) { return s ? s : "nullptr"; }

static int servlet_handler(request_rec* r)
{
    auto lg = servlet_logger();
    if (!r->handler || strcmp(r->handler, "servlet")) return DECLINED;

    int sc = OK;
    URI uri{ap_run_http_scheme(r), ap_get_server_name_for_url(r), ap_get_server_port(r),
            r->parsed_uri.path, r->parsed_uri.query};

    try
    {
        string_view path = uri.path();
        typename webapp_dispatcher::pair_type *web_pair = WEBAPP_DISPATCHER.get_pair(path);
        if (!web_pair) return DECLINED; /* Webapp is not found. Let's try to process it with apache default handler */
        if (SERVLET_CONFIG.translate_path) translate_path(r, uri.path());
        sc = web_pair->value.service_request(r, uri);
    }
    catch(const std::exception& e)
    {
        lg->info() << e << '\n';
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    catch(...)
    {
        lg->info() << "Unrecognized exception while processing request" << '\n';
        return HTTP_INTERNAL_SERVER_ERROR;
    }

//    timed_lru_tree_map<std::string, std::shared_ptr<http_session>> sessions{100};
//    servlet::http_request req{r, uri, "/test", "/tst", sessions};
//    servlet::http_response resp{r};
//    std::ostream &out = resp.get_output_stream();
//
//    request_instream &in = req.get_input_stream();
//    char buf[256];
//    std::streamsize read = 0;
//    while(in.read(buf, 256)) read += in.gcount();
////    int rv = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR);
////    if (rv != OK) return rv;
////    std::streamsize read = 0;
////    if (ap_should_client_block(r))
////    {
////        char buf[256];
////        std::streamsize len;
////        while((len = ap_get_client_block(r, buf, 256)) > 0) read += len;
////    }
//
//    out << "<!DOCTYPE html>\n"
//        << "<html>\n"
//        << "<head>\n"
//        << "<title>Servlet module is here</title>\n"
//        << "</head>\n"
//        << "<body>\n"
//        << "<h1>Servlet module is here!</h1>\n"
//        << "<p>Current thread: " << std::this_thread::get_id() << "</p>\n"
//        << "<p>r->finfo.filetype: " << r->finfo.filetype << "</p>\n"
//        << "<p>r->used_path_info: " << r->used_path_info << "</p>\n"
//        << "<p>r->path_info: " << p(r->path_info) << "</p>\n"
//
//        << "<p>bytes read from request: " << read << "</p>\n"
//        << "<p>r->remaining: " << r->remaining << "</p>\n"
//
//        << "<p>scheme: " << uri.get_scheme() << "</p>\n"
//        << "<p>hostname: " << uri.get_hostname() << "</p>\n"
//        << "<p>port: " << uri.get_port() << "</p>\n"
//        << "<p>path: " << uri.get_path() << "</p>\n"
//        << "<p>query: " << uri.get_query() << "</p>\n"
//        << "<p>ap_get_local_host: " << ap_get_local_host(r->pool) << "</p>\n"
//        << "<p>auth_type: " << p(r->ap_auth_type) << "</p>\n"
//        << "<p>canonical_filename: " << p(r->canonical_filename) << "</p>\n"
//        << "<p>chunked: " << r->chunked << "</p>\n"
//        << "<p>clength: " << r->clength << "</p>\n"
//        << "<p>hostname: " << p(r->hostname) << "</p>\n"
//        << "<p>filename: " << p(r->filename) << "</p>\n"
//        << "<p>handler: " << p(r->handler) << "</p>\n"
//        << "<p>method: " << p(r->method) << "</p>\n"
//        << "<p>status_line: " << p(r->status_line) << "</p>\n"
//        << "<p>path_info: " << p(r->path_info) << "</p>\n"
//        << "<p>parsed_uri.scheme: " << p(r->parsed_uri.scheme) << "</p>\n"
//        << "<p>parsed_uri.user: " << p(r->parsed_uri.user) << "</p>\n"
//        << "<p>parsed_uri.password: " << p(r->parsed_uri.password) << "</p>\n"
//        << "<p>parsed_uri.hostname: " << p(r->parsed_uri.hostname) << "</p>\n"
//        << "<p>parsed_uri.port: " << r->parsed_uri.port << "</p>\n"
//        << "<p>parsed_uri.path: " << p(r->parsed_uri.path) << "</p>\n"
//        << "<p>parsed_uri.query: " << p(r->parsed_uri.query) << "</p>\n"
//        << "<p>parsed_uri.fragment: " << p(r->parsed_uri.fragment) << "</p>\n"
//        << "<p>protocol: " << p(r->protocol) << "</p>\n"
//        << "<p>unparsed_uri: " << p(r->unparsed_uri) << "</p>\n"
//        << "<p>uri: " << p(r->uri) << "</p>\n";
//    std::vector<std::pair<std::string, std::string>> headers;
//    req.get_headers(headers);
//    for (auto &header : headers)
//    {
//        out << "<p>header: " << header.first << " -> " << header.second << "</p>\n";
//    }
////    out << "<p>number of cookies: " << cookies.size() << "</p>\n";
////    for (auto &ck : cookies)
////    {
////        out << "<p>cookie: " << ck.get_name() << " = " << ck.get_value() << "</p>\n";
////    }
////    out << "<p>session cookie: " << c.to_string() << "</p>\n";
//    out << "<p>connection->client_addr->servname: " << p(r->connection->client_addr->servname) << "</p>\n"
//        << "<p>connection->client_addr->hostname: " << p(r->connection->client_addr->hostname) << "</p>\n"
//        << "<p>connection->client_addr->port: " << r->connection->client_addr->port << "</p>\n"
//        << "<p>connection->client_ip: " << p(r->connection->client_ip) << "</p>\n"
//        << "<p>connection->local_addr->servname: " << p(r->connection->local_addr->servname) << "</p>\n"
//        << "<p>connection->local_addr->hostname: " << p(r->connection->local_addr->hostname) << "</p>\n"
//        << "<p>connection->local_addr->port: " << r->connection->local_addr->port << "</p>\n"
//        << "<p>connection->local_host: " << p(r->connection->local_host) << "</p>\n"
//        << "<p>connection->local_ip: " << p(r->connection->local_ip) << "</p>\n"
//        << "<p>connection->remote_host: " << p(r->connection->remote_host) << "</p>\n"
//        << "<p>connection->client_addr->hostname: " << p(r->connection->client_addr->hostname) << "</p>\n"
//        << "<p>connection->client_addr->servname: " << p(r->connection->client_addr->servname) << "</p>\n"
//        << "<p>server->path: " << p(r->server->path) << "</p>\n"
//        << "<p>ap_http_scheme(r): " << p(ap_run_http_scheme(r)) << "</p>\n"
//        << "<p>ap_get_server_name_for_url(r): " << p(ap_get_server_name_for_url(r)) << "</p>\n"
//        << "<p>ap_get_server_port(r): " << ap_get_server_port(r) << "</p>\n"
//        << "<p>server->server_scheme: " << p(r->server->server_scheme) << "</p>\n"
//        << "<p>server->server_admin: " << p(r->server->server_admin) << "</p>\n"
//        << "<p>server->server_hostname: " << p(r->server->server_hostname) << "</p>\n"
//        << "<p>server->addrs->host_addr->hostname: " << p(r->server->addrs->host_addr->hostname) << "</p>\n"
//        << "<p>server->addrs->host_addr->port: " << r->server->addrs->host_addr->port << "</p>\n"
//        << "<p>server->addrs->host_addr->servname: " << p(r->server->addrs->host_addr->servname) << "</p>\n";
//    out << "</body>\n"
//        << "</html>\n";

    return sc;
}

int post_config(apr_pool_t *conf_pool, apr_pool_t *log_pool, apr_pool_t *tmp_pool, server_rec *server)
{
    if (!WEBAPP_DISPATCHER.is_finalized())
    {
        servlet_config_t *cfg = (servlet_config_t *) ap_get_module_config(server->module_config, &servlet_module);
        finalize_servlet_config(cfg, tmp_pool);
        init_logging(cfg, tmp_pool);
        WEBAPP_DISPATCHER.init();
        WEBAPP_DISPATCHER.finalize();
    }
    return 0;
}

void register_hooks(apr_pool_t* pool)
{
    ap_hook_post_config(post_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler((ap_HOOK_handler_t *) servlet_handler, NULL, NULL, APR_HOOK_MIDDLE);
}
