/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_CONTEXT_H
#define MOD_SERVLET_IMPL_CONTEXT_H

#include <memory>
#include <experimental/string_view>

#include <servlet/context.h>

namespace servlet
{

using std::experimental::string_view;

class content_type_map
{
public:
    content_type_map(std::map<std::string, std::string, std::less<>> &&mime_type_mapping);

    optional_ref<const std::string> get_content_type(const std::string& file_name) const;
    optional_ref<const std::string> get_content_type(string_view file_name) const;
private:
    std::map<std::string, std::string, std::less<>> _mime_type_mapping;
    uint_fast16_t _max_extension_length;
};

class _servlet_context : public servlet_context
{
public:
    _servlet_context(const std::string& ctx_path, const std::string& webapp_path, init_params_map &&init_params) :
            servlet_context{ctx_path, webapp_path, std::move(init_params)} {}

    std::map<std::string, std::string, std::less<>>& _access_init_params() { return _init_params_map; }

    optional_ref<const std::string> get_mime_type(string_view file_name) const override
    { return _content_types->get_content_type(file_name); }

    void set_content_types(std::shared_ptr<content_type_map> content_types) { _content_types = content_types; }

private:
    std::shared_ptr<content_type_map> _content_types;
};

class _servlet_config : public servlet_config
{
public:
    _servlet_config(std::string &&servlet_name, const std::string& ctx_path, const std::string& webapp_path) :
            servlet_config{std::move(servlet_name)}, _ctx{ctx_path, webapp_path, {}} {}
    _servlet_config(std::string &&servlet_name, const std::string& ctx_path, const std::string& webapp_path,
                    std::map<std::string, std::string, std::less<>> &&init_params) :
            servlet_config{std::move(servlet_name)}, _ctx{ctx_path, webapp_path, std::move(init_params)} {}
    ~_servlet_config() override = default;

    const servlet_context& get_servlet_context() const override { return _ctx; }

    _servlet_context& _get_servlet_context() { return _ctx; }

    void set_content_types(std::shared_ptr<content_type_map> content_types) { _ctx.set_content_types(content_types); }
private:
    _servlet_context _ctx;
};

class _filter_config : public filter_config
{
public:
    _filter_config(std::string &&filter_name, const std::string& ctx_path, const std::string& webapp_path,
                   std::map<std::string, std::string, std::less<>> &&init_params) :
            filter_config{std::move(filter_name)}, _ctx{ctx_path, webapp_path, std::move(init_params)} {}

    servlet_context& get_servlet_context() override { return _ctx; }

    _servlet_context& _get_servlet_context() { return _ctx; }

private:
    _servlet_context _ctx;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_CONTEXT_H
