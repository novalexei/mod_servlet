#include "properties.h"

namespace servlet
{

std::istream &operator>>(std::istream &in, properties_file &props)
{
    props.load(in);
    return in;
}

std::ostream &operator<<(std::ostream &out, properties_file &props)
{
    props.list(out);
    return out;
}

inline wchar_t read_wchar(const char *&from, const char *from_end)
{
    using std::experimental::string_view;
    string_view_istream in{string_view{from, static_cast<string_view::size_type>(from_end - from)}};
    int ich;
    in >> std::hex >> ich;
    from += in.tellg();
//    from = from_end - in.in_avail();
    return static_cast<wchar_t>(ich);
}

void properties_file::load(std::istream &in)
{
    std::string line{};
    std::string key{};
    std::string value{};
    bool reading_key = true;
    bool reading_value = false;
    bool eat_whitespaces = false;
    while (std::getline(in, line))
    {
        const char *pch = line.data();
        const char *line_end = line.data() + line.size();
        bool had_backslash = false;
        while (pch != line_end)
        {
            if (had_backslash)
            {
                if (*pch == 'u' || *pch == 'U' || *pch == 'x')
                {
                    ++pch;
                    wchar_t wch = read_wchar(pch, line_end);
                    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
                    (reading_key ? key : value) += utf8_conv.to_bytes(wch);
                }
                else
                {
                    (reading_key ? key : value) += *pch;
                    ++pch;
                }
                had_backslash = false;
                continue;
            }
            if (eat_whitespaces)
            {
                if (*pch == ' ' || *pch == '\t' || *pch == '\r' || *pch == '\f')
                {
                    ++pch;
                    continue;
                }
                else eat_whitespaces = false;
            }
            switch (*pch)
            {
                case '\\':had_backslash = true;
                    break;
                case '!':
                case '#':goto out_of_loop;
                case ' ':
                case '\t':
                case '\r':
                case '\f':
                    if (reading_key) reading_key = false;
                    else if (reading_value) value += *pch;
                    break;
                case '=':
                case ':':
                    if (reading_value) value += *pch;
                    else
                    {
                        reading_key = false;
                        reading_value = true;
                        eat_whitespaces = true;
                    }
                    break;
                default:(reading_key ? key : value) += *pch;
                    break;
            }
            ++pch;
        }
        out_of_loop:
        if (!had_backslash)
        {
            if (!key.empty()) emplace(key, value);
            key.clear();
            value.clear();
            reading_key = true;
            reading_value = false;
        }
        else eat_whitespaces = true;
    }
}

} // end of servlet namespace
