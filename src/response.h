/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_RESPONSE_H
#define MOD_SERVLET_IMPL_RESPONSE_H

#include <servlet/response.h>
#include <servlet/uri.h>
#include "time.h"

#include <http_protocol.h>

namespace servlet
{

class response_sink
{
public:
    response_sink(request_rec *req) : _request{req}, _count{0} {}
    ~response_sink() { flush(); }

    inline std::streamsize write(const char* s, std::streamsize n)
    {
        int bytesNum = ap_rwrite(s, static_cast<int>(n), _request);
        if (bytesNum < 0) return 0;
        _count += bytesNum;
        return bytesNum;
    }
    inline bool flush() { return ap_rflush(_request) == 0; }
    inline std::streamsize get_count() { return _count; }
private:
    request_rec *_request;
    std::streamsize _count;
};

typedef basic_outstream<response_sink, non_buffered, char> response_ostream;

class http_response_base : public http_response
{
public:
    explicit http_response_base(request_rec* request) noexcept : _request{request}, _out{_request} {}

    /* No copying, no moving */
    http_response_base(const http_response_base& ) = delete;
    http_response_base(http_response_base&& ) = delete;
    http_response_base& operator=(const http_response_base& ) = delete;
    http_response_base& operator=(http_response_base&& ) = delete;

    void add_cookie(const cookie& c) override { add_header("Set-cookie", c.to_string()); }

    void add_header(const std::string &name, const std::string &value) override;

    void add_date_header(const std::string &name, long timeSec) override
    {
        add_header(name, format_time("%a, %d-%b-%Y %H:%M:%S %Z", get_gmtm(timeSec), 32));
    }

    void set_header(const std::string &name, const std::string &value) override;

    void set_date_header(const std::string &name, long timeSec) override
    {
        set_header(name, format_time("%a, %d-%b-%Y %H:%M:%S %Z", get_gmtm(timeSec), 32));
    }

    bool contains_header(const std::string &name) const override;

    string_view get_header(const std::string& name) const override;
    long get_date_header(const std::string& name) const override;
    void get_headers(const std::string& name, std::vector<std::string>& headers) const override;
    void get_headers(std::vector<std::pair<std::string, std::string>>& headers) const override;

    string_view get_content_type() const override;
    void set_content_type(const std::string &content_type) override;
    void set_content_length(std::size_t content_length) override;

    void send_redirect(const std::string &redirectURL) override;

    void set_status(int sc) override { _sc = sc; }
    int get_status() const override { return _sc; }

    std::ostream& get_output_stream() { return _out; }

private:
    friend class http_servlet;

    request_rec *_request;
    response_ostream _out;
    int _sc = OK;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_RESPONSE_H
