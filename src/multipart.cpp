/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include "request.h"

namespace servlet
{

request_mutipart_source::request_mutipart_source(request_rec *request, const std::string &boundary,
                                                 std::size_t in_limit,
                                                 std::map<std::string, std::vector<std::string>, std::less<>> *params,
                                                 std::size_t max_value_size, std::size_t buf_size) :
        _request{request}, _boundary{boundary}, _in_limit{in_limit}, _remainder_buf{new char[boundary.size() + 2]},
        _remainder{_remainder_buf}, _buf_size{buf_size > 0 ? buf_size : 1024},
        _params{params}, _max_value_size{max_value_size}
{
    _buffer = new char[_buf_size];
    if (_max_value_size == 0) _max_value_size = std::numeric_limits<std::size_t>::max();
    if (_in_limit == 0) _in_limit = std::numeric_limits<std::size_t>::max();
    if (ap_setup_client_block(_request, REQUEST_CHUNKED_DECHUNK) != OK || !ap_should_client_block(_request))
    {
        _request = nullptr;
    }
}

std::pair<char*, std::size_t> request_mutipart_source::get_buffer()
{
    if (!_request || _in_count >= _in_limit) return {nullptr, 0};
    std::pair<char*, std::size_t> res = _get_buffer();
    if (res.second + _in_count > _in_limit) res.second = _in_limit-_in_count;
    _in_count += res.second;
    if (_reading_value)
    {
        auto name_it = _headers.find("name");
        if (name_it != _headers.end())
        {
            if (res.second == 0)
            {
                _params->try_emplace(name_it->second.front()).first->second.push_back(_value);
                _value.clear();
                _reading_value = false;
            }
            else if (_value.size() < _max_value_size)
            {
                std::size_t sz = res.second > _max_value_size-_value.size() ?
                                 _max_value_size-_value.size() : res.second;
                _value.append(res.first, sz);
            }
        }
    }
    return res;
}

std::pair<char*, std::size_t> request_mutipart_source::_get_buffer()
{
    if (_stream_closed || (_current_stream_cosed && !_skip_to_next)) return {nullptr, 0};
    if (_next_size > 0)
    {
        if (_skip_to_next)
        {
            _next_ptr = nullptr;
            _next_size = 0;
        }
        else
        {
            if (_next_size > 0)
            {
                char* next_ptr = _next_ptr;
                std::size_t next_size = _next_size;
                _next_ptr = nullptr;
                _next_size = 0;
                return {next_ptr, next_size};
            }
        }
    }
    if (_next_buf_ptr != 0)
    {
        if (!_skip_to_next) return {nullptr, 0};
        _buf_ptr = _next_buf_ptr;
        _next_buf_ptr = 0;
    }
    while(true)
    {
        std::streamsize res = _process_buffer();
        if (res == 0)
        {
            if (_buf_ptr < _in_buf && _skip_to_next) continue;
            _read_to_buf();
            if (_in_buf == 0) return {nullptr, 0};
        }
        else if (res == -1)
        {
            std::size_t res_size = _remainder_size;
            std::size_t df = _remainder-_remainder_buf;
            _remainder_size = 0;
            _remainder = _remainder_buf;
            if (_skip_to_next) continue;
            return {_remainder_buf, res_size+df};
        }
        else if (res == -2)
        {
            return {nullptr, 0};
        }
        else /* res > 0 */
        {
            char* res_buffer = _buffer + _buf_ptr;
            _buf_ptr = _in_buf;
            if (_skip_to_next) continue;
            return {res_buffer, res};
        }
    }
}

bool request_mutipart_source::next()
{
    if (_stream_closed) return false;
    _skip_to_next = true;
    std::pair<char*, size_t> nb = get_buffer();
    if (_stream_closed || !nb.first) return false;
    _current_stream_cosed = false;
    _skip_to_next = false;
    _headers.clear();
    _parse_headers(nb.first, nb.second);
    _reading_value = _params && _headers.find("filename") == _headers.end();
    if (_reading_value)
    {
        optional_ref<std::vector<std::string>> values = _headers.get("Content-Disposition");
        if (values && begins_with_ic(values->front(), "attachment")) _reading_value = false;
    }
    return true;
}

void request_mutipart_source::_read_to_buf()
{
    if (_stream_closed)
    {
        _in_buf = 0;
        return;
    }
    std::streamsize read = ap_get_client_block(_request, _buffer, _buf_size);
    _buf_ptr = 0;
    _next_buf_ptr = 0;
    _in_buf = read < 0 ? 0 : static_cast<std::size_t>(read);
}

/* Doesn't equal = -1; equals = 0; partially equals = 1 */
int_fast16_t __cmp(const std::string& str, std::size_t start_pos, const char* to_cmp, std::size_t to_cmp_len)
{
    if (to_cmp_len >= str.size()-start_pos)
    {
        if (str.compare(start_pos, str.size()-start_pos, to_cmp, str.size()-start_pos) == 0) return 0;
        return -1; /* Doesn't equal */
    }
    else if(str.compare(start_pos, to_cmp_len, to_cmp, to_cmp_len) == 0) return 1;
    return -1;
}

inline int_fast16_t __eat_new_line(const char* buffer, std::size_t len)
{
    int_fast16_t num_eaten = 0;
    while (num_eaten < len && (buffer[num_eaten] == '\r' || buffer[num_eaten] == '\n')) ++ num_eaten;
    return num_eaten;
}
inline int_fast16_t __check_for_dashes(const char* buffer, std::size_t len)
{
    int_fast16_t num_found = 0;
    while (num_found < len && buffer[num_found] == '-') ++num_found;
    return num_found;
}

void request_mutipart_source::_reinit_remainder(char rn1, char rn2)
{
    if (rn1 != '\0')
    {
        _remainder_buf[0] = rn1;
        _remainder = _remainder_buf+1;
    }
    if (rn2 != '\0')
    {
        _remainder_buf[1] = rn2;
        _remainder = _remainder_buf+2;
    }
    _remainder_size = 0;
}

/* returns val == -1 if buffer needs to be copied to remainder; val == -2 return empty buffer;
 * val == 0 if needs to read more and val > 0 to return buffer with a given size */
std::streamsize request_mutipart_source::_process_buffer()
{
    if (_in_buf <= _buf_ptr) return 0;
    if (_check_for_ending_dashes)
    {
        int_fast16_t dashes = __check_for_dashes(_buffer+_buf_ptr, _in_buf-_buf_ptr);
        if (dashes+_ending_dashes_read >= 2)
        {
            _in_buf = 0;
            _stream_closed = true;
            return -2;
        }
        else if (_in_buf <= _buf_ptr+dashes)
        {
            _ending_dashes_read += dashes;
            _buf_ptr += dashes;
            return 0;
        }
        _ending_dashes_read = 0;
        _check_for_ending_dashes = false;
        _eat_new_line = true;
    }
    if (_eat_new_line)
    {
        _buf_ptr += __eat_new_line(_buffer+_buf_ptr, _in_buf-_buf_ptr);
        if (_in_buf <= _buf_ptr) return 0;
        _eat_new_line = false;
        _current_stream_cosed = true;
        if (!_skip_to_next) return -2;
        _skip_to_next = false;
    }
    if (_remainder_size == 0 && _remainder-_remainder_buf == 1 && _buffer[_buf_ptr] == '\n' && _remainder_buf[0] == '\r')
    {
        _reinit_remainder('\0', '\n');
        ++_buf_ptr;
    }
    if (_remainder_buf != _remainder || _remainder_size > 0 || _initializing)
    {
        _initializing = false;
        int_fast16_t res = __cmp(_boundary, _remainder_size, _buffer+_buf_ptr, _in_buf-_buf_ptr);
        if (res < 0)
        {
            if (_remainder_size > 0 || _remainder_buf != _remainder) return -1; /* feed remainder */
        }
        else if (res == 0)
        {
            _buf_ptr += _boundary.size()-_remainder_size;
            _remainder_size = 0;
            _remainder = _remainder_buf;
            _check_for_ending_dashes = true;
            return _skip_to_next ? 0 : -2;
        }
        else /* res > 0 */
        {
            std::copy_n(_buffer+_buf_ptr, _in_buf-_buf_ptr, _remainder+_remainder_size);
            _remainder_size += _in_buf-_buf_ptr;
            _buf_ptr = _in_buf;
            return 0; /* Read more */
        }
    }
    /* here _remainder_size should always be 0 */
    string_view buf_view{_buffer+_buf_ptr, _in_buf-_buf_ptr};
    string_view::size_type start_pos = 0;
    while (true)
    {
        start_pos = buf_view.find('\n', start_pos);
        if (start_pos == string_view::npos)
        {
            if (buf_view[buf_view.size()-1] == '\r')
            {
                _reinit_remainder('\r');
                return _in_buf-_buf_ptr-1;
            }
            return _in_buf-_buf_ptr;
        }
        else if (start_pos+1 == buf_view.size())
        {
            if (buf_view.size() > 1)
            {
                if (buf_view[buf_view.size()-2] == '\r')
                {
                    _reinit_remainder('\r', '\n');
                    return _in_buf-_buf_ptr-2;
                }
                else
                {
                    _reinit_remainder('\n');
                    return _in_buf-_buf_ptr-1;
                }
            }
            else if (_remainder-_remainder_buf == 1 && _remainder_buf[0] == '\r')
            {
                _reinit_remainder('\0', '\n');
                return _in_buf-_buf_ptr-1;
            }
            else
            {
                _reinit_remainder('\n');
                return _in_buf-_buf_ptr-1;
            }
        }
        ++start_pos;
        int_fast16_t res = __cmp(_boundary, 0, _buffer+_buf_ptr+start_pos, _in_buf-_buf_ptr-start_pos);
        if (res == 0)
        {
            _check_for_ending_dashes = true;
            _next_buf_ptr = _buf_ptr + start_pos + _boundary.size();
            if (start_pos == 1 || (start_pos == 2 && buf_view[0] == '\r'))
            {
                _remainder = _remainder_buf;
                _remainder_size = 0;
                return -2;
            }
            return start_pos - (buf_view[start_pos-2] == '\r' ? 2 : 1);
        }
        else if (res < 0) continue;
        else /* res > 0 */
        {
            --start_pos;
            string_view::size_type r_adjust = 1;
            if (start_pos > 0 && buf_view[start_pos-1] == '\r')
            {
                r_adjust = 2;
                --start_pos;
            }
            std::copy_n(_buffer+_buf_ptr+start_pos, _in_buf-_buf_ptr-start_pos, _remainder_buf);
            _remainder_size += _in_buf-_buf_ptr-start_pos-r_adjust;
            _remainder = _remainder_buf+r_adjust;
            return start_pos;
        }
    }
}

void __trim_if_needs(std::string& str, bool remove_quotes)
{
    if (str.empty()) return;
    string_view s{str};
    if (s.front() == ' ' || s.front() == '\t' || s.back() == ' ' || s.back() == '\t') s = trim_view(s);
    if (remove_quotes && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size()-2);
    if (s.size() != str.size()) str = s.to_string();
}

void __add_to_headers(std::string& name, std::string& value, tree_map<std::string, std::vector<std::string>>& headers)
{
    if (name.empty()) return;
    __trim_if_needs(name, false);
    __trim_if_needs(value, true);
    headers.ensure_get(name).push_back(value);
    name.clear();
    value.clear();
}

void request_mutipart_source::_parse_headers(char* buf, std::size_t buf_size)
{
    std::string name;
    std::string value;
    int new_lines_in_a_raw = 0;
    bool parsingName = true;
    bool parsing = true;
    std::size_t idx = 0;
    for (; parsing; ++idx)
    {
        if (idx >= buf_size)
        {
            std::pair<char*, size_t> nb = get_buffer();
            if (_stream_closed || !nb.first)
            {
                __add_to_headers(name, value, _headers);
                break;
            }
            idx = 0;
            buf = nb.first;
            buf_size = nb.second;
        }
        char ch = buf[idx];
        switch(ch)
        {
            case ':':
                parsingName = false;
                new_lines_in_a_raw = 0;
                break;
            case '=':
                parsingName = false;
                new_lines_in_a_raw = 0;
                break;
            case '\r':
                __add_to_headers(name, value, _headers);
                parsingName = true;
                break;
            case '\n':
                __add_to_headers(name, value, _headers);
                parsingName = true;
                ++new_lines_in_a_raw;
                if (new_lines_in_a_raw >= 2) parsing = false;
                break;
            case ';':
                __add_to_headers(name, value, _headers);
                parsingName = true;
                new_lines_in_a_raw = 0;
                break;
            default:
                if (parsingName) name.push_back(ch);
                else value.push_back(ch);
                new_lines_in_a_raw = 0;
                break;
        }
    }
    if (idx < buf_size)
    {
        _next_ptr = buf+idx;
        _next_size = buf_size-idx;
    }
    else
    {
        _next_ptr = nullptr;
        _next_size = 0;
    }
}

} // end of servlet namespace
