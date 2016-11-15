/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <httpd.h>
#include <http_config.h>
#include <apr_strings.h>

#include <cstring>
#include <experimental/filesystem>

#include <boost/core/demangle.hpp>

#include "string.h"
#include "properties.h"

AP_DECLARE_MODULE(servlet) =
        {
                STANDARD20_MODULE_STUFF,
                NULL,
                NULL,
                make_config_servlet_state,
                merge_make_config_servlet_state,
                config_servlet_cmds,
                register_hooks
        };

mod_servlet_config SERVLET_CONFIG;

std::shared_ptr<servlet::logging::logger> LG = servlet_logger();

namespace fs = std::experimental::filesystem;

const char *set_webapp_root(cmd_parms *cmd, void *dummy, const char *path)
{
    servlet_config_t *cfg = (servlet_config_t *) ap_get_module_config(cmd->server->module_config, &servlet_module);
    if (path) cfg->webapp_root = ap_server_root_relative(cmd->pool, path);
    return NULL;
}
const char *set_document_root(cmd_parms *cmd, void *dummy, const char *path)
{
    servlet_config_t *cfg = (servlet_config_t *) ap_get_module_config(cmd->server->module_config, &servlet_module);
    if (path) cfg->document_root = ap_server_root_relative(cmd->pool, path);
    return NULL;
}
const char *set_config_file(cmd_parms *cmd, void *dummy, const char *path)
{
    servlet_config_t *cfg = (servlet_config_t *) ap_get_module_config(cmd->server->module_config, &servlet_module);
    if (path) cfg->servlet_properties_file = ap_server_root_relative(cmd->pool, path);
    return NULL;
}

const char *set_error_log_dir(cmd_parms *cmd, void *dummy, const char *path)
{
    namespace fs = std::experimental::filesystem;
    servlet_config_t *cfg = (servlet_config_t *) ap_get_module_config(cmd->server->module_config, &servlet_module);
    if (path && *path)
    {
        const char *filepath = ap_server_root_relative(cmd->pool, path);
        fs::path el_path{filepath};
        std::string dirname = el_path.parent_path().generic_string();
        cfg->log_dir = apr_pstrndup(cmd->pool, dirname.data(), dirname.length());
    }
    return NULL;
}

void *make_config_servlet_state(apr_pool_t *pool, server_rec *s)
{
    servlet_config_t *cfg = (servlet_config_t *) apr_palloc(pool, sizeof(servlet_config_t));
    /* Populate with default values up front */
    cfg->webapp_root = ap_server_root_relative(pool, "webapps");
    cfg->servlet_properties_file = nullptr;
    cfg->document_root = nullptr;
    cfg->log_dir = ap_server_root_relative(pool, "logs");
    return cfg;
}

void *merge_make_config_servlet_state(apr_pool_t *p, void *basev, void *addv)
{
    servlet_config_t *base = (servlet_config_t *) basev;
    servlet_config_t *add = (servlet_config_t *) addv;

    if (base->document_root) add->document_root = base->document_root;
    if (base->webapp_root) add->webapp_root = base->webapp_root;
    if (base->servlet_properties_file) add->servlet_properties_file = base->servlet_properties_file;
    if (base->log_dir) add->log_dir = base->log_dir;
    return add;
}

static inline bool __substitute(const std::string& path, servlet::string_view subst_from,
                                const std::string& subst_to, std::string& to)
{
    std::string::size_type idx = path.find(subst_from.data());
    if (idx == std::string::npos) return false;
    to.clear();
    to.reserve(path.size() + subst_to.size() - subst_from.size());
    to.append(path, 0, idx).append(subst_to).append(path, idx+subst_from.size(), path.size()-idx-subst_from.size());
    return true;
}
static bool __read_path(const std::string& path, std::string& to)
{
    if (__substitute(path, "${ServerRoot}", SERVLET_CONFIG.server_root, to)) return true;
    if (__substitute(path, "${DocumentRoot}", SERVLET_CONFIG.document_root, to)) return true;
    if (__substitute(path, "${WebappRoot}", SERVLET_CONFIG.webapp_root, to)) return true;
    if (__substitute(path, "${LogDirectory}", SERVLET_CONFIG.log_directory, to)) return true;
    to = path;
    return false;
}

void finalize_servlet_config(servlet_config_t *cfg, apr_pool_t *tmp_pool)
{
    using namespace servlet;
    SERVLET_CONFIG.server_root.assign(ap_server_root_relative(tmp_pool, ""));
    SERVLET_CONFIG.log_directory.assign(cfg->log_dir);
    SERVLET_CONFIG.webapp_root.assign(cfg->webapp_root);
    SERVLET_CONFIG.document_root.assign(cfg->document_root);
    std::string props_file;
    if (cfg->servlet_properties_file) props_file.assign(cfg->servlet_properties_file);
    else /* Default location is ${webapp_root}/servlet.ini */
    {
        props_file = fs::absolute("servlet.ini", SERVLET_CONFIG.webapp_root).string();
    }
    properties_file props{props_file};
    auto log_config_file = props.get("logging.properties");
    if (log_config_file.has_value())
    {
        if (!__read_path(*log_config_file, SERVLET_CONFIG.logging_properties_file))
        {
            char *log_props;
            apr_filepath_merge(&log_props, SERVLET_CONFIG.webapp_root.data(),
                               SERVLET_CONFIG.logging_properties_file.data(), 0, tmp_pool);
            SERVLET_CONFIG.logging_properties_file.assign(log_props);
        }
    }
    else /* Default location is ${webapp_root}/logging.properties */
    {
        SERVLET_CONFIG.logging_properties_file = string_view{cfg->webapp_root} + "/logging.properties";
    }
    optional_ref<const std::string> translate_path = props.get("translate.filepath");
    if (translate_path.has_value())
    {
        string_view trimmed = trim_view(*translate_path);
        SERVLET_CONFIG.translate_path = !(equal_ic(trimmed, "off") || equal_ic(trimmed, "false"));
    }
    optional_ref<const std::string> share_sessions = props.get("share.sessions");
    if (share_sessions.has_value())
    {
        string_view trimmed = trim_view(*share_sessions);
        SERVLET_CONFIG.share_sessions = equal_ic(trimmed, "on") || equal_ic(trimmed, "true");
    }
    optional_ref<const std::string> session_timeout = props.get("session.timeout");
    if (session_timeout.has_value())
    {
        string_view trimmed = trim_view(*session_timeout);
        SERVLET_CONFIG.session_timeout = from_string<std::size_t>(trimmed, 30);
        if (SERVLET_CONFIG.session_timeout == 0)
        {
            SERVLET_CONFIG.session_timeout = std::numeric_limits<std::size_t>::max(); /* 0 is no limit */
        }
    }
    optional_ref<const std::string> input_limit = props.get("input.stream.limit");
    if (input_limit.has_value())
    {
        string_view trimmed = trim_view(*input_limit);
        SERVLET_CONFIG.input_stream_limit = from_string<std::size_t>(trimmed, DEFAULT_INPUT_STREAM_LIMIT);
        if (SERVLET_CONFIG.input_stream_limit == 0)
        {
            SERVLET_CONFIG.input_stream_limit = std::numeric_limits<std::size_t>::max(); /* 0 is no limit */
        }
    }
}

void translate_path(request_rec* r, servlet::string_view uri_path)
{
    bool eat_first_path_char = SERVLET_CONFIG.webapp_root.back() == '/' && uri_path.front() == '/';
    std::size_t combined_len = SERVLET_CONFIG.webapp_root.size() + uri_path.size();
    if (eat_first_path_char) --combined_len;
    char* res = (char*) apr_palloc(r->pool, combined_len + 1);
    std::memcpy(res, SERVLET_CONFIG.webapp_root.data(), SERVLET_CONFIG.webapp_root.size());
    if (eat_first_path_char) std::memcpy(res+SERVLET_CONFIG.webapp_root.size(), uri_path.data()+1, uri_path.size()-1);
    else std::memcpy(res + SERVLET_CONFIG.webapp_root.size(), uri_path.data(), uri_path.size());
    res[combined_len] = '\0';
    r->filename = res;
    r->canonical_filename = res;

    /* We don't update file status, as it will be updated by default servlet if needed */
//    apr_status_t rv = apr_stat(&r->finfo, r->filename, APR_FINFO_MIN | APR_FINFO_LINK, r->pool);
//    lg->config() << "checked stat for traslated path " << rv << "'" << std::endl;
//    if (r->used_path_info == AP_REQ_REJECT_PATH_INFO) return;
//    if (rv != APR_SUCCESS && rv != APR_INCOMPLETE) r->finfo.filetype = APR_NOFILE;
//    else r->used_path_info = AP_REQ_ACCEPT_PATH_INFO;
}

static servlet::logging::log_registry& servlet_log_registry()
{
    static servlet::logging::log_registry REGISTRY{};
    return REGISTRY;
}

bool LOGGING_INITIALIZED = false;

void init_logging(servlet_config_t *cfg, apr_pool_t *tmp_pool)
{
    servlet::logging::log_registry &registry  = servlet_log_registry();
    registry.set_base_directory(SERVLET_CONFIG.log_directory);
    if (fs::exists(fs::path{SERVLET_CONFIG.logging_properties_file}))
    {
        registry.read_configuration(SERVLET_CONFIG.logging_properties_file, SERVLET_CONFIG.log_directory);
    }
    else
    {
        servlet::logging::log_registry::properties_type props{{".level", "warning"},
                                                              {"output.handler", "file"},
                                                              {"file.log.file", "servlet.log"}};
        registry.read_configuration(std::move(props), SERVLET_CONFIG.log_directory);
    }
    LOGGING_INITIALIZED = true;
    if (!LG->is_loggable(servlet::logging::LEVEL::CONFIG)) return;
    LG->config() << "Configuration parameters:\n"
                 << "Server root: " << SERVLET_CONFIG.server_root << '\n'
                 << "Document root: " << SERVLET_CONFIG.document_root << '\n'
                 << "Webapp root: " << SERVLET_CONFIG.webapp_root << '\n'
                 << "Logging properties file: " << SERVLET_CONFIG.logging_properties_file << '\n'
                 << "Log directory: " << SERVLET_CONFIG.log_directory << '\n'
                 << "Input stream limit: " << SERVLET_CONFIG.input_stream_limit << '\n'
                 << "Translate path: " << std::boolalpha << SERVLET_CONFIG.translate_path << '\n'
                 << "Share sessions: " << SERVLET_CONFIG.share_sessions << '\n'
                 << "Session timeout: " << SERVLET_CONFIG.session_timeout << std::endl;
}

std::shared_ptr<servlet::logging::logger> servlet_logger(const std::string& name) { return servlet_log_registry().log(name); }
std::shared_ptr<servlet::logging::logger> servlet_logger() { return servlet_log_registry().log(); }

std::string demangle(const char* name) { return boost::core::demangle(name); }
