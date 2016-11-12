/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IMPL_MULTIPART_H
#define MOD_SERVLET_IMPL_MULTIPART_H

#include <servlet/request.h>
#include "map_ex.h"

namespace servlet
{

class request_mutipart_source
{
public:
    typedef buffer_provider category;

    request_mutipart_source(request_rec* request, const std::string &boundary, std::size_t in_limit,
                            std::map<std::string, std::vector<std::string>, std::less<>> *params,
                            std::size_t max_value_size, std::size_t buf_size = 1024);
    ~request_mutipart_source() noexcept { delete[] _buffer; ap_discard_request_body(_request); }

    std::pair<char*, std::size_t> get_buffer();

    bool next();
    const tree_map<std::string, std::vector<std::string>> &get_headers() const { return _headers; }

private:
    std::pair<char *, std::size_t> _get_buffer();
    void _parse_headers(char* buf, std::size_t buf_size);
    std::streamsize _process_buffer();
    void _read_to_buf();
    void _reinit_remainder(char rn1, char rn2 = '\0');

    request_rec *_request;
    std::size_t _in_limit;
    std::string _boundary;

    std::map<std::string, std::vector<std::string>, std::less<>> *_params;
    std::size_t _max_value_size;
    std::string _value;
    bool _reading_value = false;

    char *_remainder_buf;
    char *_remainder;
    std::size_t _remainder_size = 0;

    char *_buffer;
    std::size_t _buf_size;
    std::size_t _buf_ptr = 0;
    std::size_t _next_buf_ptr = 0;
    std::size_t _in_buf = 0;

    char * _next_ptr = nullptr;
    std::size_t _next_size = 0;
    bool _skip_to_next = false;

    bool _initializing = true;
    bool _eat_new_line = false;
    bool _check_for_ending_dashes = false;
    int_fast16_t _ending_dashes_read = 0;
    bool _current_stream_cosed = true;
    bool _stream_closed = false;

    std::size_t _in_count = 0;

    tree_map<std::string, std::vector<std::string>> _headers;
};

class multipart_instream : public instream<request_mutipart_source, non_buffered>
{
public:
    using instream<request_mutipart_source, non_buffered>::basic_instream;

    bool next()
    {
        if (this->bad() || this->fail() || !(*this)->next()) return false;
        this->clear();
        return true;
    }
};

class multipart_input_impl : public multipart_input
{
public:
    multipart_input_impl(request_rec* request, const std::string &boundary, std::size_t in_limit,
                         std::map<std::string, std::vector<std::string>, std::less<>> *params, std::size_t max_value_size) :
            _in{request, boundary, in_limit, params, max_value_size} {}

    const std::map<std::string, std::vector<std::string>, std::less<>>& get_headers() const override
    { return _in->get_headers(); }

    std::istream& get_input_stream() override { return _in; }
    bool to_next_part() override { return _in->next(); }

private:
    multipart_instream _in;
};

} // end of servlet namespace

#endif // MOD_SERVLET_IMPL_MULTIPART_H
