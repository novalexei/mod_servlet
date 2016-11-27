/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include "dispatcher.h"

#include <cstring>

#include "string.h"

namespace servlet
{

static void _read_servlet_mapping(_webapp_config& cfg, apr_xml_elem *base_elem)
{
    string_view url_pattern;
    string_view servlet_name;
    apr_xml_elem *elem = base_elem->first_child;
    while (elem)
    {
        if (std::strcmp(elem->name, "url-pattern") == 0)
        {
            url_pattern = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "servlet-name") == 0)
        {
            if (!elem->first_cdata.first || !elem->first_cdata.first->text)
                LG->warning() << "Tag servlet-mapping with empty servlet-name" << std::endl;
            else servlet_name = elem->first_cdata.first->text;
        }
        elem = elem->next;
    }
    if (!url_pattern.empty() && !servlet_name.empty())
        cfg.get_servlets().try_emplace(servlet_name).first->second.add_mapping(url_pattern);
}
static void _read_filter_mapping(_webapp_config& cfg, apr_xml_elem *base_elem, std::size_t order)
{
    string_view url_pattern;
    string_view servlet_name;
    string_view filter_name;
    apr_xml_elem *elem = base_elem->first_child;
    while (elem)
    {
        if (std::strcmp(elem->name, "url-pattern") == 0)
        {
            url_pattern = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "servlet-name") == 0)
        {
            if (!elem->first_cdata.first || !elem->first_cdata.first->text)
            {
                auto WRN = LG->warning();
                WRN << "Tag filter-mapping with empty servlet-name";
                if (!filter_name.empty()) WRN << " for filter-name " << filter_name;
                WRN << std::endl;
            }
            else servlet_name = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "filter-name") == 0)
        {
            if (!elem->first_cdata.first || !elem->first_cdata.first->text)
                LG->warning() << "Tag filter-mapping with empty filter-name" << std::endl;
            else filter_name = elem->first_cdata.first->text;
        }
        elem = elem->next;
    }
    if (filter_name.empty()) return;
    if (!url_pattern.empty())
    {
        cfg.get_filter_mapping().ensure_get(url_pattern).emplace_back(filter_name, order);
    }
    if (!servlet_name.empty())
    {
        cfg.get_filter_to_servlet_mapping().ensure_get(servlet_name).emplace_back(filter_name, order);
    }
}

static std::size_t _read_int(apr_xml_elem *base_elem, const char* int_element_name, std::size_t dflt)
{
    apr_xml_elem *elem = base_elem->first_child;
    while (elem)
    {
        if (std::strcmp(elem->name, int_element_name) == 0 && elem->first_cdata.first && elem->first_cdata.first->text)
        {
            return string_cast<std::size_t>(elem->first_cdata.first->text);
        }
        elem = elem->next;
    }
    return dflt;
}

static void _read_error_page(apr_xml_elem *base_elem, tree_map<int, std::string>& pages)
{
    int code = 0;
    string_view location;
    for (apr_xml_elem *elem = base_elem->first_child; elem; elem = elem->next)
    {
        if (std::strcmp(elem->name, "error-code") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text)
                code = string_cast<int>(elem->first_cdata.first->text);
        }
        else if (std::strcmp(elem->name, "location") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) location = elem->first_cdata.first->text;
        }
    }
    if (code > 0 && !location.empty()) pages.put(code, location.to_string());
}

static void _read_mime_type_mapping(apr_xml_elem *base_elem, std::map<std::string, std::string, std::less<>>& map)
{
    string_view key;
    string_view value;
    for (apr_xml_elem *elem = base_elem->first_child; elem; elem = elem->next)
    {
        if (std::strcmp(elem->name, "extension") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) key = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "mime-type") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) value = elem->first_cdata.first->text;
        }
    }
    if (!key.empty()) map.emplace(key.to_string(), value.to_string());
}

static void _read_init_param(apr_xml_elem *base_elem, std::map<std::string, std::string, std::less<>>& map)
{
    string_view key;
    string_view value;
    for (apr_xml_elem *elem = base_elem->first_child; elem; elem = elem->next)
    {
        if (std::strcmp(elem->name, "param-name") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) key = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "param-value") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) value = elem->first_cdata.first->text;
        }
    }
    if (!key.empty()) map.emplace(key.to_string(), value.to_string());
}

void dispatcher::_read_servlet_tag(apr_xml_elem *base_elem, _webapp_config& cfg,
                                   std::map<std::string, std::shared_ptr<dso>>& dso_map)
{
    string_view name;
    string_view factory;
    bool has_name = false;
    int load_on_startup = -2;
    std::map<std::string, std::string, std::less<>> init_params{};
    for (apr_xml_elem *elem = base_elem->first_child; elem; elem = elem->next)
    {
        if (std::strcmp(elem->name, "servlet-name") == 0)
        {
            has_name = true;
            if (elem->first_cdata.first && elem->first_cdata.first->text) name = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "servlet-factory") == 0)
        {
            if (!elem->first_cdata.first || !elem->first_cdata.first->text)
            {
                auto WRN = LG->warning();
                WRN << "Tag servlet with empty servlet-factory";
                if (!name.empty()) WRN << " for servlet-name " << name;
                WRN << std::endl;
            }
            else factory = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "load-on-startup") == 0)
        {
            string_view value;
            if (elem->first_cdata.first && elem->first_cdata.first->text)
                value = trim_view(elem->first_cdata.first->text);
            if (value.empty()) load_on_startup = -1;
            else
            {
                load_on_startup = string_cast<int>(value);
                if (load_on_startup < 0) load_on_startup = -1;
            }
        }
        else if (std::strcmp(elem->name, "init-param") == 0) _read_init_param(elem, init_params);
    }
    if (has_name)
    {
        auto s_it = cfg.get_servlets().find(name);
        if (s_it != cfg.get_servlets().end() && s_it->second.get_factory())
        {
            LG->warning() << "More than one definition for servlet " << name << std::endl;
            return;
        }
        if (factory.empty() && name == "default") /* Configuration for default servlet */
        {
            _servlet_config *s_config = new _servlet_config{name.to_string(), _ctx_path, _path, std::move(init_params)};
            std::shared_ptr<servlet_factory> sf{new servlet_factory{new default_servlet{}, s_config}};
            cfg.get_servlets().try_emplace(name).first->second.set_factory(sf);
            return;
        }
        auto colon_ind = factory.find(':');
        if (colon_ind == string_view::npos || colon_ind == 0 || colon_ind >= factory.size()-1)
            throw config_exception{"Invalid servlet-factory string: '" + factory + "'"};
        std::string dso_name = factory.substr(0, colon_ind).to_string();
        std::string symbol_name = factory.substr(colon_ind+1).to_string();
        std::shared_ptr<dso> d = _find_or_load_dso(dso_map, dso_name);
        _servlet_config *s_config = new _servlet_config{name.to_string(), _ctx_path, _path, std::move(init_params)};
        std::shared_ptr<servlet_factory> sf{new servlet_factory{d, symbol_name, s_config, load_on_startup}};
        cfg.get_servlets().try_emplace(name).first->second.set_factory(sf);
    }
}

void dispatcher::_read_filter_tag(apr_xml_elem *base_elem, _webapp_config &cfg,
                                  std::map<std::string, std::shared_ptr<dso>> &dso_map)
{
    string_view name;
    string_view factory;
    bool has_name = false;
    std::map<std::string, std::string, std::less<>> init_params{};
    for (apr_xml_elem *elem = base_elem->first_child; elem; elem = elem->next)
    {
        if (std::strcmp(elem->name, "filter-name") == 0)
        {
            if (elem->first_cdata.first && elem->first_cdata.first->text) name = elem->first_cdata.first->text;
            has_name = true;
        }
        else if (std::strcmp(elem->name, "filter-factory") == 0)
        {
            if (!elem->first_cdata.first || !elem->first_cdata.first->text)
            {
                auto WRN = LG->warning();
                WRN << "Tag filter with empty filter-factory";
                if (!name.empty()) WRN << " for filter-name " << name;
                WRN << std::endl;
            }
            else factory = elem->first_cdata.first->text;
        }
        else if (std::strcmp(elem->name, "init-param") == 0) _read_init_param(elem, init_params);
    }
    if (has_name)
    {
        auto colon_ind = factory.find(':');
        if (colon_ind == string_view::npos || colon_ind == 0 || colon_ind >= factory.size() - 1)
            throw config_exception{"Invalid servlet-factory string: '" + factory + "'"};
        std::string dso_name = factory.substr(0, colon_ind).to_string();
        std::string symbol_name = factory.substr(colon_ind + 1).to_string();
        std::shared_ptr<dso> d = _find_or_load_dso(dso_map, dso_name);
        _filter_config *s_config = new _filter_config{name.to_string(), _ctx_path, _path, std::move(init_params)};
        std::shared_ptr<filter_factory> ff{new filter_factory{d, symbol_name, s_config}};
        cfg.get_filters().emplace(name, ff);
    }
}

void dispatcher::_read_webapp_config(_webapp_config &cfg, apr_xml_elem *root)
{
    std::size_t filter_order = 0;
    std::map<std::string, std::shared_ptr<dso>> dso_map{};
    apr_xml_elem *elem = root->first_child;
    while (elem)
    {
        if (std::strcmp(elem->name, "servlet") == 0)
            _read_servlet_tag(elem, cfg, dso_map);
        else if (std::strcmp(elem->name, "filter") == 0)
            _read_filter_tag(elem, cfg, dso_map);
        else if (std::strcmp(elem->name, "servlet-mapping") == 0)
            _read_servlet_mapping(cfg, elem);
        else if (std::strcmp(elem->name, "filter-mapping") == 0)
            _read_filter_mapping(cfg, elem, filter_order++);
        else if (std::strcmp(elem->name, "mime-mapping") == 0)
            _read_mime_type_mapping(elem, cfg.get_mime_type_mapping());
        else if (std::strcmp(elem->name, "session-config") == 0)
            cfg.set_session_timeout(_read_int(elem, "session-timeout", 30));
        else if (std::strcmp(elem->name, "error-page") == 0)
            _read_error_page(elem, _error_pages);
        elem = elem->next;
    }
}

} // end of servlet namespace
