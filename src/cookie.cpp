#include <servlet/cookie.h>
#include <servlet/lib/exception.h>
#include "string.h"
#include "time.h"

/**
 * This code primarily borrowed from Apache Tomcat 7.0.40 source.
 */

namespace servlet
{

static std::string HTTP_SEPARATORS{"\t \"(),:;<=>?@[\\]{}"};

inline bool already_quoted(const std::string &value)
{
    return value.length() > 1 && value[0] == '\"' && value[value.length()-1] == '\"';
}

/**
 * Returns true if the byte is a separator as defined by V1 of the cookie spec, RFC2109.
 */
bool is_http_separator(char c)
{
    if (c < 0x20 || c >= 0x7f)
    {
        if (c != 0x09) throw invalid_argument_exception{"Control character in cookie value or attribute."};
    }

    return HTTP_SEPARATORS.find(c) != std::string::npos;
}

bool is_http_token(const std::string &value)
{
    if(value.empty()) return false;

    std::size_t i = 0;
    std::size_t len = value.length();

    if (already_quoted(value))
    {
        ++i;
        --len;
    }

    for (; i < len; ++i)
    {
        if (is_http_separator(value[i])) return true;
    }
    return false;
}

void escape_double_quotes(std::string &buf, const std::string &s, std::size_t begin_idx, std::size_t end_idx)
{

    if (s.empty() || s.find('"') == std::string::npos) return;

    for (std::size_t i = begin_idx; i < end_idx; ++i)
    {
        char c = s[i];
        if (c == '\\' )
        {
            buf.append(1, c);
            /* ignore the character after an escape, just append it */
            if (++i >= end_idx)
            {
                throw invalid_argument_exception{"Invalid escape character in cookie value."};
            }
            buf.append(1, s[i]);
        }
        else if (c == '"') buf.append(1, '\\').append(1, '"');
        else buf.append(1, c);
    }
}

void maybe_quote(std::string &buf, const std::string &value)
{
    if (value.empty()) buf.append("\"\"");
    else if (already_quoted(value))
    {
        buf.append(1, '"');
        escape_double_quotes(buf, value, 1, value.length()-1);
        buf.append(1, '"');
    }
    else if (is_http_token(value))
    {
        buf.append(1, '"');
        escape_double_quotes(buf, value, 0, value.length());
        buf.append(1, '"');
    }
    else buf.append(value);
}

std::string cookie::to_string() const
{
    std::string buf{};
    buf.append(_name);
    buf.append("=");

    /*
     * The spec allows some latitude on when to send the version attribute
     * with a Set-Cookie header. To be nice to clients, we'll make sure the
     * version attribute is first. That means checking the various things
     * that can cause us to switch to a v1 cookie first.
     *
     * Note that by checking for tokens we will also throw an exception if a
     * control character is encountered.
     */
    /* Start by using the version we were asked for */
    int newVersion = _version;

    /* If it is v0, check if we need to switch */
    if (newVersion == 0 && is_http_token(_value))  newVersion = 1; /* HTTP token in value - need to use v1 */
    if (newVersion == 0 && !_comment.empty())      newVersion = 1; /* Using a comment makes it a v1 cookie */
    if (newVersion == 0 && is_http_token(_path))   newVersion = 1; /* HTTP token in path - need to use v1 */
    if (newVersion == 0 && is_http_token(_domain)) newVersion = 1; /* HTTP token in domain - need to use v1 */

    /* Now build the cookie header */
    /* Value */
    maybe_quote(buf, _value);
    /* Add version 1 specific information */
    if (newVersion == 1)
    {
        /* Version=1 ... required */
        buf.append ("; Version=1");

        /* Comment=comment */
        if (!_comment.empty())
        {
            buf.append ("; Comment=");
            maybe_quote(buf, _comment);
        }
    }

    /* Add domain information, if present */
    if (!_domain.empty())
    {
        buf.append("; Domain=");
        maybe_quote(buf, _domain);
    }

    /* Max-Age=secs ... or use old "Expires" format */
    if (_max_age >= 0)
    {
        if (newVersion > 0)
        {
            buf.append("; Max-Age=");
            buf << _max_age;
        }
        /* IE6, IE7 and possibly other browsers don't understand Max-Age.
           They do understand Expires, even with V1 cookies! */
        if (newVersion == 0)
        {
            /* Wdy, DD-Mon-YY HH:MM:SS GMT ( Expires Netscape format ) */
            buf.append("; Expires=");
            /* To expire immediately we need to set the time in past */
            buf.append(format_time("%a, %d-%b-%Y %H:%M:%S %Z", get_gmtm(_max_age == 0 ? 10 : _max_age), 32));
        }
    }

    /* Path=path */
    if (!_path.empty())
    {
        buf.append("; Path=");
        maybe_quote(buf, _path);
    }

    /* Secure */
    if (_secure) buf.append("; Secure");

    /* HttpOnly */
    if (_http_only) buf.append("; HttpOnly");

    return buf;
}

} // end of servlet namespace
