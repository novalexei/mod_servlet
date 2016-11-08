#include <servlet/servlet.h>
#include "string.h"

namespace servlet
{

const std::string http_servlet::METHOD_DELETE = "DELETE";
const std::string http_servlet::METHOD_HEAD = "HEAD";
const std::string http_servlet::METHOD_GET = "GET";
const std::string http_servlet::METHOD_OPTIONS = "OPTIONS";
const std::string http_servlet::METHOD_POST = "POST";
const std::string http_servlet::METHOD_PUT = "PUT";
const std::string http_servlet::METHOD_TRACE = "TRACE";

const std::string http_servlet::HEADER_IFMODSINCE = "If-Modified-Since";
const std::string http_servlet::HEADER_LASTMOD = "Last-Modified";

void http_servlet::init(servlet_config& cfg)
{
    _cfg = &cfg;
    init();
}

void http_servlet::service(http_request& req, http_response& resp)
{
    string_view method = req.get_method();

    if (method == METHOD_GET)
    {
        long lastModified = get_last_modified(req);
        if (lastModified == -1)
        {
            /* servlet doesn't support if-modified-since, no reason to go through further expensive logic */
            do_get(req, resp);
        }
        else
        {
            long ifModifiedSince;
            try
            {
                ifModifiedSince = req.get_date_header(HEADER_IFMODSINCE);
            }
            catch (bad_cast e)
            {
                /* Invalid date header - proceed as if none was set */
                ifModifiedSince = -1;
            }
            if (ifModifiedSince < (lastModified / 1000 * 1000))
            {
                /* If the servlet mod time is later, call doGet()
                 * Round down to the nearest second for a proper compare
                 * A if_modified_since of -1 will always be less */
                _maybe_set_last_modified(resp, lastModified);
                do_get(req, resp);
            }
            else
            {
                resp.set_status(http_response::SC_NOT_MODIFIED);
            }
        }

    }
    else if (method == METHOD_POST) do_post(req, resp);
    else if (method == METHOD_PUT) do_put(req, resp);
    else if (method == METHOD_DELETE) do_delete(req, resp);
    else if (method == METHOD_OPTIONS) do_options(req, resp);
    else if (method == METHOD_TRACE) do_trace(req, resp);
    else if (method == METHOD_HEAD)
    {
        _maybe_set_last_modified(resp, get_last_modified(req));
        do_head(req, resp);

    }
    else
    {
        /* Note that this means NO servlet supports whatever method was requested, anywhere on this server. */
        resp.set_status(http_response::SC_NOT_IMPLEMENTED);
    }
}

inline void _error_on_method(http_request &req, http_response &resp)
{
    string_view protocol = req.get_protocol();
    if (protocol.length() > 3 && protocol.compare(protocol.size()-3, 3, "1.1") == 0)
    {
        resp.set_status(http_response::SC_METHOD_NOT_ALLOWED);
    }
    else
    {
        resp.set_status(http_response::SC_BAD_REQUEST);
    }
}

void http_servlet::do_get(http_request &req, http_response &resp) { _error_on_method(req, resp); }
void http_servlet::do_post(http_request &req, http_response &resp) { _error_on_method(req, resp); }
void http_servlet::do_put(http_request &req, http_response &resp) { _error_on_method(req, resp); }
void http_servlet::do_delete(http_request &req, http_response &resp) { _error_on_method(req, resp); }

class empty_counting_out_filter : public out_filter
{
public:
    empty_counting_out_filter(std::size_t &count) : _count{count} {}

    std::streamsize write(char* s, std::streamsize n, basic_sink<char>& dst) override
    { _count += n; return n; }
private:
    std::size_t &_count;
};

class empty_counting_response_wrapper : public http_response_wrapper
{
public:
    empty_counting_response_wrapper(http_response &resp) : http_response_wrapper(resp) {}

    std::size_t get_count() const { return _count; }
protected:

    out_filter *filter() { return new empty_counting_out_filter{_count}; }
private:
    std::size_t _count = 0;
};

void http_servlet::do_head(http_request &req, http_response &resp)
{
    empty_counting_response_wrapper wrapper{resp};
    do_get(req, wrapper);
    resp.set_content_length(wrapper.get_count());
}

void http_servlet::do_trace(http_request &req, http_response &resp)
{
    std::string buffer{"TRACE"};
    string_view uri_view = req.get_request_uri().uri_view();
    buffer.append(uri_view.data(), uri_view.length());
    buffer << req.get_protocol();

    std::vector<std::pair<std::string, std::string>> headers;
    req.get_headers(headers);
    for (auto &header : headers) buffer.append(header.first).append(": ").append(header.second);

    buffer.append("\r\n");

    resp.set_content_type("message/http");
    resp.set_content_length(buffer.length());
    std::ostream &out = resp.get_output_stream();
    out << buffer;
    out.flush();
}
int http_servlet::get_allowed_methods()
{
    return GET_ALLOWED | POST_ALLOWED | PUT_ALLOWED | DELETE_ALLOWED | OPTIONS_ALLOWED | HEAD_ALLOWED | TRACE_ALLOWED;
}
void http_servlet::do_options(http_request &req, http_response &resp)
{
    int allowedMask = get_allowed_methods();
    std::string allow;
    if (allowedMask & GET_ALLOWED) allow += METHOD_GET;
    if (allowedMask & HEAD_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_HEAD;
    }
    if (allowedMask & POST_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_POST;
    }
    if (allowedMask & PUT_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_PUT;
    }
    if (allowedMask & DELETE_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_DELETE;
    }
    if (allowedMask & TRACE_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_TRACE;
    }
    if (allowedMask & OPTIONS_ALLOWED)
    {
        if (!allow.empty()) allow += ", ";
        allow += METHOD_OPTIONS;
    }

    resp.set_header("Allow", allow);
}

long http_servlet::get_last_modified(http_request& req) { return -1; }

void http_servlet::_maybe_set_last_modified(http_response &resp, long lastModifiedSec)
{
    if (resp.contains_header(HEADER_LASTMOD)) return;
    if (lastModifiedSec >= 0) resp.set_date_header(HEADER_LASTMOD, lastModifiedSec);
}

} // end of servlet namespace
