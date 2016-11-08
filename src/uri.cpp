/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <servlet/uri.h>
#include "string.h"

namespace servlet
{

const std::vector<std::pair<std::string, uint16_t>> URI::DEFAULT_PORTS =
        {
                {"http",     80  },
                {"https",    443 },
                {"ftp",      21  },
                {"ldap",     389 },
                {"nntp",     119 },
                {"gopher",   70  },
                {"imap",     143 },
                {"pop",      110 },
                {"snews",    563 },
                {"sip",      5060},
                {"rtsp",     554 },
                {"wais",     210 },
                {"z39.50r",  210 },
                {"z39.50s",  210 },
                {"prospero", 191 },
                {"nfs",      2049},
                {"tip",      3372},
                {"acap",     674 },
                {"telnet",   23  },
                {"ssh",      22  }
        };

uint16_t URI::get_default_port(string_view scheme)
{
    for (auto &entry : DEFAULT_PORTS) if (entry.first == scheme) return entry.second;
    return 0;
}

URI::URI(string_view scheme, string_view userInfo, string_view host, uint16_t port,
         string_view path, string_view query, string_view fragment)
{
    _port_i = port;
    std::string port_str;
    if (port > 0) port_str << _port_i;
    _initialize(scheme, userInfo, host, port_str, path, query, fragment);
}

void URI::_initialize(string_view scheme, string_view user_info, string_view host, string_view port,
                      string_view path, string_view query, string_view fragment)
{
    _uri.clear();
    _normalized = false;
    if (!scheme.empty()) _uri.append(scheme.data(), scheme.length());

    if (!user_info.empty() || !host.empty() || !port.empty())
    {
        if (!scheme.empty()) _uri.append("://");

        if (!user_info.empty())
        {
            _uri.append(user_info.data(), user_info.length());
            _uri.append(1, '@');
        }

        if (!host.empty()) _uri.append(host.data(), host.length());
        else throw uri_builder_error{"Host expected"};

        if (!port.empty())
        {
            _uri.append(1, ':');
            _uri.append(port.data(), port.length());
        }
    }
    else
    {
        if (!scheme.empty())
        {
            if (!path.empty() || !query.empty() || !fragment.empty()) _uri.append(1, ':');
            else throw uri_builder_error{"Path or query of fragment expected"};
        }
    }

    if (!path.empty())
    {
        /* if the URI is hierarchical and the path is not already prefixed with a '/', add one. */
        if (!host.empty() && path.front() != '/') _uri.append(1, '/').append(path.data(), path.length());
        else _uri.append(path.data(), path.length());
    }

    if (!query.empty()) _uri.append(1, '?').append(query.data(), query.length());

    if (!fragment.empty()) _uri.append(1, '#').append(fragment.data(), fragment.length());

    _uri_view = string_view(_uri);

    auto it = std::begin(_uri_view);
    if (!scheme.empty())
    {
        _scheme = string_view{&*it, scheme.size()};
        it += scheme.size();
        /* ignore ':' and "://" */
        if (*it == ':') ++it;
        if (*it == '/')
        {
            ++it;
            if (*it == '/') ++it;
        }
    }

    if (!user_info.empty())
    {
        _user_info = string_view{&*it, user_info.length()};
        it += user_info.length();
        ++it;  /* ignore '@' */
    }
    if (!host.empty())
    {
        _host = string_view{&*it, host.length()};
        it += host.length();
    }
    if (!port.empty())
    {
        ++it;  /* ignore ':' */
        _port = string_view{&*it, port.length()};
        it += port.length();
        _port >> _port_i;
    }
    if (!path.empty())
    {
        _path = string_view{&*it, path.length()};
        it += path.length();
    }
    if (!query.empty())
    {
        ++it;  /* ignore '?' */
        _query = string_view{&*it, query.length()};
        it += query.length();
    }
    if (!fragment.empty())
    {
        ++it;  /* ignore '#' */
        _fragment = string_view{&*it, fragment.length()};
    }
}

void URI::_move_parts(const URI& other)
{
    if (!other._scheme.empty())
    {
        _scheme = _uri_view.substr(other._scheme.begin()-other._uri_view.begin(), other._scheme.length());
    }
    if (!other._user_info.empty())
    {
        _user_info = _uri_view.substr(other._user_info.begin()-other._uri_view.begin(), other._user_info.length());
    }
    if (!other._host.empty())
    {
        _host = _uri_view.substr(other._host.begin()-other._uri_view.begin(), other._host.length());
    }
    if (!other._port.empty())
    {
        _port = _uri_view.substr(other._port.begin()-other._uri_view.begin(), other._port.length());
    }
    _port_i = other._port_i;
    if (!other._path.empty())
    {
        _path = _uri_view.substr(other._path.begin()-other._uri_view.begin(), other._path.length());
    }
    if (!other._query.empty())
    {
        _query = _uri_view.substr(other._query.begin()-other._uri_view.begin(), other._query.length());
    }
    if (!other._fragment.empty())
    {
        _fragment = _uri_view.substr(other._fragment.begin()-other._uri_view.begin(), other._fragment.length());
    }
}

URI &URI::operator=(const URI& other)
{
    _uri = other._uri;
    _uri_view = string_view{_uri};
    _move_parts(other);
    return *this;
}

URI &URI::operator=(URI&& other)
{
    _uri = std::move(other._uri);
    _uri_view = string_view{_uri};
    _move_parts(other);
    return *this;
}

void URI::swap(URI &other) noexcept
{
    _uri.swap(other._uri);
    _uri_view.swap(other._uri_view);
    _scheme.swap(other._scheme);
    _user_info.swap(other._user_info);
    _host.swap(other._host);
    _port.swap(other._port);
    auto tmp_port = other._port_i; other._port_i = _port_i; _port_i = tmp_port;
    _path.swap(other._path);
    _query.swap(other._query);
    _fragment.swap(other._fragment);
}

bool URI::has_authority() const noexcept { return !_host.empty(); }

bool URI::is_opaque() const noexcept
{
    if (_scheme.empty() || !_path.empty() || !_query.empty() || (_uri_view.end()-_scheme.end()) < 2) return false;
    return _uri_view[(_scheme.begin()-_uri_view.begin())+_scheme.length()+2] != '/';
}

URI::string_view URI::authority() const noexcept
{
    if (_host.empty()) return string_view{};

    auto host = this->host();

    auto user_info = string_view{};
    if (!_user_info.empty()) user_info = this->user_info();

    auto port = string_view{};
    if (!_port.empty()) port = this->port_view();

    auto first = std::begin(host), last = std::end(host);
    if (!user_info.empty() && !user_info.empty()) first = std::begin(user_info);
    else if (host.empty() && !_port.empty() && !port.empty())
    {
        first = std::begin(port);
        --first; /* include ':' before port */
    }

    if (host.empty())
    {
        if (_port.empty() && !port.empty()) last = std::end(port);
        else if (!_user_info.empty() && !user_info.empty())
        {
            last = std::end(user_info);
            ++last; /* include '@' */
        }
    }
    else if (!_port.empty())
    {
        if (port.empty()) ++last; /* include ':' after host */
        else last = std::end(port);
    }

    return string_view(first, last-first);
}

void _resize_part(std::size_t offset, URI::string_view& part, const URI::string_view& from_uri_view,
                  const URI::string_view& to_uri_view, int_fast16_t resize_bytes)
{
    if (part.empty()) return;
    std::size_t start_pos = part.begin() - from_uri_view.begin();
    URI::string_view::size_type length = part.length();
    if (start_pos > offset) part = URI::string_view{to_uri_view.data()+start_pos+resize_bytes, length};
    else if (start_pos <= offset && start_pos+length > offset)
    {
        part = URI::string_view{to_uri_view.data()+start_pos, length+resize_bytes};
    }
    else part = URI::string_view{to_uri_view.data()+start_pos, length};
}

void URI::_resize_parts(std::size_t offset, int_fast16_t resize_bytes)
{
    if (resize_bytes == 0) return;
    string_view new_uri_view{_uri};
    _resize_part(offset, _scheme, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _user_info, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _host, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _port, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _path, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _query, _uri_view, new_uri_view, resize_bytes);
    _resize_part(offset, _fragment, _uri_view, new_uri_view, resize_bytes);
    _uri_view = new_uri_view;
}

void URI::set_scheme(string_view scheme)
{
    int_fast16_t resize_bytes = scheme.length() - _scheme.length();
    std::size_t offset = _scheme.begin() - _uri_view.begin();
    _uri.replace(offset, _scheme.length(), scheme.data(), scheme.length());
    if (resize_bytes == 0) return;
    if (_scheme.empty())
    {
        _uri.replace(offset + scheme.length(), 0, "://", 3);
        resize_bytes += 3;
    }
    else if (scheme.empty())
    {
        int_fast16_t colon_slash_pos = 0;
        if (_uri[offset] == ':')
        {
            ++colon_slash_pos;
            if (_uri[offset+1] == '/')
            {
                ++colon_slash_pos;
                if (_uri[offset+1] == '/') ++colon_slash_pos;
            }
        }
        if (colon_slash_pos > 0)
        {
            _uri.erase(offset, static_cast<std::size_t>(colon_slash_pos));
            resize_bytes -= colon_slash_pos;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+offset, scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin())+resize_bytes, _user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin())+resize_bytes, _host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin())+resize_bytes, _port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin())+resize_bytes, _path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin())+resize_bytes, _query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::set_user_info(string_view user_info)
{
    int_fast16_t resize_bytes = user_info.length() - _user_info.length();
    std::size_t offset = _user_info.begin() - _uri_view.begin();
    _uri.replace(offset, _user_info.length(), user_info.data(), user_info.length());
    if (resize_bytes == 0) return;
    if (_user_info.empty())
    {
        _uri.replace(offset + user_info.length(), 0, 1, '@');
        ++resize_bytes;
    }
    else if (user_info.empty())
    {
        if (_uri[offset] == '@')
        {
            _uri.erase(offset, 1);
            --resize_bytes;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+offset, user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin())+resize_bytes, _host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin())+resize_bytes, _port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin())+resize_bytes, _path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin())+resize_bytes, _query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::set_host(string_view host)
{
    int_fast16_t resize_bytes = host.length() - _host.length();
    std::size_t offset = _host.begin() - _uri_view.begin();
    _uri.replace(offset, _host.length(), host.data(), host.length());
    if (resize_bytes == 0) return;
    if (_host.empty())
    {
        if (!_port.empty() && _uri[offset+host.length()] != ':')
        {
            _uri.replace(offset + host.length(), 0, 1, ':');
            ++resize_bytes;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin()), _user_info.length()};
    _host = string_view{new_uri_view.data()+offset, host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin())+resize_bytes, _port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin())+resize_bytes, _path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin())+resize_bytes, _query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::set_port(string_view port)
{
    uint16_t new_port_i;
    if (!(port >> new_port_i).empty()) throw uri_syntax_error{"Failed to parse port '" + port + "'"};
    _port_i = new_port_i;
    int_fast16_t resize_bytes = port.length() - _port.length();
    std::size_t offset = _port.begin() - _uri_view.begin();
    _uri.replace(offset, _port.length(), port.data(), port.length());
    if (resize_bytes == 0) return;
    if (_port.empty())
    {
        _uri.replace(offset, 0, 1, ':');
        ++offset; ++resize_bytes;
    }
    else if (port.empty())
    {
        if (offset > 0 && _uri[offset-1] == ':')
        {
            _uri.erase(offset-1, 1);
            --offset; --resize_bytes;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin()), _user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin()), _host.length()};
    _port = string_view{new_uri_view.data()+offset, port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin())+resize_bytes, _path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin())+resize_bytes, _query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::set_port(uint16_t port) { if (port <= 0) set_port(""); else set_port(std::string{} << port); }

void URI::set_path(string_view path)
{
    if (!path.empty() && path.front() != '/') throw uri_syntax_error{"Invalid path: '" + path + "'"};
    int_fast16_t resize_bytes = path.length() - _path.length();
    std::size_t offset = _path.begin() - _uri_view.begin();
    _uri.replace(offset, _path.length(), path.data(), path.length());
    if (resize_bytes == 0) return;
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin()), _user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin()), _host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin()), _port.length()};
    _path = string_view{new_uri_view.data()+offset, path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin())+resize_bytes, _query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::add_to_query(string_view name, string_view value)
{
    std::string query;
    if (_query.empty()) query.reserve(name.length() + value.length() + 1);
    else
    {
        query.reserve(_query.length() + name.length() + value.length() + 2);
        query.append(_query.data(), _query.length()).append(1, '&');
    }
    query.append(name.data(), name.length()).append(1, '=').append(value.data(), value.length());
    set_query(query);
}

void URI::parse_query(string_view str, std::function<void(const std::string& , const std::string& )> consumer)
{
    if (str.empty()) return;
    tokenizer tok{str, "&"};
    for (auto &&token : tok)
    {
        auto ind = token.find('=');
        if (ind == string_view::npos || ind == token.length()-1)
        {
            /* Name only */
            consumer(decode(ind == string_view::npos ? token : token.substr(0, ind)), {});
        }
        else consumer(decode(token.substr(0, ind)), decode(token.substr(ind+1)));
    }
}

void URI::set_query(string_view query)
{
    int_fast16_t resize_bytes = query.length() - _query.length();
    std::size_t offset = _query.begin() - _uri_view.begin();
    _uri.replace(offset, _query.length(), query.data(), query.length());
    if (resize_bytes == 0) return;
    if (_query.empty())
    {
        if (offset == 0 || _uri[offset-1] != '?')
        {
            _uri.replace(offset, 0, 1, '?');
            ++offset; ++resize_bytes;
        }
    }
    else if (query.empty())
    {
        if (offset > 0 && _uri[offset-1] == '?')
        {
            _uri.erase(offset-1, 1);
            --offset; --resize_bytes;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin()), _user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin()), _host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin()), _port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin()), _path.length()};
    _query = string_view{new_uri_view.data()+offset, query.length()};
    _fragment = string_view{new_uri_view.data()+(_fragment.begin()-_uri_view.begin())+resize_bytes, _fragment.length()};
    _uri_view = new_uri_view;
}

void URI::set_fragment(string_view fragment)
{
    int_fast16_t resize_bytes = fragment.length() - _fragment.length();
    std::size_t offset = _fragment.begin() - _uri_view.begin();
    _uri.replace(offset, _fragment.length(), fragment.data(), fragment.length());
    if (resize_bytes == 0) return;
    if (_fragment.empty())
    {
        if (offset == 0 || _uri[offset-1] != '#')
        {
            _uri.replace(offset, 0, 1, '#');
            ++offset;
        }
    }
    else if (fragment.empty())
    {
        if (offset > 0 && _uri[offset-1] == '#')
        {
            _uri.erase(offset-1, 1);
            --offset;
        }
    }
    string_view new_uri_view{_uri};
    _scheme = string_view{new_uri_view.data()+(_scheme.begin()-_uri_view.begin()), _scheme.length()};
    _user_info = string_view{new_uri_view.data()+(_user_info.begin()-_uri_view.begin()), _user_info.length()};
    _host = string_view{new_uri_view.data()+(_host.begin()-_uri_view.begin()), _host.length()};
    _port = string_view{new_uri_view.data()+(_port.begin()-_uri_view.begin()), _port.length()};
    _path = string_view{new_uri_view.data()+(_path.begin()-_uri_view.begin()), _path.length()};
    _query = string_view{new_uri_view.data()+(_query.begin()-_uri_view.begin()), _query.length()};
    _fragment = string_view{new_uri_view.data()+offset, fragment.length()};
    _uri_view = new_uri_view;
}

static bool _tokenize_path(URI::string_view path, std::vector<URI::string_view>& tokens)
{
    tokenizer t{path, "/"};
    bool has_removable_tokens = false;
    for (const auto &token : t)
    {
        tokens.push_back(token);
        if (!has_removable_tokens && (token == "." || token == "..")) has_removable_tokens = true;
    }
    if (!has_removable_tokens) return false;
    for (int ind = 0; ind < tokens.size(); ++ind)
    {
        if (tokens[ind] == ".") tokens[ind] = string_view{};
        else if (tokens[ind] == "..")
        {
            int back_ind = ind-1;
            while (back_ind >= 0)
            {
                if (!tokens[back_ind].empty())
                {
                    if (tokens[back_ind] != "..") tokens[ind] = string_view{};
                    break;
                }
                --back_ind;
            }
            tokens[back_ind] = string_view{};
        }
    }
    return true;
}

static std::unique_ptr<std::string> _normalize_path(URI::string_view path)
{
    std::vector<string_view> path_tokens;
    if (!_tokenize_path(path, path_tokens)) return {};
    bool front_slash = path.front() == '/';
    bool end_slash = path.back() == '/';
    std::unique_ptr<std::string> tmp_path{new std::string{}};
    for (auto &token : path_tokens)
    {
        if (!token.empty())
        {
            if (front_slash) tmp_path->append(1, '/');
            tmp_path->append(token.data(), token.length());
            front_slash = true;
        }
    }
    if (end_slash) tmp_path->append(1, '/');
    return tmp_path;
}

void URI::normalize_path()
{
    std::vector<string_view> path_tokens;
    if (!_tokenize_path(_path, path_tokens)) return;
    bool end_slash = _path.back() == '/';
    bool did_skip = false;
    std::size_t current_pos = _path.begin() - _uri_view.begin();
    std::size_t new_size = current_pos;
    for (auto &token : path_tokens)
    {
        if (!token.empty())
        {
            if (!did_skip) current_pos += token.length()+1;
            else
            {
                _uri[current_pos] = '/';
                ++current_pos;
                /* We use std::copy instead of std::string::replace because the ranges can overlap and replace
                 * doesn't regulate this situation and might not work correctly with all the std lib implementations */
                std::copy(token.begin(), token.end(), _uri.begin()+current_pos);
                current_pos += token.length();
            }
        }
        else did_skip = true;
    }
    if (end_slash)
    {
        _uri[current_pos] = '/';
        ++current_pos;
    }
    /* After path normalization string can only get shorter, thus no allocation and copying possible */
    new_size = current_pos - new_size;
    _uri.erase(_path.begin()-_uri_view.begin()+new_size, _path.length() - new_size);
    _resize_parts(_path.begin()-_uri_view.begin(), new_size-_path.length());
}

void URI::normalize()
{
    if (_normalized) return;
    /* All alphabetic characters in the scheme and host are lower-case... */
    if (!_scheme.empty())
    {
        std::string::iterator first = _uri.begin() + (_scheme.begin()-_uri_view.begin());
        std::string::iterator last = first + _scheme.length();
        std::transform(first, last, first, [](char ch) { return std::tolower(ch, std::locale()); });
    }

    /* ...except when used in percent encoding */ // Why do we do this if next step we decode the whole URI?
//    std::for_each(_uri.begin(), _uri.end(), percent_encoded_to_upper{});

    /* We are trying here to decode chars without the need to parse it later again. */
    _decode_encoded_unreserved_chars();
    normalize_path();
    _normalized = true;
}

URI URI::create_normalized() const
{
    URI uri{*this};
    uri.normalize();
    return uri;
}
URI URI::create_with_normalized_path() const
{
    URI uri{*this};
    uri.normalize_path();
    return uri;
}

static std::string _relativize_path(URI::string_view base, URI::string_view relative)
{
    std::vector<string_view> base_tokens;
    std::vector<string_view> relative_tokens;
    _tokenize_path(base, base_tokens);
    _tokenize_path(relative, relative_tokens);
    std::size_t up_levels = 0;
    auto base_it = base_tokens.begin();
    auto relative_it = relative_tokens.begin();
    while (true)
    {
        while (base_it != base_tokens.end() && (*base_it).empty()) ++base_it;
        while (relative_it != relative_tokens.end() && (*relative_it).empty()) ++relative_it;
        if (base_it == base_tokens.end()) break;
        else if (relative_it == relative_tokens.end() || *base_it != *relative_it)
        {
            do { if (!(*base_it).empty()) ++up_levels; ++base_it; } while (base_it != base_tokens.end());
            break;
        }
        ++base_it; ++relative_it;
    }
    if (up_levels > 0 && base.back() != '/') --up_levels;
    std::string res;
    bool set_slash = up_levels > 0;
    while (up_levels > 0)
    {
        res.append(2, '.');
        --up_levels;
        if (up_levels > 0) res.append(1, '/');
    }
    if (relative_it == relative_tokens.end()) return res;
    do
    {
        if (!(*relative_it).empty())
        {
            if (set_slash) res.append(1, '/');
            res.append((*relative_it).data(), (*relative_it).length());
            set_slash = true;
        }
        ++relative_it;
    }
    while (relative_it != relative_tokens.end());
    if (relative.back() == '/') res.append(1, '/');
    return res;
}

URI URI::relativize(const URI &other) const
{
    if (is_opaque() || other.is_opaque()) return other;
    if (_scheme.empty() || other._scheme.empty() || !equal_ic(_scheme, other._scheme)) return other;
    if (!has_authority() || !other.has_authority() || authority() != other.authority()) return other;
    if (_path.empty() || other._path.empty()) return other;

    return {string_view{}, string_view{}, string_view{}, 0,
            _relativize_path(_path, other._path), other._query, other._fragment};
}

static std::string _resolve_path(URI::string_view base, URI::string_view child)
{
    URI::string_view::size_type i = base.rfind('/');
    URI::string_view::size_type cn = child.length();
    if (cn == 0)
    {
        /* 5.2 (6a) */
        if (i >= 0)
        {
            URI::string_view path = base.substr(0, i + 1);
            /* 5.2 (6c-f) */
            std::unique_ptr<std::string> normalized = _normalize_path(path);
            return static_cast<bool>(normalized) ? std::move(*normalized) : path.to_string();
        }
    }
    else
    {
        std::string sb;
        sb.reserve(base.length() + cn);
        /* 5.2 (6a) */
        if (i >= 0) sb.append(base.data(), i + 1);
        /* 5.2 (6b) */
        sb.append(child.data(), cn);
        /* 5.2 (6c-f) */
        std::unique_ptr<std::string> normalized = _normalize_path(sb);
        return static_cast<bool>(normalized) ? std::move(*normalized) : sb;
    }
    /* 5.2 (6g): If the result is absolute but the path begins with "../",
     * then we simply leave the path as-is */

    return std::string{}; /* Only empty path is left here */
}

/* RFC2396 5.2 */
URI URI::resolve(const URI &uri) const
{
    /* check if child if opaque first so that NPE is thrown if child is null. */
    if (uri.is_absolute() && !uri.is_opaque()) return uri;
    if (is_opaque()) return uri;

    /* 5.2 (2): Reference to current document (lone fragment) */
    if (uri._scheme.empty() && !uri.has_authority() && uri._path.empty()
        && !uri._fragment.empty() && uri._query.empty())
    {
        if (!_fragment.empty() && uri._fragment == _fragment) return *this;
        URI ru;
        ru._initialize(_scheme, _user_info, _host, _port, _path, _query, uri._fragment);
        return ru;
    }
    /* 5.2 (3): Child is absolute */
    if (!uri._scheme.empty()) return uri;

    URI ru;		/* Resolved URI */
    string_view scheme = _scheme;
    string_view query = uri._query;
    string_view fragment = uri._fragment;
    string_view host = _host;
    string_view user_info = _user_info;
    string_view port = _port;
    string_view path = uri._path;
    string_type path_str;

    /* 5.2 (4): Authority */
    if (!uri.has_authority())
    {
        if (uri._path.empty() || uri._path.front() != '/') /* 5.2 (6): Resolve relative path */
        {
            path_str = _resolve_path(_path, uri._path);
            path = path_str;
        }
        /* else 5.2 (5): Child path is absolute */
    }
    else
    {
        host = uri._host;
        user_info = uri._user_info;
        host = uri._host;
        port = uri._port;
    }

    /* 5.2 (7): Recombine (nothing to do here) */
    ru._initialize(scheme, user_info, host, port, path, query, fragment);
    return ru;
}

int URI::compare(const URI &other) const noexcept
{
    /* if both URIs are empty, then we should define them as equal
     * even though they're still invalid. */
    if (empty() && other.empty()) return 0;
    if (empty()) return -1;
    if (other.empty()) return 1;

    if (_normalized)
    {
        if (other._normalized) return _uri.compare(other._uri);
        URI other_copy{other};
        other_copy.normalize();
        return _uri.compare(other_copy._uri);
    }
    if (other._normalized)
    {
        URI this_copy{*this};
        this_copy.normalize();
        return this_copy._uri.compare(other._uri);
    }
    URI this_copy{*this};
    URI other_copy{other};
    this_copy.normalize();
    other_copy.normalize();
    return this_copy._uri.compare(other_copy._uri);
}

void URI::_initialize()
{
    _normalized = false;
    _uri_view = string_view{_uri};
    if (_uri.empty())
    {
        _scheme = string_view{_uri};
        _user_info = string_view{_uri};
        _host = string_view{_uri};
        _port = string_view{_uri};
        _path = string_view{_uri};
        _query = string_view{_uri};
        _fragment = string_view{_uri};
        return;
    }
    const_iterator it = std::begin(_uri_view), last = std::end(_uri_view);
    _parse(it, last);
}

} // end of servlet namespace
