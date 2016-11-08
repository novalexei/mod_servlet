#ifndef SERVLET_DISPATCHER_H
#define SERVLET_DISPATCHER_H

#include <algorithm>
#include <memory>
#include <experimental/string_view>
#include <experimental/filesystem>

#include "pattern_map.h"
#include "session.h"
#include <servlet/servlet.h>
#include <servlet/filter.h>
#include <servlet/lib/linked_map.h>
#include <servlet/lib/lru_map.h>
#include <servlet/session.h>
#include <servlet/context.h>

#include <apr_xml.h>
#include <apr_dso.h>
#include "context.h"
#include "config.h"
#include "map_ex.h"

namespace servlet
{

namespace fs = std::experimental::filesystem;

class dso
{
public:
    dso() : _dso{nullptr} {}
    dso(const std::string& path, apr_pool_t *pool);
    ~dso() noexcept { if (_dso) apr_dso_unload(_dso); }

    apr_dso_handle_t *get_dso() const { return _dso; }

private:
    apr_dso_handle_t *_dso;
};

class default_servlet : public http_servlet
{
protected:
    void do_get(http_request& request, http_response& response) override;
    void init() override;
private:
    std::shared_ptr<logging::logger> _logger = servlet_logger();
private:
    std::map<std::string, std::string, std::less<>> _mime_type_mapping;
    uint_fast16_t _max_extension_length;
    bool _use_accept_ranges = true;
};

class servlet_factory
{
public:
    servlet_factory(http_servlet* servlet, _servlet_config *cfg);
    servlet_factory(const std::shared_ptr<dso> &d, const std::string &sym, _servlet_config *cfg, int load_on_startup);
    ~servlet_factory() noexcept { if (_servlet_inited) delete _servlet; }

    http_servlet* get_servlet();

    int get_load_on_startup() const { return _load_on_startup; }
    _servlet_config* get_servlet_config() const { return _cfg.get(); }
private:

    std::unique_ptr<_servlet_config> _cfg;
    std::shared_ptr<dso> _dso;
    http_servlet* (*_factory)();
    http_servlet* _servlet = nullptr;
    int _load_on_startup;
    std::atomic<bool> _servlet_inited{false};
    std::mutex _init_mutex;
};

class filter_factory
{
public:
    filter_factory(const std::shared_ptr<dso> &d, const std::string &sym, _filter_config *cfg);
    ~filter_factory() noexcept { if (_filter_inited) delete _filter; }

    http_filter* get_filter();

private:
    std::unique_ptr<filter_config> _cfg;
    std::shared_ptr<dso> _dso;
    http_filter* (*_factory)();
    http_filter* _filter = nullptr;
    std::atomic<bool> _filter_inited{false};
    std::mutex _init_mutex;
};

class mapped_filter
{
public:
    mapped_filter(const std::shared_ptr<filter_factory> &factory, std::size_t order) :
            _factory{factory}, _order{order} {}

    http_filter* get_filter() { return _factory->get_filter(); }
    std::size_t get_order() const { return _order; }
    void set_order(std::size_t order) { _order = order; }

private:
    std::size_t _order;
    std::shared_ptr<filter_factory> _factory;
};

class filter_chain_holder
{
public:
    filter_chain_holder() = default;
    filter_chain_holder(std::shared_ptr<mapped_filter> ff) : _chain{} { _chain.push_back(ff); }
    filter_chain_holder(mapped_filter *ff) : _chain{} { _chain.push_back(std::shared_ptr<mapped_filter>(ff)); }
    ~filter_chain_holder() noexcept = default;

    void merge(std::shared_ptr<filter_chain_holder>& holder) {}
    void add(filter_chain_holder& holder);
    void add(std::shared_ptr<mapped_filter>& ff) { _chain.push_back(ff); }
    void finalize();

    const std::vector<std::shared_ptr<mapped_filter>>& get_chain() const { return _chain; }
private:
    std::vector<std::shared_ptr<mapped_filter>> _chain;
};

class _webapp_config
{
public:
    _webapp_config() : _servlets{}, _servlet_mapping{}, _session_timeout{30} {}
    std::size_t get_session_timeout() const { return _session_timeout; }
    void set_session_timeout(std::size_t session_timeout) { _session_timeout = session_timeout; }

    /** servlet name -> factory map */
    std::map<string_view, std::shared_ptr<servlet_factory>, std::less<>> &get_servlets() { return _servlets; }
    /** servlet url-pattern -> servlet-name map */
    std::map<string_view, string_view, std::less<>> &get_servlet_mapping() { return _servlet_mapping; }
    /** filter name -> factory map */
    std::map<string_view, std::shared_ptr<filter_factory>, std::less<>> &get_filters() { return _filters; }
    /** filter url-pattern -> filter-name map */
    tree_map<string_view, std::vector<std::pair<string_view, std::size_t>>> &get_filter_mapping()
    { return _filter_mapping; }
    /** servlet-name -> filter-names */
    tree_map<string_view, std::vector<std::pair<string_view, std::size_t>>> &get_filter_to_servlet_mapping()
    { return _filter_to_servlet_mapping; }

    std::map<std::string, std::string, std::less<>> &get_mime_type_mapping() { return _mime_type_mapping; }

private:
    std::map<string_view, std::shared_ptr<servlet_factory>, std::less<>> _servlets;
    std::map<string_view, string_view, std::less<>> _servlet_mapping;
    std::map<string_view, std::shared_ptr<filter_factory>, std::less<>> _filters;
    tree_map<string_view, std::vector<std::pair<string_view, std::size_t>>> _filter_mapping;
    tree_map<string_view, std::vector<std::pair<string_view, std::size_t>>> _filter_to_servlet_mapping;
    std::map<std::string, std::string, std::less<>> _mime_type_mapping;
    std::size_t _session_timeout;
};

class dispatcher
{
public:
    typedef pattern_map<std::shared_ptr<servlet_factory>>     servlet_map_type;
    typedef typename servlet_map_type::pair_type              pair_type;
    typedef pattern_map<std::shared_ptr<filter_chain_holder>> filter_map_type;
    typedef typename filter_map_type::pair_type               filter_pair_type;
    typedef lru_tree_map<std::string, std::shared_ptr<http_session_impl>> session_map_type;

    dispatcher(const fs::path &path, std::string &&ctx_path) :
            _path{path}, _ctx_path{std::move(ctx_path)}, _max_ext_length{0} { _init(); }
    dispatcher(fs::path &&path, std::string &&ctx_path) :
            _path{std::move(path)}, _ctx_path{std::move(ctx_path)}, _max_ext_length{0} { _init(); }
    ~dispatcher() noexcept { if (_pool) apr_pool_destroy(_pool); }

    const fs::path& webapp_path() const { return _path; }

    int service_request(request_rec* r, URI &uri);

private:
    optional_ptr<pair_type> _get_factory(string_view uri);

    void _init_filters(_webapp_config &cfg);
    void _init_servlets(_webapp_config &cfg);
    void _read_servlet_tag(apr_xml_elem *base_elem, _webapp_config& cfg,
                           std::map<std::string, std::shared_ptr<dso>>& dso_map);
    void _read_filter_tag(apr_xml_elem *base_elem, _webapp_config& cfg,
                          std::map<std::string, std::shared_ptr<dso>>& dso_map);
    std::shared_ptr<dso> _find_or_load_dso(std::map<std::string, std::shared_ptr<dso>>& dso_map,
                                           const std::string& lib_subpath);
    void _read_webapp_config(_webapp_config& cfg, apr_xml_elem *root);
    void _init();

    apr_pool_t *_pool;
    fs::path _path;
    std::string _ctx_path;
    std::unique_ptr<pair_type> _root_fac; /* Root servlet. Invoked if root is requested. */
    /* Servlet on '/' context. It is invoked if no other servlets found. */
    std::shared_ptr<servlet_factory> _catch_all;
    std::shared_ptr<servlet_factory> _dflt_servlet;
    std::shared_ptr<dso> _dflt_dso;
    std::map<std::string, std::shared_ptr<servlet_factory>, std::less<>> _ext_map;
    std::size_t _max_ext_length;
    std::shared_ptr<session_map_type> _session_map;
    std::shared_ptr<content_type_map> _content_types;

    pattern_map<std::shared_ptr<servlet_factory>> _servlet_map;

    pattern_map<std::shared_ptr<filter_chain_holder>> _filter_map;
    std::map<std::string, std::shared_ptr<filter_chain_holder>, std::less<>> _name_filter_map;
    std::shared_ptr<logging::log_registry> _log_registry;
    tree_map<int, std::string> _error_pages;
};

class webapp_dispatcher : public pattern_map<dispatcher>
{
public:
    typedef pattern_map<dispatcher> pattern_map_type;

    void init();
};

} // end of servlet namespace

#endif // SERVLET_DISPATCHER_H
