/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include "logger_format.h"
#include "properties.h"
#include <servlet/lib/linked_map.h>

namespace servlet { namespace logging {

void logger::set_log_output(log_output* new_out, SYNC_POLICY policy)
{
    locked_stream* stream;
    std::shared_ptr<log_output> new_out_ptr{new_out};
    switch (policy)
    {
        case SYNC_POLICY::SINGLE_THREAD:
            stream = new single_thread_locked_stream{new_out_ptr};
            break;
        case SYNC_POLICY::SYNC:
            stream = new sync_locked_stream{new_out_ptr};
            break;
        case SYNC_POLICY::ASYNC:
            stream = new async_locked_stream{new_out_ptr};
            break;
    }
    std::atomic_store(&_lock_stream, std::shared_ptr<locked_stream>(stream));
}

log_registry::log_registry()
{
    _prefix_printer_factories.emplace("simple", new simple_prefix_printer_factory{});
    _log_output_factories.emplace("console", new console_log_output_factory{});
    _log_output_factories.emplace("file", new file_log_output_factory{});
    _log_output_factories.emplace("size-file", new size_rotation_file_log_output_factory{});
    _log_output_factories.emplace("date-file", new date_rotation_file_log_output_factory{});
    _log_output_factories.emplace("date-size-file", new date_size_rotation_file_log_output_factory{});
}

std::shared_ptr<logger> log_registry::log(const std::string& name)
{
    auto it = _loggers.find(name);
    if (it != _loggers.end()) return it->second;
    std::shared_ptr<logger> lg = std::shared_ptr<logger>(create_new_logger(name));
    _loggers.emplace(name, lg);
    return lg;
}

static SYNC_POLICY _read_sync_policy(string_view str, SYNC_POLICY dflt)
{
    if (str.empty()) return dflt;
    if (equal_ic(str, "single-thread") || equal_ic(str, "single_thread")) return SYNC_POLICY::SINGLE_THREAD;
    else if (equal_ic(str, "sync"))  return SYNC_POLICY::SYNC;
    else if (equal_ic(str, "async")) return SYNC_POLICY::ASYNC;
    return dflt;
}
static LEVEL _read_level(string_view str, LEVEL dflt)
{
    if (str.empty()) return dflt;
    if (equal_ic(str, "critical"))     return LEVEL::CRITICAL;
    else if (equal_ic(str, "error"))   return LEVEL::ERROR;
    else if (equal_ic(str, "warning")) return LEVEL::WARNING;
    else if (equal_ic(str, "info"))    return LEVEL::INFO;
    else if (equal_ic(str, "config"))  return LEVEL::CONFIG;
    else if (equal_ic(str, "debug"))   return LEVEL::DEBUG;
    else if (equal_ic(str, "trace"))   return LEVEL::TRACE;
    return dflt;
}

LEVEL log_registry::get_log_level(const std::string &name)
{
    auto it = _properties.find(name + ".level");
    if (it == _properties.end() || it->second.empty()) return _log_level;
    return _read_level(_trim(it->second), _log_level.load());
}

logger *log_registry::create_new_logger(std::string name)
{
    return new logger{get_or_create_prefix_printer(), get_or_create_locked_stream(),
                      get_log_level(name), std::move(name), _loc};
}
void log_registry::set_base_directory(const std::string& base_dir)
{
    std::lock_guard<std::recursive_mutex> l{_config_mx};
    _base_dir = base_dir;
}
void log_registry::set_base_directory(std::string&& base_dir)
{
    std::lock_guard<std::recursive_mutex> l{_config_mx};
    _base_dir = std::move(base_dir);
}

void log_registry::set_synchronization_policy(SYNC_POLICY sync_policy)
{
    std::lock_guard<std::recursive_mutex> l{_config_mx};
    if (_sync_policy == sync_policy) return;
    if (!_loggers.empty()) throw config_exception{"cannot change synchronization policy runtime"};
    _sync_policy = sync_policy;
    std::atomic_exchange(&_locked_stream, std::shared_ptr<locked_stream>{});
}

void log_registry::set_log_output(log_output *log_out)
{
    std::lock_guard<std::recursive_mutex> lock{_config_mx};
    std::atomic_store(&_log_out, std::shared_ptr<log_output>(log_out));
}

std::shared_ptr<log_output> log_registry::get_or_create_output(bool force_create)
{
    if (!force_create && std::atomic_load(&_log_out)) return _log_out;
    std::lock_guard<std::recursive_mutex> lock(_config_mx);
    if (!force_create && std::atomic_load(&_log_out)) return _log_out; /* double checked lock here */
    log_output *out_ptr;
    auto it = _properties.find("output.handler");
    if (it == _properties.end())
    {
        out_ptr = new console_log_output{};
        out_ptr->load_config(_properties, "", _base_dir);
    }
    else
    {
        auto pp_it = _log_output_factories.find(it->second);
        if (pp_it == _log_output_factories.end())
        {
            out_ptr = new console_log_output{};
            out_ptr->load_config(_properties, "", _base_dir);
        }
        else
        {
            out_ptr = pp_it->second->new_log_output();
            out_ptr->load_config(_properties, it->second + ".", _base_dir);
        }
    }
    std::atomic_store(&_log_out, std::shared_ptr<log_output>(out_ptr));
    return _log_out;
}
std::shared_ptr<prefix_printer> log_registry::get_or_create_prefix_printer(bool force_create)
{
    if (!force_create && std::atomic_load(&_prefix_printer)) return _prefix_printer;
    std::lock_guard<std::recursive_mutex> lock(_config_mx);
    if (!force_create && std::atomic_load(&_prefix_printer)) return _prefix_printer; /* double checked lock here */
    prefix_printer *pp = nullptr;
    auto it = _properties.find("prefix.printer");
    if (it == _properties.end())
    {
        pp = new simple_prefix_printer{};
        pp->load_config(_properties, "");
    }
    else
    {
        auto pp_it = _prefix_printer_factories.find(it->second);
        if (pp_it == _prefix_printer_factories.end())
        {
            pp = new simple_prefix_printer{};
            pp->load_config(_properties, "");
        }
        else
        {
            pp = pp_it->second->new_prefix_printer();
            pp->load_config(_properties, it->second + ".");
        }
    }
    std::atomic_store(&_prefix_printer, std::shared_ptr<prefix_printer>(pp));
    return _prefix_printer;
}

std::shared_ptr<locked_stream> log_registry::get_or_create_locked_stream(bool force_create)
{
    if (!force_create && std::atomic_load(&_locked_stream)) return _locked_stream;
    std::lock_guard<std::recursive_mutex> lock(_config_mx);
    if (!force_create && std::atomic_load(&_locked_stream)) return _locked_stream; /* double checked lock here */
    locked_stream* stream;
    switch (_sync_policy.load())
    {
        case SYNC_POLICY::SINGLE_THREAD:
            stream = new single_thread_locked_stream{get_or_create_output(false)};
            break;
        case SYNC_POLICY::SYNC:
            stream = new sync_locked_stream{get_or_create_output(false)};
            break;
        case SYNC_POLICY::ASYNC:
            auto it = _properties.find("async.queue.size");
            std::size_t async_queue_size = log_registry::DEFAULT_ASYNC_QUEUE_SIZE;
            if (it != _properties.end() && !it->second.empty())
                async_queue_size = from_string(_trim(it->second), log_registry::DEFAULT_ASYNC_QUEUE_SIZE);
            stream = new async_locked_stream{get_or_create_output(false), async_queue_size};
            break;
    }
    std::atomic_store(&_locked_stream, std::shared_ptr<locked_stream>(stream));
    return _locked_stream;
}

void log_registry::reset_loggers_config(bool update_prefix_printer, bool update_log_output, bool update_locale)
{
    if (!update_log_output && !update_prefix_printer) return;
    for (auto &&lg : _loggers)
    {
        lg.second->set_log_level(get_log_level(lg.first));
        if (update_log_output) lg.second->set_locked_stream(_locked_stream);
        if (update_prefix_printer) lg.second->set_prefix_printer(_prefix_printer);
        if (update_locale) lg.second->imbue(_loc);
    }
}

void log_registry::read_configuration(const std::string& config_file_name,
                                      const std::string& base_dir, bool update_loggers)
{
    read_configuration(std::move(properties_file{config_file_name}), base_dir, update_loggers);
}

void log_registry::read_configuration(properties_type props, const std::string& base_dir, bool update_loggers)
{
    std::lock_guard<std::recursive_mutex> lock(_config_mx);
    _base_dir = base_dir;
    _properties = std::move(props);
    auto it = _properties.find("sync.policy");
    if (it != _properties.end()) _sync_policy = _read_sync_policy(_trim(it->second), SYNC_POLICY::SYNC);
    it = _properties.find(".level");
    if (it != _properties.end()) _log_level = _read_level(_trim(it->second), LEVEL::WARNING);
    it = _properties.find("locale");
    if (it != _properties.end()) _loc = std::locale{_trim(it->second).to_string()};
    get_or_create_prefix_printer(true);
    get_or_create_output(true);
    get_or_create_locked_stream(true);
    if (update_loggers) reset_loggers_config(true, true, true);
}

thread_local std::shared_ptr<log_registry> THREAD_REGISTRY;

log_registry& registry()
{
    if (THREAD_REGISTRY) return *THREAD_REGISTRY;
    static log_registry REGISTRY{};
    return REGISTRY;
}

log_registry& global_registry()
{
    static log_registry REGISTRY{};
    return REGISTRY;
}

void load_config(const char* cfg_file_name, const std::string& base_dir)
{
    global_registry().read_configuration(cfg_file_name, base_dir);
}

std::shared_ptr<logger> get_class_logger(log_registry& registry, const char* class_name)
{ return registry.log(demangle(class_name)); }

} // end of servlet::logging namespace

} // end of servlet namespace
