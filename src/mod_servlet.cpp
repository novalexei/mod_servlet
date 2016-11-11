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

#include "pattern_map.h"
#include "dispatcher.h"

using namespace servlet;

APLOG_USE_MODULE(servlet); /* For logging */

static servlet::webapp_dispatcher WEBAPP_DISPATCHER;

inline static const char* p(const char* s) { return s ? s : "nullptr"; }

static int servlet_handler(request_rec* r)
{
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
        LG->info() << e << '\n';
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    catch(...)
    {
        LG->info() << "Unrecognized exception while processing request" << '\n';
        return HTTP_INTERNAL_SERVER_ERROR;
    }

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
