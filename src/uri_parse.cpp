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

static const uint_fast32_t T_SLASH          = 0x01;     /* '/' */
static const uint_fast32_t T_QUESTION       = 0x02;     /* '?' */
static const uint_fast32_t T_HASH           = 0x04;     /* '#' */
static const uint_fast32_t T_AMP            = 0x08;     /* '&' */
static const uint_fast32_t T_ALPHA          = 0x10;     /* 'A' ... 'Z', 'a' ... 'z' */
static const uint_fast32_t T_DIGIT          = 0x20;     /* '0' ... '9' */
static const uint_fast32_t T_XDIGIT         = 0x40;     /* '0' ... '9' "abcdefABCDEF */
static const uint_fast32_t T_SCHEME_ADD     = 0x80;     /* "-+." (allowed in scheme except first char)*/
static const uint_fast32_t T_ESCAPE         = 0x100;    /* "$-_.+!*'"(),:@&=" */
static const uint_fast32_t T_UNRESERVED_ADD = 0x200;    /* "-._~" */
static const uint_fast32_t T_SUBDELIM       = 0x400;    /* "!$&'()*+,;=" */
static const uint_fast32_t T_PCHAR_ADD      = 0x800;    /* ":@" */
static const uint_fast32_t T_COLON          = 0x1000;   /* ":" */
static const uint_fast32_t T_SQBR           = 0x2000;   /* "[]" */
static const uint_fast32_t T_PATH           = 0x4000;   /* "/.@%;=" */
static const uint_fast32_t T_NUL            = 0x8000;   /* '\0' */

static const uint_fast32_t T_SCHEME         = T_SCHEME_ADD|T_ALPHA|T_DIGIT;
static const uint_fast32_t T_UNRESERVED     = T_UNRESERVED_ADD|T_ALPHA|T_DIGIT;
static const uint_fast32_t T_PCHAR          = T_PCHAR_ADD|T_UNRESERVED|T_SUBDELIM;

/* Delimiter table for the ASCII character set */
static const uint_fast32_t CHAR_MAP[256] =
        {
                T_NUL,                                               /* 0x00     */
                0,                                                   /* 0x01     */
                0,                                                   /* 0x02     */
                0,                                                   /* 0x03     */
                0,                                                   /* 0x04     */
                0,                                                   /* 0x05     */
                0,                                                   /* 0x06     */
                0,                                                   /* 0x07     */
                0,                                                   /* 0x08     */
                0,                                                   /* 0x09     */
                0,                                                   /* 0x0a '\n'*/
                0,                                                   /* 0x0b     */
                0,                                                   /* 0x0c     */
                0,                                                   /* 0x0d     */
                0,                                                   /* 0x0e     */
                0,                                                   /* 0x0f     */
                0,                                                   /* 0x10     */
                0,                                                   /* 0x11     */
                0,                                                   /* 0x12     */
                0,                                                   /* 0x13     */
                0,                                                   /* 0x14     */
                0,                                                   /* 0x15     */
                0,                                                   /* 0x16     */
                0,                                                   /* 0x17     */
                0,                                                   /* 0x18     */
                0,                                                   /* 0x19     */
                0,                                                   /* 0x1a     */
                0,                                                   /* 0x1b     */
                0,                                                   /* 0x1c     */
                0,                                                   /* 0x1d     */
                0,                                                   /* 0x1e     */
                0,                                                   /* 0x1f     */
                0,                                                   /* 0x20 ' ' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x21 '!' */
                T_ESCAPE,                                            /* 0x22 '"' */
                T_HASH,                                              /* 0x23 '#' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x24 '$' */
                T_PATH,                                              /* 0x25 '%' */
                T_ESCAPE | T_SUBDELIM | T_AMP,                       /* 0x26 '&' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x27 ''' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x28 '(' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x29 ')' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x2a '*' */
                T_SCHEME_ADD | T_ESCAPE | T_SUBDELIM,                /* 0x2b '+' */
                T_ESCAPE | T_SUBDELIM,                               /* 0x2c ',' */
                T_SCHEME_ADD | T_ESCAPE | T_UNRESERVED_ADD,          /* 0x2d '-' */
                T_SCHEME_ADD | T_ESCAPE | T_UNRESERVED_ADD | T_PATH, /* 0x2e '.' */
                T_SLASH | T_PATH,                                    /* 0x2f '/' */
                T_DIGIT | T_XDIGIT,                                  /* 0x30 '0' */
                T_DIGIT | T_XDIGIT,                                  /* 0x31 '1' */
                T_DIGIT | T_XDIGIT,                                  /* 0x32 '2' */
                T_DIGIT | T_XDIGIT,                                  /* 0x33 '3' */
                T_DIGIT | T_XDIGIT,                                  /* 0x34 '4' */
                T_DIGIT | T_XDIGIT,                                  /* 0x35 '5' */
                T_DIGIT | T_XDIGIT,                                  /* 0x36 '6' */
                T_DIGIT | T_XDIGIT,                                  /* 0x37 '7' */
                T_DIGIT | T_XDIGIT,                                  /* 0x38 '8' */
                T_DIGIT | T_XDIGIT,                                  /* 0x39 '9' */
                T_ESCAPE | T_PCHAR_ADD | T_COLON,                    /* 0x3a ':' */
                T_SUBDELIM | T_PATH,                                 /* 0x3b ';' */
                0,                                                   /* 0x3c '<' */
                T_ESCAPE | T_SUBDELIM | T_PATH,                      /* 0x3d '=' */
                0,                                                   /* 0x3e '>' */
                T_QUESTION,                                          /* 0x3f '?' */
                T_ESCAPE | T_PCHAR_ADD | T_PATH,                     /* 0x40 '@' */
                T_ALPHA | T_XDIGIT,                                  /* 0x41 'A' */
                T_ALPHA | T_XDIGIT,                                  /* 0x42 'B' */
                T_ALPHA | T_XDIGIT,                                  /* 0x43 'C' */
                T_ALPHA | T_XDIGIT,                                  /* 0x44 'D' */
                T_ALPHA | T_XDIGIT,                                  /* 0x45 'E' */
                T_ALPHA | T_XDIGIT,                                  /* 0x46 'F' */
                T_ALPHA,                                             /* 0x47 'G' */
                T_ALPHA,                                             /* 0x48 'H' */
                T_ALPHA,                                             /* 0x49 'I' */
                T_ALPHA,                                             /* 0x4a 'J' */
                T_ALPHA,                                             /* 0x4b 'K' */
                T_ALPHA,                                             /* 0x4c 'L' */
                T_ALPHA,                                             /* 0x4d 'M' */
                T_ALPHA,                                             /* 0x4e 'N' */
                T_ALPHA,                                             /* 0x4f 'O' */
                T_ALPHA,                                             /* 0x50 'P' */
                T_ALPHA,                                             /* 0x51 'Q' */
                T_ALPHA,                                             /* 0x52 'R' */
                T_ALPHA,                                             /* 0x53 'S' */
                T_ALPHA,                                             /* 0x54 'T' */
                T_ALPHA,                                             /* 0x55 'U' */
                T_ALPHA,                                             /* 0x56 'V' */
                T_ALPHA,                                             /* 0x57 'W' */
                T_ALPHA,                                             /* 0x58 'X' */
                T_ALPHA,                                             /* 0x59 'Y' */
                T_ALPHA,                                             /* 0x5a 'Z' */
                T_SQBR,                                              /* 0x5b '[' */
                0,                                                   /* 0x5c '\' */
                T_SQBR,                                              /* 0x5d ']' */
                0,                                                   /* 0x5e '^' */
                T_ESCAPE | T_UNRESERVED_ADD,                         /* 0x5f '_' */
                0,                                                   /* 0x60 '`' */
                T_ALPHA | T_XDIGIT,                                  /* 0x61 'a' */
                T_ALPHA | T_XDIGIT,                                  /* 0x62 'b' */
                T_ALPHA | T_XDIGIT,                                  /* 0x63 'c' */
                T_ALPHA | T_XDIGIT,                                  /* 0x64 'd' */
                T_ALPHA | T_XDIGIT,                                  /* 0x65 'e' */
                T_ALPHA | T_XDIGIT,                                  /* 0x66 'f' */
                T_ALPHA,                                             /* 0x67 'g' */
                T_ALPHA,                                             /* 0x68 'h' */
                T_ALPHA,                                             /* 0x69 'i' */
                T_ALPHA,                                             /* 0x6a 'j' */
                T_ALPHA,                                             /* 0x6b 'k' */
                T_ALPHA,                                             /* 0x6c 'l' */
                T_ALPHA,                                             /* 0x6d 'm' */
                T_ALPHA,                                             /* 0x6e 'n' */
                T_ALPHA,                                             /* 0x6f 'o' */
                T_ALPHA,                                             /* 0x70 'p' */
                T_ALPHA,                                             /* 0x71 'q' */
                T_ALPHA,                                             /* 0x72 'r' */
                T_ALPHA,                                             /* 0x73 's' */
                T_ALPHA,                                             /* 0x74 't' */
                T_ALPHA,                                             /* 0x75 'u' */
                T_ALPHA,                                             /* 0x76 'v' */
                T_ALPHA,                                             /* 0x77 'w' */
                T_ALPHA,                                             /* 0x78 'x' */
                T_ALPHA,                                             /* 0x79 'y' */
                T_ALPHA,                                             /* 0x7a 'z' */
                0,                                                   /* 0x7b '{' */
                0,                                                   /* 0x7c '|' */
                0,                                                   /* 0x7d '}' */
                T_UNRESERVED_ADD,                                    /* 0x7e '~' */
                0,                                                   /* 0x7f     */
                0,                                                   /* 0x80     */
                0,                                                   /* 0x81     */
                0,                                                   /* 0x82     */
                0,                                                   /* 0x83     */
                0,                                                   /* 0x84     */
                0,                                                   /* 0x85     */
                0,                                                   /* 0x86     */
                0,                                                   /* 0x87     */
                0,                                                   /* 0x88     */
                0,                                                   /* 0x89     */
                0,                                                   /* 0x8a     */
                0,                                                   /* 0x8b     */
                0,                                                   /* 0x8c     */
                0,                                                   /* 0x8d     */
                0,                                                   /* 0x8e     */
                0,                                                   /* 0x8f     */
                0,                                                   /* 0x90     */
                0,                                                   /* 0x91     */
                0,                                                   /* 0x92     */
                0,                                                   /* 0x93     */
                0,                                                   /* 0x94     */
                0,                                                   /* 0x95     */
                0,                                                   /* 0x96     */
                0,                                                   /* 0x97     */
                0,                                                   /* 0x98     */
                0,                                                   /* 0x99     */
                0,                                                   /* 0x9a     */
                0,                                                   /* 0x9b     */
                0,                                                   /* 0x9c     */
                0,                                                   /* 0x9d     */
                0,                                                   /* 0x9e     */
                0,                                                   /* 0x9f     */
                0,                                                   /* 0xa0     */
                0,                                                   /* 0xa1     */
                0,                                                   /* 0xa2     */
                0,                                                   /* 0xa3     */
                0,                                                   /* 0xa4     */
                0,                                                   /* 0xa5     */
                0,                                                   /* 0xa6     */
                0,                                                   /* 0xa7     */
                0,                                                   /* 0xa8     */
                0,                                                   /* 0xa9     */
                0,                                                   /* 0xaa     */
                0,                                                   /* 0xab     */
                0,                                                   /* 0xac     */
                0,                                                   /* 0xad     */
                0,                                                   /* 0xae     */
                0,                                                   /* 0xaf     */
                0,                                                   /* 0xb0     */
                0,                                                   /* 0xb1     */
                0,                                                   /* 0xb2     */
                0,                                                   /* 0xb3     */
                0,                                                   /* 0xb4     */
                0,                                                   /* 0xb5     */
                0,                                                   /* 0xb6     */
                0,                                                   /* 0xb7     */
                0,                                                   /* 0xb8     */
                0,                                                   /* 0xb9     */
                0,                                                   /* 0xba     */
                0,                                                   /* 0xbb     */
                0,                                                   /* 0xbc     */
                0,                                                   /* 0xbd     */
                0,                                                   /* 0xbe     */
                0,                                                   /* 0xbf     */
                0,                                                   /* 0xc0     */
                0,                                                   /* 0xc1     */
                0,                                                   /* 0xc2     */
                0,                                                   /* 0xc3     */
                0,                                                   /* 0xc4     */
                0,                                                   /* 0xc5     */
                0,                                                   /* 0xc6     */
                0,                                                   /* 0xc7     */
                0,                                                   /* 0xc8     */
                0,                                                   /* 0xc9     */
                0,                                                   /* 0xca     */
                0,                                                   /* 0xcb     */
                0,                                                   /* 0xcc     */
                0,                                                   /* 0xcd     */
                0,                                                   /* 0xce     */
                0,                                                   /* 0xcf     */
                0,                                                   /* 0xd0     */
                0,                                                   /* 0xd1     */
                0,                                                   /* 0xd2     */
                0,                                                   /* 0xd3     */
                0,                                                   /* 0xd4     */
                0,                                                   /* 0xd5     */
                0,                                                   /* 0xd6     */
                0,                                                   /* 0xd7     */
                0,                                                   /* 0xd8     */
                0,                                                   /* 0xd9     */
                0,                                                   /* 0xda     */
                0,                                                   /* 0xdb     */
                0,                                                   /* 0xdc     */
                0,                                                   /* 0xdd     */
                0,                                                   /* 0xde     */
                0,                                                   /* 0xdf     */
                0,                                                   /* 0xe0     */
                0,                                                   /* 0xe1     */
                0,                                                   /* 0xe2     */
                0,                                                   /* 0xe3     */
                0,                                                   /* 0xe4     */
                0,                                                   /* 0xe5     */
                0,                                                   /* 0xe6     */
                0,                                                   /* 0xe7     */
                0,                                                   /* 0xe8     */
                0,                                                   /* 0xe9     */
                0,                                                   /* 0xea     */
                0,                                                   /* 0xeb     */
                0,                                                   /* 0xec     */
                0,                                                   /* 0xed     */
                0,                                                   /* 0xee     */
                0,                                                   /* 0xef     */
                0,                                                   /* 0xf0     */
                0,                                                   /* 0xf1     */
                0,                                                   /* 0xf2     */
                0,                                                   /* 0xf3     */
                0,                                                   /* 0xf4     */
                0,                                                   /* 0xf5     */
                0,                                                   /* 0xf6     */
                0,                                                   /* 0xf7     */
                0,                                                   /* 0xf8     */
                0,                                                   /* 0xf9     */
                0,                                                   /* 0xfa     */
                0,                                                   /* 0xfb     */
                0,                                                   /* 0xfc     */
                0,                                                   /* 0xfd     */
                0,                                                   /* 0xfe     */
                0                                                    /* 0xff     */
        };

static char HEX_CHARS[] = "0123456789ABCDEF";

static inline bool _is_pct_encoded(URI::string_view::iterator beg, URI::string_view::iterator end)
{
    if (beg == end || *beg != '%') return false;
    ++beg;
    if (beg == end || !(CHAR_MAP[static_cast<unsigned char>(*beg)] & T_XDIGIT)) return false;
    ++beg;
    return beg != end && (CHAR_MAP[static_cast<unsigned char>(*beg)] & T_XDIGIT) > 0;
}

static inline bool _is_pchar(URI::string_view::iterator beg, URI::string_view::iterator end)
{
    return beg != end && ((CHAR_MAP[static_cast<unsigned char>(*beg)] & T_PCHAR) > 0 || _is_pct_encoded(beg, end));
}

static inline void _encode(URI::string_view::iterator& beg, URI::string_view::iterator end,
                           uint_fast32_t allowed_mask, std::string& to)
{
    while (beg != end)
    {
        if (CHAR_MAP[static_cast<unsigned char>(*beg)] & allowed_mask) to.append(1, *beg);
        else if (_is_pct_encoded(beg, end))
        {
            to.append(beg, beg+3);
            beg += 2; /* another one will be added at the end of the loop */
        }
        else to.append(1, '%').append(1, HEX_CHARS[(*beg >> 4) & 0x0f]).append(1, HEX_CHARS[*beg & 0x0f]);
        ++beg;
    }
}

std::string URI::to_ASCII_string() const
{
    std::string ascii;
    ascii.reserve(_uri.length());
    auto it = _uri_view.begin();
    if (_scheme.begin() > it || !_scheme.empty()) { ascii.append(&*it, _scheme.end()-it); it = _scheme.end(); }
    if (_user_info.begin() > it) { ascii.append(&*it, _user_info.begin()-it); it = _user_info.begin(); }
    if (!_user_info.empty()) _encode(it, _user_info.end(), T_UNRESERVED|T_SUBDELIM|T_COLON, ascii);
    if (_host.begin() > it) { ascii.append(&*it, _host.begin()-it); it = _host.begin(); }
    if (!_host.empty())
    {
        if (_host.front() == '[' && _host.back() == ']') /* IPv6. We don't mess with it */
        {
            ascii.append(&*it, _host.end()-it); it = _host.end();
        }
        else _encode(it, _host.end(), T_UNRESERVED|T_SUBDELIM, ascii);
    }
    if (!_port.empty()) { ascii.append(&*it, _port.end()-it); it = _port.end(); }
    if (_path.begin() > it) { ascii.append(&*it, _path.begin()-it); it = _path.begin(); }
    if (!_path.empty()) _encode(it, _path.end(), T_PCHAR|T_SLASH, ascii);
    if (_query.begin() > it) { ascii.append(&*it, _query.begin()-it); it = _query.begin(); }
    if (!_query.empty()) _encode(it, _query.end(), T_PCHAR|T_SLASH|T_QUESTION, ascii);
    if (_fragment.begin() > it) { ascii.append(&*it, _fragment.begin()-it); it = _fragment.begin(); }
    if (!_fragment.empty()) _encode(it, _fragment.end(), T_PCHAR|T_SLASH|T_QUESTION, ascii);
    return ascii;
}

static int_fast16_t letter_to_hex(char in)
{
    if ((in >= '0') && (in <= '9')) return in - '0';
    if ((in >= 'a') && (in <= 'f')) return in + 10 - 'a';
    if ((in >= 'A') && (in <= 'F')) return in + 10 - 'A';
    throw uri_syntax_error{string_view{"Unable to decode character with symbol '"} + in + "'"};
}

template <typename ConstIterator>
static inline char percent_encode(ConstIterator it)
{
    if (*++it >= '8') return '\0';
    auto v0 = letter_to_hex(*it); ++it;
    auto v1 = letter_to_hex(*it);
    return static_cast<char>((0x10 * v0) + v1);
}

void URI::_decode_encoded_unreserved_chars()
{
    auto it = _uri.begin();
    auto it2 = it;
    while (it != _uri.end())
    {
        if (*it == '%')
        {
            const auto sfirst = it;
            const char opt_char = percent_encode(sfirst);
            if (opt_char == '\0')
            {
                throw uri_syntax_error{string_view{"Unable to decode characters outside the ASCII character set: '"}
                                       + (*++it) + (*++it) + "'"};
            }
            if (CHAR_MAP[static_cast<unsigned char>(opt_char)] & T_UNRESERVED)
            {
                *it2 = opt_char;
                /* We use here +1 iterator in order to ensure that the iterator is located
                 * totally within part's boundaries */
                auto it_offset  = it-_uri.begin();
                auto it2_offset = it2-_uri.begin();
                _uri.erase((it2-_uri.begin())+1, 2);
                _resize_parts(it2-_uri.begin()+1, -2);
                it = _uri.begin() + it_offset + 1;
                it2 = _uri.begin() + it2_offset + 1;
            }
            else *it2 = *it;
        }
        else *it2 = *it;
        ++it; ++it2;
    }
    if (it2 == _uri.end()) return;
    _uri.erase(it2, _uri.end());
}

std::string URI::decode(string_view str)
{
    if (str.empty()) return {};
    std::string res;
    res.reserve(str.length());
    auto it = str.begin();
    auto it2 = it;
    while (it != str.end())
    {
        if (*it == '%')
        {
            const char opt_char = percent_encode(it);
            if (opt_char != '\0')
            {
                res += opt_char;
                it += 2;
            }
            else res += *it;
        }
        else if (*it == '+') res += ' ';
        else res += *it;
        ++it; ++it2;
    }
    return res;
}

enum class uri_state
{
    none,
    scheme,
    hier_part,
    query,
    fragment
};
enum class hier_part_state
{
    first_slash,
    second_slash,
    authority,
    host,
    host_ipv6,
    port,
    path
};

inline static bool _advance(string_view::const_iterator &it, string_view::const_iterator last, uint_fast32_t mask)
{
    bool res = false;
    while (it != last && (CHAR_MAP[static_cast<unsigned char>(*it)] & mask)) ++it, res = true;
    return res;
}

static inline bool _skip_pct_encoded(string_view::const_iterator &it, string_view::const_iterator last)
{
    if (it == last || *it != '%') return false;

    string_view::const_iterator it_copy = it;
    ++it_copy;
    if (it_copy == last || !(CHAR_MAP[static_cast<unsigned char>(*it_copy)] & T_XDIGIT)) return false;
    ++it_copy;
    if (it_copy == last || !(CHAR_MAP[static_cast<unsigned char>(*it_copy)] & T_XDIGIT)) return false;
    ++it_copy;
    it = it_copy;
    return true;
}

static inline bool _skip_pchar(string_view::const_iterator &it, string_view::const_iterator last)
{
    if (it != last && (CHAR_MAP[static_cast<unsigned char>(*it)] & T_PCHAR))
    {
        ++it;
        return true;
    }
    return _skip_pct_encoded(it, last);
}

static bool _validate_scheme(string_view::const_iterator &it, string_view::const_iterator last)
{
    return _advance(it, last, T_SCHEME) && *it == ':';
}

static bool _validate_user_info(string_view::const_iterator it, string_view::const_iterator last)
{
    while (it != last)
    {
        bool res = _advance(it, last, T_UNRESERVED | T_SUBDELIM) || _skip_pct_encoded(it, last);
        if (it == last) return true;
        if (*it == ':')
        {
            ++it;
            return true;
        }
        else if (!res) return false;
    }
    return true;
}

static bool _validate_fragment(string_view::const_iterator &it, string_view::const_iterator last)
{
    while (it != last)
    {
        if (!_advance(it, last, T_PCHAR|T_SLASH|T_QUESTION) && !_skip_pct_encoded(it, last)) return false;
    }
    return true;
}

static inline bool _read_port(const URI::string_view& port_str, uint16_t& port)
{
    if (port_str.length() > 5) return false;
    uint32_t int_port = 0;
    if (!(port_str >> int_port).empty() || int_port >= std::numeric_limits<unsigned short>::max()) return false;
    port = static_cast<uint16_t>(int_port);
    return true;
}

bool URI::_set_host_and_port(string_view::const_iterator first,
                             string_view::const_iterator last,
                             string_view::const_iterator last_colon)
{
    if (first == last_colon || last_colon < first)
    {
        _host = string_view{&*first, static_cast<string_view::size_type>(last-first)};
        _port = string_view{&*last, 0};
        return true;
    }
    auto port_start = last_colon;
    ++port_start;
    _host = string_view{&*first, static_cast<string_view::size_type>(last_colon-first)};
    _port = string_view{&*port_start, static_cast<string_view::size_type>(last-port_start)};
    return _read_port(_port, _port_i);
}

void URI::_parse(string_view::const_iterator &it, string_view::const_iterator last)
{
    if (it == last) throw uri_syntax_error{"Empty URI"};

    string_view::const_iterator first = it;

    uri_state state = uri_state::hier_part;
    hier_part_state hp_state;
    if (!_validate_scheme(it, last))
    {
        /* OK. Let's try to read other stuff without scheme if it is empty */
        _scheme = string_view{_uri_view.data(), 0};
        if (it == first)
        {
            hp_state = hier_part_state::first_slash;
            if (*it == '?')
            {
                _user_info = string_view{_uri_view.data(), 0};
                _host = string_view{_uri_view.data(), 0};
                _port = string_view{_uri_view.data(), 0};
                _path = string_view{_uri_view.data(), 0};
                state = uri_state::query, ++it;
            }
            else if (*it == '#')
            {
                _user_info = string_view{_uri_view.data(), 0};
                _host = string_view{_uri_view.data(), 0};
                _port = string_view{_uri_view.data(), 0};
                _path = string_view{_uri_view.data(), 0};
                _query = string_view{_uri_view.data(), 0};
                state = uri_state::fragment, ++it;
            }
        }
        else if (*it == '/') /* Looks like we're reading path */
        {
            hp_state = hier_part_state::path;
        }
        else if (*it == '@') /* Authority? */
        {
            hp_state = hier_part_state::authority;
        }
    }
    else
    {
        _scheme = string_view{&*first, static_cast<string_view::size_type>(it-first)};
        ++it; /* move past the scheme delimiter */
        first = it;
        hp_state = hier_part_state::first_slash;
        if (it != last)
        {
            if (*it == '.') hp_state = hier_part_state::path; /* If it starts with "." or ".." we assume it is path */
            else if (*it == '/') hp_state = hier_part_state::first_slash;
            else hp_state = hier_part_state::authority; /* Not a slash nor dot - go for authority */
        }
    }

    /* this is used by the user_info/port */
    auto last_colon = first;
    while (it != last)
    {
        if (hp_state == hier_part_state::first_slash)
        {
            if (*it == '/')
            {
                hp_state = hier_part_state::second_slash;
                /* set the first iterator in case the second slash is not forthcoming */
                first = it;
                ++it;
                continue;
            }
            else
            {
                hp_state = hier_part_state::path;
                first = it;
            }
        }
        else if (hp_state == hier_part_state::second_slash)
        {
            if (*it == '/')
            {
                hp_state = hier_part_state::authority;
                ++it;
                first = it;
                continue;
            }
            else
            {
                /* it's a valid URI, and this is the beginning of the path */
                hp_state = hier_part_state::path;
                _user_info = string_view{&*(it-1), 0};
                _host = string_view{&*(it-1), 0};
                _port = string_view{&*(it-1), 0};
            }
        }
        else if (hp_state == hier_part_state::authority)
        {
            if (first == last || (CHAR_MAP[static_cast<unsigned char>(*first)] & T_PCHAR_ADD))
            {
                throw uri_syntax_error{"Failed to parse authority info '" + _uri + "': " + (first-_uri_view.begin())};
            }

            if (first == it) last_colon = first; /* reset the last colon */

            if (*it == '@')
            {
                if (!_validate_user_info(first, it))
                {
                    throw uri_syntax_error{"Failed to parse user info '" + _uri + "': " + (first-_uri_view.begin())};
                }
                _user_info = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                hp_state = hier_part_state::host;
                ++it;
                first = it;

                if (*first == '[') hp_state = hier_part_state::host_ipv6; // this is an IPv6 address
                continue;
            }
            else if (*it == '[') /* this is an IPv6 address */
            {
                _user_info = string_view{&*it, 0};
                hp_state = hier_part_state::host_ipv6;
                first = it;
                continue;
            }
            else if (*it == ':') last_colon = it;
            else if (*it == '/') /* we skipped right past the host and port, and are at the path. */
            {
                if (!_set_host_and_port(first, it, last_colon))
                {
                    throw uri_syntax_error{"Failed to parse host and port info '" + _uri
                                           + "': " + (first-_uri_view.begin())};
                }
                _user_info = string_view{&*first, 0};
                hp_state = hier_part_state::path;
                first = it;
                continue;
            }
            else if (*it == '?') /* the path is empty, but valid, and the next part is the query */
            {
                if (!_set_host_and_port(first, it, last_colon))
                {
                    throw uri_syntax_error{"Failed to parse host and port info '" + _uri
                                           + "': " + (first-_uri_view.begin())};
                }
                _user_info = string_view{&*it, 0};
                _path = string_view{&*it, 0};
                state = uri_state::query;
                ++it;
                first = it;
                break;
            }
            else if (*it == '#') /* the path is empty, but valid, and the next part is the fragment */
            {
                if (!_set_host_and_port(first, it, last_colon))
                {
                    throw uri_syntax_error{"Failed to parse host and port info '" + _uri
                                           + "': " + (first-_uri_view.begin())};
                }
                _user_info = string_view{&*it, 0};
                _path = string_view{&*it, 0};
                state = uri_state::fragment;
                ++it;
                first = it;
                break;
            }
        }
        else if (hp_state == hier_part_state::host)
        {
            if (*first == ':')
            {
                throw uri_syntax_error{"Unexpected symbol ':' '" + _uri + "': " + (first-_uri_view.begin())};
            }
            if (*it == ':')
            {
                _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (_user_info.empty()) _user_info = string_view{&*first, 0};
                hp_state = hier_part_state::port;
                ++it;
                first = it;
                continue;
            }
            else if (*it == '/')
            {
                _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (_user_info.empty()) _user_info = string_view{&*first, 0};
                _port = string_view{&*it, 0};
                hp_state = hier_part_state::path;
                first = it;
                continue;
            }
            else if (*it == '?') /* the path is empty, but valid, and the next part is the query */
            {
                _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (_user_info.empty()) _user_info = string_view{&*first, 0};
                _port = string_view{&*it, 0};
                _path = string_view{&*it, 0};
                state = uri_state::query;
                ++it;
                first = it;
                break;
            }
            else if (*it == '#') /* the path is empty, but valid, and the next part is the fragment */
            {
                _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (_user_info.empty()) _user_info = string_view{&*first, 0};
                _port = string_view{&*it, 0};
                _path = string_view{&*it, 0};
                state = uri_state::fragment;
                ++it;
                first = it;
                break;
            }
        }
        else if (hp_state == hier_part_state::host_ipv6)
        {
            if (*first != '[')
            {
                throw uri_syntax_error{"Expected symbol '[' in IPv6 portion '" + _uri
                                       + "': " + (first-_uri_view.begin())};
            }
            if (*it == ']')
            {
                ++it;
                /* Then test if the next part is a host, part, or the end of the file */
                if (it == last)
                {
                    if (!_set_host_and_port(first, last, last_colon))
                    {
                        throw uri_syntax_error{"Failed to set host and port '" + _uri
                                               + "': " + (first-_uri_view.begin())};
                    }
                    _path = string_view{&*last, 0};
                    _query = string_view{&*last, 0};
                    _fragment = string_view{&*last, 0};
                    state = uri_state::none;
                    break;
                }
                else if (*it == ':')
                {
                    _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                    if (_user_info.empty()) _user_info = string_view{&*first, 0};
                    hp_state = hier_part_state::port;
                    ++it;
                    first = it;
                }
                else if (*it == '/')
                {
                    _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                    _port = string_view{&*it, 0};
                    if (_user_info.empty()) _user_info = string_view{&*first, 0};
                    hp_state = hier_part_state::path;
                    first = it;
                }
                else if (*it == '?')
                {
                    _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                    if (_user_info.empty()) _user_info = string_view{&*first, 0};
                    _port = string_view{&*it, 0};
                    _path = string_view{&*it, 0};
                    state = uri_state::query;
                    ++it;
                    first = it;
                    break;
                }
                else if (*it == '#')
                {
                    _host = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                    if (_user_info.empty()) _user_info = string_view{&*first, 0};
                    _port = string_view{&*it, 0};
                    _path = string_view{&*it, 0};
                    state = uri_state::fragment;
                    ++it;
                    first = it;
                    break;
                }
                continue;
            }
        }
        else if (hp_state == hier_part_state::port)
        {
            if (*first == '/') /* the port is empty, but valid */
            {
                _port = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (!_read_port(_port, _port_i))
                {
                    throw uri_syntax_error{"Failed to parse port '" + _uri + "': " + (first-_uri_view.begin())};
                }
                /* the port isn't set, but the path is */
                hp_state = hier_part_state::path;
                continue;
            }
            if (*it == '/')
            {
                _port = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                if (!_read_port(_port, _port_i))
                {
                    throw uri_syntax_error{"Failed to parse port '" + _uri + "': " + (first-_uri_view.begin())};
                }
                hp_state = hier_part_state::path;
                first = it;
                continue;
            }
            else
            {
                if (it != last && (CHAR_MAP[static_cast<unsigned char>(*it)] & T_DIGIT)) ++it;
                else
                {
                    throw uri_syntax_error{"Expected digit symbol '" + _uri + "': " + (it-_uri_view.begin())};
                }
            }
        }
        else /* if (hp_state == hier_part_state::path) */
        {
            if (*it == '?')
            {
                _path = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                /* move past the query delimiter */
                ++it;
                first = it;
                state = uri_state::query;
                break;
            }
            else if (*it == '#')
            {
                _path = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                /* move past the fragment delimiter */
                ++it;
                first = it;
                state = uri_state::fragment;
                break;
            }

            if (!_skip_pchar(it, last))
            {
                if (it == last || *it != '/')
                {
                    throw uri_syntax_error{"Unexpected symbol '" + _uri + "': " + (it-_uri_view.begin())};
                }
                ++it;
            }
            continue;
        }
        ++it;
    }

    if (state == uri_state::query)
    {
        while (it != last)
        {
            if (!_skip_pchar(it, last))
            {
                if (*it !='?' && *it != '/')
                {
                    if (*it != '#')
                    {
                        throw uri_syntax_error{"Expected beginning of fragment '" + _uri +
                                               "': " + (it-_uri_view.begin())};
                    }
                    /* If this is a fragment, keep going */
                    _query = string_view{&*first, static_cast<string_view::size_type>(it-first)};
                    /* move past the fragment delimiter */
                    ++it;
                    first = it;
                    state = uri_state::fragment;
                    break;
                }
                else ++it;
            }
        }
    }

    if (state == uri_state::fragment && !_validate_fragment(it, last))
    {
        throw uri_syntax_error{"Invalid fragment '" + _uri + "': " + (it-_uri_view.begin())};
    }

    if (state == uri_state::hier_part)
    {
        if (hp_state == hier_part_state::authority)
        {
            if (!_set_host_and_port(first, last, last_colon))
            {
                throw uri_syntax_error{"Failed to set host and port '" + _uri + "': " + (first-_uri_view.begin())};
            }
            _user_info = string_view{&*first, 0};
            _path = string_view{&*last, 0};
            _query = string_view{&*last, 0};
            _fragment = string_view{&*last, 0};
        }
        else if (hp_state == hier_part_state::host)
        {
            if (!_set_host_and_port(first, last, last_colon))
            {
                throw uri_syntax_error{"Failed to set host and port '" + _uri + "': " + (first-_uri_view.begin())};
            }
            _path = string_view{&*last, 0};
            _query = string_view{&*last, 0};
            _fragment = string_view{&*last, 0};
        }
        else if (hp_state == hier_part_state::host_ipv6)
        {
            /* If we here it means that there was no closing bracket in IPv6 address */
            throw uri_syntax_error{"Invalid IPv6 address '" + _uri + "': " + (first-_uri_view.begin())};
        }
        else if (hp_state == hier_part_state::port)
        {
            _port = string_view{&*first, static_cast<string_view::size_type>(last-first)};
            if (!_read_port(_port, _port_i))
            {
                throw uri_syntax_error{"Failed to parse port '" + _uri + "': " + (first-_uri_view.begin())};
            }
            _path = string_view{&*last, 0};
            _query = string_view{&*last, 0};
            _fragment = string_view{&*last, 0};
        }
        else if (hp_state == hier_part_state::path)
        {
            _path = string_view{&*first, static_cast<string_view::size_type>(last-first)};
            _query = string_view{&*last, 0};
            _fragment = string_view{&*last, 0};
        }
    }
    else if (state == uri_state::query)
    {
        _query = string_view{&*first, static_cast<string_view::size_type>(last-first)};
        if (_fragment.empty()) _fragment = string_view{_query.data()+_query.length(), 0};
    }
    else if (state == uri_state::fragment)
    {
        if (_query.empty()) _query = string_view{_path.begin()+_path.length(), 0};
        _fragment = string_view{&*first, static_cast<string_view::size_type>(last-first)};
    }
//    else if (state == uri_state::hier_part)
//    {
//        if (_query.empty()) _query = string_view{_path.begin()+_path.length(), 0};
//        if (_fragment.empty()) _fragment = string_view{_query.begin()+_query.length(), 0};
//    }
}

} // end of servlet namespace
