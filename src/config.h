/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_CONFIG_H
#define SERVLET_CONFIG_H

#include <httpd.h>
#include <http_config.h>

#include <string>

#include <servlet/lib/logger.h>

extern module AP_MODULE_DECLARE_DATA servlet_module;

constexpr std::size_t DEFAULT_INPUT_STREAM_LIMIT = 1024 * 1024 * 2; /* 2Mb */

struct mod_servlet_config
{
    std::string webapp_root;
    std::string log_directory;
    std::string server_root;
    std::string document_root;
    std::string logging_properties_file;
    bool translate_path = true;
    std::size_t input_stream_limit = DEFAULT_INPUT_STREAM_LIMIT;
    bool share_sessions = false;
    std::size_t session_timeout = 30;
};

extern mod_servlet_config SERVLET_CONFIG;

void register_hooks(apr_pool_t* pool);
void *make_config_servlet_state(apr_pool_t *pool, server_rec *s);
void *merge_make_config_servlet_state(apr_pool_t *p, void *basev, void *addv);

const char *set_webapp_root(cmd_parms *cmd, void *dummy, const char *path);
const char *set_document_root(cmd_parms *cmd, void *dummy, const char *path);
const char *set_error_log_dir(cmd_parms *cmd, void *dummy, const char *path);
const char *set_config_file(cmd_parms *cmd, void *dummy, const char *path);

static const command_rec config_servlet_cmds[] =
        {
                AP_INIT_TAKE1("WebAppRoot", (cmd_func) set_webapp_root, NULL, RSRC_CONF,
                              "Base directory for deployed web applications"),
                AP_INIT_TAKE1("ErrorLog", (cmd_func) set_error_log_dir, NULL, RSRC_CONF,
                              "Global error log property"),
                AP_INIT_TAKE1("DocumentRoot", (cmd_func) set_document_root, NULL, RSRC_CONF, "Document root"),
                AP_INIT_TAKE1("ServletConfigurationFile", (cmd_func) set_config_file, NULL, RSRC_CONF,
                              "Path to servlet module configuration file"),
                {NULL}
        };

typedef struct
{
    const char *webapp_root;
    const char *log_dir;
    const char *document_root;
    const char *servlet_properties_file;
} servlet_config_t;

void finalize_servlet_config(servlet_config_t *cfg, apr_pool_t *tmp_pool);

void translate_path(request_rec* r, servlet::string_view uri_path);

void init_logging(servlet_config_t *cfg, apr_pool_t *tmp_pool);

std::shared_ptr<servlet::logging::logger> servlet_logger(const std::string& name);
std::shared_ptr<servlet::logging::logger> servlet_logger();

std::string demangle(const char* name);

#endif // SERVLET_CONFIG_H
