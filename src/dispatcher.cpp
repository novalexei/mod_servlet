/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <experimental/filesystem>
#include <map>
#include <cstring>

#include "dispatcher.h"
#include "filter_chain.h"
#include "request.h"
#include "response.h"

namespace servlet
{

namespace fs = std::experimental::filesystem;

std::shared_ptr<dispatcher::session_map_type> GLOBAL_SESSIONS_MAP;

class pool_guard
{
    apr_pool_t * _pool;
public:
    pool_guard() { if (apr_pool_create(&_pool, NULL) != APR_SUCCESS) _pool = nullptr; }
    ~pool_guard() noexcept { if (_pool) apr_pool_destroy(_pool); }

    operator bool() const { return _pool != nullptr; }
    apr_pool_t *operator*() { return _pool; }
    apr_pool_t *operator->() { return _pool; }
};

class log_registry_guard
{
public:
    log_registry_guard(std::shared_ptr<logging::log_registry> reg) { logging::THREAD_REGISTRY = reg; }
    ~log_registry_guard() noexcept { logging::THREAD_REGISTRY.reset(); }
};

dso::dso(const std::string& path, apr_pool_t *pool)
{
    if (apr_dso_load(&_dso, path.data(), pool) != APR_SUCCESS)
    {
        char err_buf[256];
        LG->warning() << apr_dso_error(_dso, err_buf, 256) << std::endl;
        _dso = nullptr;
    }
}

servlet_factory::servlet_factory(http_servlet *servlet, _servlet_config *cfg) :
        _servlet{servlet}, _cfg{cfg}, _load_on_startup{-2}, _servlet_inited{true}
{
    if (_servlet)
    {
        if (cfg) _servlet->init(*_cfg);
        else _servlet->init();
    }
}

servlet_factory::servlet_factory(const std::shared_ptr<dso> &d, const std::string &sym,
                                 _servlet_config *cfg, int load_on_startup) :
        _cfg{cfg}, _dso{d}, _load_on_startup{load_on_startup}
{
    if (!_dso->get_dso() || sym.empty() ||
            apr_dso_sym((apr_dso_handle_sym_t*)&_factory, _dso->get_dso(), sym.data()) != APR_SUCCESS)
    {
        string_view s_name;
        if (_cfg) s_name = _cfg->get_servlet_name();
        LG->warning() << "Failed to load servlet " << s_name << " from " << sym << std::endl;
        _factory = nullptr;
    }
}

http_servlet* servlet_factory::get_servlet()
{
    bool srvlt_inited = _servlet_inited.load();
    if (srvlt_inited) return _servlet;
    std::lock_guard<std::mutex> lock{_init_mutex};
    srvlt_inited = _servlet_inited.load();
    if (srvlt_inited) return _servlet;
    if (!_factory)
    {
        if (LG->is_loggable(logging::LEVEL::INFO))
            LG->info() << "No factory found for servlet " << _cfg->get_servlet_name() << std::endl;
        _servlet_inited.store(true);
        return nullptr;
    }
    if (LG->is_loggable(logging::LEVEL::DEBUG))
        LG->debug() << "Creating servlet " << _cfg->get_servlet_name() << std::endl;
    _servlet = _factory();
    if (_servlet)
    {
        if (LG->is_loggable(logging::LEVEL::DEBUG))
            LG->debug() << "Initializing servlet " << _cfg->get_servlet_name()
                        << "(" << demangle(typeid(*_servlet).name()) << ')' << std::endl;
        _servlet->init(*_cfg);
    }
    _servlet_inited.store(true);
    return _servlet;
}

filter_factory::filter_factory(const std::shared_ptr<dso> &d, const std::string &sym, _filter_config *cfg) :
        _cfg{cfg}, _dso{d}
{
    if (!_dso->get_dso() || sym.empty() ||
            apr_dso_sym((apr_dso_handle_sym_t*)&_factory, _dso->get_dso(), sym.data()) != APR_SUCCESS)
    {
        string_view f_name;
        if (_cfg) f_name = _cfg->get_filter_name();
        LG->warning() << "Failed to load filter " << f_name << " from " << sym << std::endl;
        _factory = nullptr;
    }
}

http_filter* filter_factory::get_filter()
{
    bool fltr_inited = _filter_inited.load();
    if (fltr_inited) return _filter;
    std::lock_guard<std::mutex> lock{_init_mutex};
    fltr_inited = _filter_inited.load();
    if (fltr_inited) return _filter;
    if (!_factory)
    {
        if (LG->is_loggable(logging::LEVEL::INFO))
            LG->info() << "No factory found for filter " << _cfg->get_filter_name() << std::endl;
        _filter_inited.store(true);
        return nullptr;
    }
    if (LG->is_loggable(logging::LEVEL::DEBUG))
        LG->debug() << "Creating filter " << _cfg->get_filter_name() << std::endl;
    _filter = _factory();
    if (_filter)
    {
        if (LG->is_loggable(logging::LEVEL::DEBUG))
            LG->debug() << "Initializing filter " << _cfg->get_filter_name()
                        << "(" << demangle(typeid(*_filter).name()) << ')' << std::endl;
        _filter->init(*_cfg);
    }
    _filter_inited.store(true);
    return _filter;
}

void filter_chain_holder::add(filter_chain_holder& holder)
{
    _chain.reserve(_chain.size() + holder._chain.size());
    for (auto &&element : holder._chain) _chain.push_back(element);
}

void filter_chain_holder::finalize()
{
    auto cmp = [](std::shared_ptr<mapped_filter>& f1, std::shared_ptr<mapped_filter>& f2)
    {
        return f1->get_order() < f2->get_order();
    };
    std::sort(_chain.begin(), _chain.end(), cmp);
    auto identity_cmp = [](std::shared_ptr<mapped_filter>& f1, std::shared_ptr<mapped_filter>& f2)
    {
        return f1.get() == f2.get();
    };
    std::unique(_chain.begin(), _chain.end(), identity_cmp);
    _chain.shrink_to_fit();
}

static fs::path _find_lib_path(const fs::path& context_path, const std::string& lib_subpath)
{
    /* First check if this is a lib path from other webapp */
    if (lib_subpath.back() != ')') return context_path / "WEB-INF" / "lib" / lib_subpath;
    /* It is from other webapp */
    std::string::size_type open_bracket = lib_subpath.find('(');
    if (open_bracket == std::string::npos) throw config_exception{"Invalid library name: '" + lib_subpath + "'"};
    string_view lib_name = string_view{lib_subpath}.substr(0, open_bracket);
    int offset = lib_subpath[open_bracket+1] == '/' ? 2 : 1;
    int back_offset = lib_subpath[lib_subpath.length()-2] == '/' ? 2 : 1;
    std::string webapp_name = lib_subpath.substr(open_bracket+offset,
                                                 lib_subpath.length()-open_bracket-offset-back_offset);
    std::replace(webapp_name.begin(), webapp_name.end(), '/', '#');
    return fs::path{SERVLET_CONFIG.webapp_root} / webapp_name / "WEB-INF" /
            "lib" / fs::path{lib_name.begin(), lib_name.end()};
}

std::shared_ptr<dso> dispatcher::_find_or_load_dso(std::map<std::string, std::shared_ptr<dso>>& dso_map,
                                                   const std::string& lib_subpath)
{
    auto it = dso_map.find(lib_subpath);
    if (it != dso_map.end()) return it->second;
    fs::path lib_path = _find_lib_path(_path, lib_subpath);
    if (LG->is_loggable(logging::LEVEL::DEBUG)) LG->debug() << "Loading library " << lib_path << std::endl;
    std::string lib_path_str = lib_path.generic_string();
    std::shared_ptr<dso> d{new dso{lib_path_str.data(), _pool}};
    if (d->get_dso() == nullptr) throw config_exception{"Failed to load shared library for: '" + lib_path_str + "'"};
    dso_map.emplace(lib_subpath, d);
    return d;
}

static string_view get_extension(string_view uri, std::size_t max_ext_length)
{
    if (uri.size() > max_ext_length) uri = uri.substr(uri.size() - max_ext_length - 1);
    string_view::size_type found = uri.rfind('.');
    if (found == string_view::npos || found >= uri.size()-1) return string_view{};
    return uri.substr(found + 1);
}

optional_ptr<dispatcher::pair_type> dispatcher::_get_factory(string_view uri)
{
    if ((uri.empty() || uri == "/") && _root_fac.get()) return optional_ptr<pair_type>{_root_fac.get()};
    pair_type *res_pair = _servlet_map.get_pair(uri);
    if (res_pair)
    {
        if (res_pair->exact || _ext_map.empty()) return optional_ptr<pair_type>{res_pair};
        string_view ext = get_extension(uri, _max_ext_length);
        if (ext.empty()) return optional_ptr<pair_type>{res_pair};
        auto it = _ext_map.find(ext);
        return it == _ext_map.end() ? optional_ptr<pair_type>{res_pair} :
               optional_ptr<pair_type>{new pair_type{std::string{}, false, it->second}, true};
    }
    string_view ext = get_extension(uri, _max_ext_length);
    if (!ext.empty())
    {
        auto it = _ext_map.find(ext);
        if (it != _ext_map.end()) return optional_ptr<pair_type>{new pair_type{std::string{}, false, it->second}, true};
    }
    if (_catch_all) return optional_ptr<pair_type>{new pair_type{uri.to_string(), false, _catch_all}, true};
    return optional_ptr<pair_type>{new pair_type{uri.to_string(), false, _dflt_servlet}, true};
}

int dispatcher::service_request(request_rec* r, URI &uri)
{
    if (LG->is_loggable(logging::LEVEL::DEBUG)) LG->debug() << "Serving request " << uri << std::endl;
    string_view path = uri.path();
    string_view servlet_path = path.substr(_ctx_path.length());
    optional_ptr<pair_type> servlet_ptr = _get_factory(servlet_path);
    if (!servlet_ptr.has_value()) /* Servlet mapping is not found. Let's try process it with apache default handler */
    {
        if (LG->is_loggable(logging::LEVEL::DEBUG))
            LG->debug() << "No servlet detected for request " << uri << std::endl;
        return DECLINED;
    }
    log_registry_guard reg_guard{_log_registry};
    http_servlet *srvlt = servlet_ptr->value->get_servlet();
    if (!srvlt) /* No servlet created - default apache handling. */
    {
        _servlet_config *sConf = servlet_ptr->value->get_servlet_config();
        auto warning = LG->warning();
        warning << "Failed to create servlet ";
        if (sConf) warning << sConf->get_servlet_name();
        else warning << "unknown";
        warning << " for URL " << uri << std::endl;
        return DECLINED;
    }
    std::shared_ptr<filter_chain_holder> named_filters;
    auto named_filters_it = _name_filter_map.find(srvlt->get_servlet_name());
    if (named_filters_it != _name_filter_map.end()) named_filters = named_filters_it->second;
    filter_pair_type *filters_pair = _filter_map.get_pair(servlet_path);
    std::shared_ptr<filter_chain_holder> url_filters;
    if (filters_pair) url_filters = filters_pair->value;
    servlet::http_request_base req{r, uri, _ctx_path, servlet_ptr->uri_pattern, _session_map};
    servlet::http_response_base resp{r};
    if (named_filters)
    {
        if (url_filters)
        {
            _filter_chain chain{&url_filters->get_chain(), &named_filters->get_chain(), srvlt};
            chain.do_filter(req, resp);
        }
        else
        {
            _filter_chain chain{nullptr, &named_filters->get_chain(), srvlt};
            chain.do_filter(req, resp);
        }
    }
    else if (url_filters)
    {
        _filter_chain chain{&url_filters->get_chain(), nullptr, srvlt};
        chain.do_filter(req, resp);
    }
    else
    {
        if (LG->is_loggable(logging::LEVEL::TRACE))
        {
            LG->trace() << "Calling servlet " << srvlt->get_servlet_name() << " for URL " << uri << std::endl;
        }
        srvlt->service(req, resp);
    }
    int status = resp.get_status();
    auto found_it = _error_pages.find(status);
    if (found_it != _error_pages.end())
    {
        status = OK;
        req.forward(found_it->second);
    }
    return status;
}

class _apr_file
{
public:
    _apr_file(const char* file_path, apr_pool_t *pool)
    {
        if (apr_file_open(&_fd, file_path, APR_READ, APR_OS_DEFAULT, pool) != APR_SUCCESS) _fd = nullptr;
    }
    ~_apr_file() noexcept { if (_fd) apr_file_close(_fd); }

    apr_file_t* get_descriptor() const { return _fd; }
private:
    apr_file_t * _fd;
};

class filter_map_visitor : public tree_visitor<std::shared_ptr<filter_chain_holder>>
{
public:
    void in(std::shared_ptr<filter_chain_holder>& value) override { _filter_stack.push_back(value); }
    void out() override;
private:
    std::vector<std::shared_ptr<filter_chain_holder>> _filter_stack;
};

void filter_map_visitor::out()
{
    std::shared_ptr<filter_chain_holder> &removed_element = _filter_stack[_filter_stack.size() - 1];
    for (int i = 0; i < _filter_stack.size()-1; ++i) removed_element->add(*_filter_stack[i]);
    removed_element->finalize();
    _filter_stack.pop_back();
}

void dispatcher::_init_filters(_webapp_config &cfg)
{
    for (auto &&mapping : cfg.get_filter_mapping())
    {
        const bool exact = !mapping.first.empty() && mapping.first[mapping.first.length()-1] != '*';
        string_view url_pattern = !exact ? mapping.first.substr(0, mapping.first.length()-1) : mapping.first;
        if (exact && url_pattern.empty()) url_pattern = "/";
        for (auto &&f_item : mapping.second)
        {
            auto found = cfg.get_filters().find(f_item.first);
            if (found == cfg.get_filters().end())
            {
                throw config_exception{"Did not find filter with name '" + f_item.first +
                                       "' which is mapped to URL '" + mapping.first + "'"};
            }
            filter_chain_holder *holder = new filter_chain_holder{new mapped_filter{found->second, f_item.second}};
            if (LG->is_loggable(logging::LEVEL::DEBUG))
            {
                LG->debug() << "Setting filter URL mapping " << url_pattern
                            << (exact ? " -> " : "/* -> ") << f_item.first << std::endl;
            }
            _filter_map.add(url_pattern.to_string(), exact, std::shared_ptr<filter_chain_holder>{holder});
        }
    }
    filter_map_visitor visitor;
    _filter_map.traverse(visitor);
    for (auto &&fs_mapping : cfg.get_filter_to_servlet_mapping())
    {
        auto found = _name_filter_map.find(fs_mapping.first);
        std::shared_ptr<filter_chain_holder> name_filters;
        if (found == _name_filter_map.end())
        {
            name_filters.reset(new filter_chain_holder{});
            _name_filter_map.emplace(fs_mapping.first.to_string(), name_filters);
        }
        else name_filters = found->second;
        for (auto &&filter_name : fs_mapping.second)
        {
            auto fit = cfg.get_filters().find(filter_name.first);
            if (fit == cfg.get_filters().end())
            {
                throw config_exception{"Did not find filter with name '" + filter_name.first +
                                       "' which is mapped to servlet '" + fs_mapping.first + "'"};
            }
            if (LG->is_loggable(logging::LEVEL::DEBUG))
            {
                LG->debug() << "Setting filter to servlet mapping " << filter_name.first
                            << " -> " << fs_mapping.first << std::endl;
            }
            std::shared_ptr<mapped_filter> mf{new mapped_filter{fit->second, filter_name.second}};
            name_filters->add(mf);
        }
        name_filters->finalize();
    }
}

void dispatcher::_init_servlets(_webapp_config &cfg)
{
    std::vector<std::shared_ptr<servlet_factory>> servlets_to_load;
    std::shared_ptr<servlet_factory> ds;
    for (auto &&srvlt : cfg.get_servlets())
    {
        string_view servlet_name = srvlt.first;
        std::shared_ptr<servlet_factory> sf = srvlt.second.get_factory();
        std::vector<string_view>& mappings = srvlt.second.get_mappings();
        if (!sf)
        {
            if (servlet_name == "default")
            {
                ds.reset(new servlet_factory{new default_servlet{},
                                                          new _servlet_config{"default", _ctx_path, _path}});
                sf = ds;
            }
        }
        sf->get_servlet_config()->set_content_types(_content_types);
        if (sf->get_load_on_startup() != -2) servlets_to_load.push_back(sf);
        for (auto &&mapping : mappings)
        {
            const bool exact = !mapping.empty() && mapping[mapping.length()-1] != '*';
            string_view url_pattern = !exact ? mapping.substr(0, mapping.length()-1) : mapping;
            if (exact)
            {
                if (url_pattern == "/") _catch_all = sf;
                if (url_pattern.empty()) _root_fac.reset(new pair_type{"", true, sf});
                if (url_pattern.size() > 3 && url_pattern[0] == '*' && url_pattern[1] == '.') /* extension mapping */
                {
                    std::string ext = url_pattern.substr(2).to_string();
                    if (_max_ext_length < ext.size()) _max_ext_length = ext.length();
                    if (LG->is_loggable(logging::LEVEL::DEBUG))
                    {
                        _servlet_config *sc = sf->get_servlet_config();
                        auto DBG = LG->debug();
                        DBG << "Setting servlet extension mapping " << ext << " -> ";
                        if (sc) DBG << sc->get_servlet_name() << std::endl;
                        else DBG << "unknown" << std::endl;
                    }
                    _ext_map.emplace(std::move(ext), sf);
                }
                else
                {
                    if (LG->is_loggable(logging::LEVEL::DEBUG))
                    {
                        _servlet_config *sc = sf->get_servlet_config();
                        auto DBG = LG->debug();
                        DBG << "Setting servlet URL mapping " << url_pattern << (exact ? " -> " : "/* -> ");
                        if (sc) DBG << sc->get_servlet_name() << std::endl;
                        else DBG << "unknown" << std::endl;
                    }
                    _servlet_map.add(url_pattern.to_string(), exact, sf);
                }
            }
            else
            {
                if (LG->is_loggable(logging::LEVEL::DEBUG))
                {
                    _servlet_config *sc = sf->get_servlet_config();
                    auto DBG = LG->debug();
                    DBG << "Setting servlet URL mapping " << url_pattern << (exact ? " -> " : "/* -> ");
                    if (sc) DBG << sc->get_servlet_name() << std::endl;
                    else DBG << "unknown" << std::endl;
                }
                _servlet_map.add(url_pattern.to_string(), exact, sf);
            }
        }
    }
    if (!_dflt_servlet)
    {
        if (!ds)
        {
            ds.reset(new servlet_factory{new default_servlet{},
                                                      new _servlet_config{"default", _ctx_path, _path}});
            ds->get_servlet_config()->set_content_types(_content_types);
        }
        _dflt_servlet = ds;
    }
    auto cmp = [](std::shared_ptr<servlet_factory>& f1, std::shared_ptr<servlet_factory>& f2)
    {
        return f1->get_load_on_startup() < f2->get_load_on_startup();
    };
    std::sort(servlets_to_load.begin(), servlets_to_load.end(), cmp);
    _dflt_servlet->get_servlet(); /* load default servlet before the others if not loaded yet */
    for (auto &&servlet : servlets_to_load) /* first load with explicit order in load-on-startup */
    {
        if (servlet->get_load_on_startup() >= 0)
        {
            if (LG->is_loggable(logging::LEVEL::DEBUG))
            {
                auto DBG = LG->debug();
                DBG << "Loading servlet ";
                if (servlet->get_servlet_config()) DBG << servlet->get_servlet_config()->get_servlet_name();
                else DBG << "unknown";
                DBG << " on startup" << std::endl;
            }
            servlet->get_servlet();
        }
    }
    for (auto &&servlet : servlets_to_load) /* now let's load those without order in load-on-startup */
    {
        if (servlet->get_load_on_startup() < 0)
        {
            if (LG->is_loggable(logging::LEVEL::DEBUG))
            {
                auto DBG = LG->debug();
                DBG << "Loading servlet ";
                if (servlet->get_servlet_config()) DBG << servlet->get_servlet_config()->get_servlet_name();
                else DBG << "unknown";
                DBG << " on startup" << std::endl;
            }
            servlet->get_servlet();
        }
    }
    _servlet_map.finalize();
}

static std::shared_ptr<logging::log_registry> __init_log_registry(const fs::path &log_config_file,
                                                                  const std::string& context_path)
{
    std::shared_ptr<logging::log_registry> reg = std::make_shared<logging::log_registry>();
    if (fs::exists(log_config_file))
    {
        if (LG->is_loggable(logging::LEVEL::DEBUG))
            LG->debug() << "Reading logging properties from " << log_config_file
                        << " for context " << context_path << std::endl;
        reg->read_configuration(log_config_file.generic_string(), SERVLET_CONFIG.log_directory);
    }
    else
    {
        if (LG->is_loggable(logging::LEVEL::DEBUG))
            LG->debug() << "Using default logging properties for context " << context_path << std::endl;
        std::string log_file_name;
        if (context_path.front() == '/') log_file_name = context_path.substr(1);
        else log_file_name = context_path;
        std::replace(log_file_name.begin(), log_file_name.end(), '/', '#');
        log_file_name += ".log";
        logging::log_registry::properties_type props{{".level",         "warning"},
                                                     {"output.handler", "file"},
                                                     {"file.log.file",  log_file_name}};
        reg->read_configuration(std::move(props), SERVLET_CONFIG.log_directory);
    }
    return reg;
}

dispatcher::~dispatcher() noexcept
{
    LG->config() << "Cleaning webapp " << _path << std::endl;
    /* We need explicitly clean up the map to ensure all destructors are called. */
    _root_fac.reset();
    _catch_all.reset();
    _dflt_servlet.reset();
    _dflt_dso.reset();
    _ext_map.clear();
    _session_map.reset();
    _servlet_map.clear();
    _filter_map.clear();
    _name_filter_map.clear();
    if (_pool) apr_pool_destroy(_pool);
}

void dispatcher::_init()
{
    apr_xml_parser * parser;
    apr_xml_doc * doc;
    if (apr_pool_create(&_pool, NULL) != APR_SUCCESS)
    {
        _pool = nullptr;
        return;
    }

    pool_guard pool;
    fs::path web_xml_path = _path / "WEB-INF" / "web.xml";
    _webapp_config cfg;
    _log_registry = __init_log_registry(_path / "WEB-INF" / "logging.properties", _ctx_path);
    log_registry_guard reg_guard{_log_registry};
    if (fs::exists(web_xml_path))
    {
        {
            _apr_file fd{web_xml_path.generic_string().data(), *pool};
            if (apr_xml_parse_file(*pool, &parser, &doc, fd.get_descriptor(), 4096) != APR_SUCCESS) return;
        }
        _read_webapp_config(cfg, doc->root);
    }
    _content_types.reset(new content_type_map{std::move(cfg.get_mime_type_mapping())});
    if (SERVLET_CONFIG.share_sessions) _session_map = GLOBAL_SESSIONS_MAP;
    else _session_map.reset(new session_map_type{cfg.get_session_timeout()*60});

    _init_servlets(cfg);
    _init_filters(cfg);
}

void webapp_dispatcher::init()
{
    if (SERVLET_CONFIG.share_sessions && !GLOBAL_SESSIONS_MAP)
    {
        GLOBAL_SESSIONS_MAP.reset(new dispatcher::session_map_type{SERVLET_CONFIG.session_timeout*60});
    }
    for (auto &&webapp : fs::directory_iterator{fs::path{SERVLET_CONFIG.webapp_root}})
    {
        fs::path webapp_path = webapp.path();
        if (!fs::is_directory(webapp_path)) continue;
        std::string webapp_name = webapp_path.generic_string().substr(SERVLET_CONFIG.webapp_root.size());
        try
        {
            LG->config() << "Loading webapp " << webapp_name << std::endl;
            if ("/ROOT" == webapp_name)
                pattern_map_type::add(std::string{"/"}, false, webapp_path, std::move(webapp_name));
            else
            {
                std::replace(webapp_name.begin(), webapp_name.end(), '#', '/');
                pattern_map_type::add(webapp_name, false, webapp_path, std::move(webapp_name));
            }
        }
        catch(std::exception& ex)
        {
            LG->warning() << "Failed to configure webapp " << webapp_name << ": " << ex << std::endl;
        }
    }
}

} // end of servlet namespace
