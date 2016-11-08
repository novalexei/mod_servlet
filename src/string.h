#ifndef SERVLET_STRING_H
#define SERVLET_STRING_H

#include <algorithm>
#include <type_traits>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <cstring>
#include <experimental/string_view>

#include <servlet/lib/exception.h>
#include <servlet/lib/io.h>
#include <servlet/lib/io_string.h>
#include "config.h"

namespace servlet
{

using std::experimental::string_view;
using std::experimental::basic_string_view;

template<typename CharT, typename Traits>
inline bool __ic_equal(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return std::equal(str1.begin(), str1.end(), str2.begin(),
                      [](auto ch1, auto ch2) { return std::tolower(ch1) == std::tolower(ch2); });
}
inline bool __ic_equal(const string_view str1, const string_view str2)
{
    return std::equal(str1.begin(), str1.end(), str2.begin(),
                      [](auto ch1, auto ch2) { return std::tolower(ch1) == std::tolower(ch2); });
}

template<typename CharT, typename Traits>
inline bool equal_ic(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return str1.length() == str2.length() && __ic_equal(str1, str2);
}
inline bool equal_ic(const string_view str1, const string_view str2)
{
    return str1.length() == str2.length() && __ic_equal(str1, str2);
}
template<typename CharT, typename Traits>
inline bool begins_with(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return str1.length() >= str2.length() && str1.substr(0, str2.length()) == str2;
}
inline bool begins_with(const string_view str1, const string_view str2)
{
    return str1.length() >= str2.length() && str1.substr(0, str2.length()) == str2;
}
template<typename CharT, typename Traits>
inline bool begins_with_ic(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return str1.length() >= str2.length() && __ic_equal(str1.substr(0, str2.length()), str2);
}
inline bool begins_with_ic(const string_view str1, const string_view str2)
{
    return str1.length() >= str2.length() && __ic_equal(str1.substr(0, str2.length()), str2);
}
template<typename CharT, typename Traits>
inline bool ends_with(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return str1.length() >= str2.length() && str1.substr(str1.length()-str2.length()) == str2;
}
inline bool ends_with(const string_view str1, const string_view str2)
{
    return str1.length() >= str2.length() && str1.substr(str1.length()-str2.length()) == str2;
}
template<typename CharT, typename Traits>
inline bool ends_with_ic(const basic_string_view<CharT, Traits> str1, const basic_string_view<CharT, Traits> str2)
{
    return str1.length() >= str2.length() && __ic_equal(str1.substr(str1.length()-str2.length()), str2);
}
inline bool ends_with_ic(const string_view str1, const string_view str2)
{
    return str1.length() >= str2.length() && __ic_equal(str1.substr(str1.length()-str2.length()), str2);
}

template<typename CharT, typename Traits>
class token_iterator
{
public:
    typedef basic_string_view<CharT, Traits> string_type;
    typedef token_iterator<CharT, Traits> self_type;

    typedef string_type value_type;
    typedef string_type *pointer;
    typedef string_type reference;
    typedef std::ptrdiff_t difference_type;
    typedef std::forward_iterator_tag iterator_category;

    token_iterator(const string_type &str, const string_type &delim, bool include_delim_in_token) noexcept :
            _string{str}, _delim{delim}, _include_delim_in_token{include_delim_in_token}
    {
        load_next();
    }

    token_iterator(const self_type& other) = default;
    token_iterator(self_type&& other) = default;

    token_iterator() noexcept : _string(""), _delim(""),
                                _start_pos(string_type::npos), _delim_index(string_type::npos) {}

    self_type& operator=(const self_type& other) = default;
    self_type& operator=(self_type&& other) = default;

    value_type operator*() const { return _string.substr(_start_pos, _delim_index - _start_pos); }

    /* We don't support -> as all dereferenced elements are temporary */
    pointer operator->() const noexcept = delete;

    inline self_type &operator++() noexcept
    {
        load_next();
        return *this;
    }

    inline self_type operator++(int) noexcept
    {
        self_type tmp(*this);
        load_next();
        return tmp;
    }

    bool operator==(const token_iterator &other) const noexcept
    {
        if (_start_pos == string_type::npos) return other._start_pos == string_type::npos;
        return _string == other._string &&
               _delim == other._delim &&
               _start_pos == other._start_pos &&
               _delim_index == other._delim_index &&
               _include_delim_in_token == other._include_delim_in_token;
    }

    bool operator!=(const token_iterator &other) const noexcept
    {
        return !(other == *this);
    }

private:
    bool load_next() noexcept;

    string_type _string;
    string_type _delim;
    typename string_type::size_type _start_pos = 0;
    typename string_type::size_type _delim_index = 0;
    bool _include_delim_in_token;
};

template<typename CharT, typename Traits>
class basic_tokenizer
{
public:
    typedef basic_string_view<CharT, Traits> string_type;

    basic_tokenizer(const string_type str, const string_type delim = " \t\n\r\f", bool include_delim_in_token = false) :
            _string{str}, _delim{delim}, _include_delim_in_token{include_delim_in_token} {}

    token_iterator<CharT, Traits> begin() const noexcept
    {
        return token_iterator<CharT, Traits>{_string, _delim, _include_delim_in_token};
    }

    token_iterator<CharT, Traits> end() const noexcept
    {
        return token_iterator<CharT, Traits>{};
    }

    void append_tokens(std::vector<string_view>& tokens)
    {
        for (auto &token : *this) tokens.push_back(token);
    }

private:
    string_type _string;
    string_type _delim;
    bool _include_delim_in_token;
};

typedef basic_tokenizer<typename string_view::value_type, typename string_view::traits_type> tokenizer;

template<typename CharT, typename Traits>
bool token_iterator<CharT, Traits>::load_next() noexcept
{
    _start_pos = _delim_index;
    if (_start_pos == string_type::npos) return false;
    if (_delim.size() == 1)
    {
        if (!_include_delim_in_token) _start_pos = _string.find_first_not_of(_delim[0], _start_pos);
        _delim_index = _string.find(_delim[0], _start_pos);
    }
    else
    {
        if (!_include_delim_in_token) _start_pos = _string.find_first_not_of(_delim, _start_pos);
        _delim_index = _string.find_first_of(_delim, _start_pos);
    }
    if (_include_delim_in_token && _delim_index != string_type::npos)
    {
        ++_delim_index;
        if (_delim_index >= _string.size()) _delim_index = string_type::npos;
    }
    return true;
}

/* Trim functions.
 * Functions trim*_view will return string_view to avoid new string allocations. */
/* trim_left */
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_left_view(basic_string_view<CharT, Traits> str,
                                                       basic_string_view<CharT, Traits> delim)
{
    if (str.empty()) return str;
    const typename basic_string_view<CharT, Traits>::size_type index = str.find_first_not_of(delim);
    return index == basic_string_view<CharT, Traits>::npos ? str : str.substr(index);
};
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_left_view(basic_string_view<CharT, Traits> str, const CharT* delim)
{
    if (str.empty()) return str;
    const typename basic_string_view<CharT, Traits>::size_type index = str.find_first_not_of(delim);
    return index == basic_string_view<CharT, Traits>::npos ? str : str.substr(index);
};
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_left_view(basic_string_view<CharT, Traits> str)
{
    return trim_left_view(str, " \t\n\r\f");
};
template<typename CharT>
inline basic_string_view<CharT, std::char_traits<CharT>> trim_left_view(const CharT *str)
{
    return trim_left_view(str, " \t\n\r\f");
};

template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_left_view(const std::basic_string<CharT, Traits> &str,
                                                       const std::basic_string<CharT, Traits> &delim)
{
    return trim_left_view(basic_string_view<CharT, Traits>{str}, basic_string_view<CharT, Traits>{delim});
};
template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_left_view(const std::basic_string<CharT, Traits> &str)
{
    return trim_left_view(str, " \t\n\r\f");
};
template<typename CharT, typename Traits>
inline std::basic_string<CharT, Traits> trim_left(const std::basic_string<CharT, Traits> &str,
                                                  const std::basic_string<CharT, Traits> &delim = " \t\n\r\f")
{
    if (str.empty()) return str;
    const typename std::basic_string<CharT, Traits>::size_type index = str.find_first_not_of(delim);
    return index == std::basic_string<CharT, Traits>::npos ? str : str.substr(index);
};

/* trim_right */
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_right_view(basic_string_view<CharT, Traits> str,
                                                        basic_string_view<CharT, Traits> delim)
{
    if (str.empty()) return str;
    const typename basic_string_view<CharT, Traits>::size_type index = str.find_last_not_of(delim);
    if (index == basic_string_view<CharT, Traits>::npos) return basic_string_view<CharT, Traits>{};
    if (index == str.size() - 1) return str;
    return str.substr(0, index + 1);
};
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_right_view(basic_string_view<CharT, Traits> str, const CharT* delim)
{
    if (str.empty()) return str;
    const typename basic_string_view<CharT, Traits>::size_type index = str.find_last_not_of(delim);
    if (index == basic_string_view<CharT, Traits>::npos) return basic_string_view<CharT, Traits>{};
    if (index == str.size() - 1) return str;
    return str.substr(0, index + 1);
};
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_right_view(basic_string_view<CharT, Traits> str)
{
    return trim_right_view(str, " \t\n\r\f");
};
template<typename CharT>
inline basic_string_view<CharT, std::char_traits<CharT>> trim_right_view(const CharT *str)
{
    return trim_right_view(basic_string_view<CharT, std::char_traits<CharT>>{str}, " \t\n\r\f");
};
template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_right_view(const std::basic_string<CharT, Traits> &str,
                                                        const std::basic_string<CharT, Traits> &delim)
{
    return trim_right_view(basic_string_view<CharT, std::char_traits<CharT>>{str},
                           basic_string_view<CharT, std::char_traits<CharT>>{delim});
};
template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_right_view(const std::basic_string<CharT, Traits> &str)
{
    return trim_right_view(basic_string_view<CharT, Traits>{str}, " \t\n\r\f");
};

template<typename CharT, typename Traits>
inline std::basic_string<CharT, Traits> trim_right(const std::basic_string<CharT, Traits> &str,
                                                   const std::basic_string<CharT, Traits> &delim = " \t\n\r\f")
{
    if (str.empty()) return str;
    const typename std::basic_string<CharT, Traits>::size_type index = str.find_last_not_of(delim);
    if (index == std::basic_string<CharT, Traits>::npos) return std::basic_string<CharT, Traits>{};
    if (index == str.size() - 1) return str;
    return str.substr(0, index + 1);
};

/* trim */
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_view(basic_string_view<CharT, Traits> str, const CharT* delim)
{
    if (str.empty()) return str;
    typename basic_string_view<CharT, Traits>::size_type left_index = str.find_first_not_of(delim);
    if (left_index == basic_string_view<CharT, Traits>::npos) return basic_string_view<CharT, Traits>{};
    typename basic_string_view<CharT, Traits>::size_type right_index = str.find_last_not_of(delim);
    right_index = right_index == str.size() - 1 ? basic_string_view<CharT, Traits>::npos : right_index + 1;
    if (left_index == 0 && right_index == basic_string_view<CharT, Traits>::npos) return str;
    return str.substr(left_index, right_index-left_index);
}
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_view(basic_string_view<CharT, Traits> str,
                                                  basic_string_view<CharT, Traits> delim)
{
    if (str.empty()) return str;
    typename basic_string_view<CharT, Traits>::size_type left_index = str.find_first_not_of(delim);
    if (left_index == basic_string_view<CharT, Traits>::npos) return basic_string_view<CharT, Traits>{};
    typename basic_string_view<CharT, Traits>::size_type right_index = str.find_last_not_of(delim);
    right_index = right_index == str.size() - 1 ? basic_string_view<CharT, Traits>::npos : right_index + 1;
    if (left_index == 0 && right_index == basic_string_view<CharT, Traits>::npos) return str;
    return str.substr(left_index, right_index-left_index);
}
template<typename CharT, typename Traits>
inline basic_string_view<CharT, Traits> trim_view(basic_string_view<CharT, Traits> str)
{
    return trim_view(str, " \t\n\r\f");
}
template<typename CharT>
inline basic_string_view<CharT, std::char_traits<CharT>> trim_view(const CharT* str)
{
    return trim_view(basic_string_view<CharT, std::char_traits<CharT>>{str}, " \t\n\r\f");
}
template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_view(const std::basic_string<CharT, Traits> &str)
{
    return trim_view(basic_string_view<CharT, Traits>{str}, " \t\n\r\f");
};
template<typename CharT, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> trim_view(const std::basic_string<CharT, Traits> &str, const CharT* delim)
{
    return trim_view(basic_string_view<CharT, Traits>{str}, delim);
};

template<typename CharT, typename Traits>
inline std::basic_string<CharT, Traits> trim(const std::basic_string<CharT, Traits> &str,
                                             const std::basic_string<CharT, Traits> &delim = " \t\n\r\f")
{
    if (str.empty()) return str;
    typename std::basic_string<CharT, Traits>::size_type left_index = str.find_first_not_of(delim);
    if (left_index == std::basic_string<CharT, Traits>::npos) return std::basic_string<CharT, Traits>{};
    typename std::basic_string<CharT, Traits>::size_type right_index = str.find_last_not_of(delim);
    right_index = right_index == str.size() - 1 ? std::basic_string<CharT, Traits>::npos : right_index + 1;
    if (left_index == 0 && right_index == std::basic_string<CharT, Traits>::npos) return str;
    return str.substr(left_index, right_index-left_index);
};

template<typename CharT, typename Traits = std::char_traits<CharT>>
class string_ref_sink
{
public:
    typedef std::basic_string<CharT, Traits>                    string_type;
    typedef std::experimental::basic_string_view<CharT, Traits> string_view_type;

    string_ref_sink(string_type& str) : _buffer{str}, _flushed{false} {}

    inline std::streamsize write(const CharT* s, std::streamsize n)
    {
        _buffer.append(s, n);
        return n;
    }
    inline bool flush() { _flushed = true; return true; }

    inline void reset() { _buffer.clear(); _flushed = false; }
    inline string_view_type view() const { return string_view_type{_buffer}; }
    inline string_type& str() { return _buffer; }
    inline bool was_flushed() const { return _flushed; }
private:
    string_type& _buffer;
    bool _flushed;
};

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_inplace_ref_ostream = basic_outstream<string_ref_sink<CharT, Traits>, non_buffered, CharT, Traits>;
typedef basic_inplace_ref_ostream<char> inplace_ref_ostream;

template<class CharT, class Traits = std::char_traits<CharT>>
class string_view_buffer_provider
{
public:
    typedef buffer_provider category;

    typedef std::basic_string<CharT, Traits>                    string_type;
    typedef std::experimental::basic_string_view<CharT, Traits> string_view_type;

    string_view_buffer_provider(string_view_type str) : _str{str} {}

    std::pair<char*, std::size_t> get_buffer()
    {
        if (_buffer_provided) return std::pair<char*, std::size_t>{nullptr, 0};
        _buffer_provided = true;
        return std::make_pair(const_cast<char*>(_str.data()), _str.size());
    }

    inline void reset() { _buffer_provided = false; }
    inline string_type str() { return _str.to_string(); }
private:
    string_view_type _str;
    bool _buffer_provided = false;
};

template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view_istream = basic_instream<string_view_buffer_provider<CharT, Traits>, non_buffered, CharT, Traits>;
typedef basic_string_view_istream<char> string_view_istream;

template<typename Stream>
class counting_sink
{
public:
    typedef Stream                            stream_type;
    typedef typename stream_type::char_type   char_type;

    template <class... Args>
    counting_sink(Args&&... args) : _out{std::forward<Args>(args)...}, _count{0} {}

    inline std::streamsize write(const char_type* s, std::streamsize n)
    {
        _count += n;
        _out.write(s, n);
        return n;
    }
    inline bool flush() { return true; }

    inline void reset() { _count = 0; }
    inline std::size_t count() const { return _count; }

    inline stream_type& stream() { return _out; }

private:
    stream_type _out;
    std::size_t _count;
};

template<typename Stream, typename Buffering = non_buffered>
using counted_ostream = basic_outstream<counting_sink<Stream>, Buffering,
                                        typename Stream::char_type, typename Stream::traits_type>;
typedef counted_ostream<std::ofstream> counted_ofstream;

/* string_cast operation allows to parse string into data type in generic manner:
 * int i = string_cast<int>("25") */
template<typename T, typename CharT, typename Traits>
T string_cast(basic_string_view<CharT, Traits> str, bool throw_if_not_whole_string = false)
{
    static_assert(std::is_trivially_constructible<T>::value,
                  "Cannot cast into non trivially constructable type. Use string_cast(string_view& , T& ) instead.");
    if (str.empty())
    {
        throw bad_cast{std::string{"cannot cast from empty string to type "}.append(demangle(typeid(T).name()))};
    }
    basic_string_view_istream<CharT, Traits> input{str};
    T value;
    input >> value;
    if (!input)
    {
        throw bad_cast{std::string{"failed to cast from string to type "}.append(demangle(typeid(T).name()))};
    }
    if (throw_if_not_whole_string && input.rdbuf()->in_avail() > 0)
    {
        throw bad_cast{std::string{"failed to cast from whole string \""}.append(str.begin(), str.end()).
                append("\" to ").append(demangle(typeid(T).name()))};
    }
    return value;
}
template<typename T, typename CharT>
inline T string_cast(const CharT *str, bool throw_if_not_whole_string = false)
{ return string_cast<T>(basic_string_view<CharT>{str}, throw_if_not_whole_string); }

template<typename T, typename CharT, typename Traits>
inline T string_cast(const std::basic_string<CharT, Traits>& str, bool throw_if_not_whole_string = false)
{ return string_cast<T>(basic_string_view<CharT, Traits>{str}, throw_if_not_whole_string); }

template<typename T, typename CharT, typename Traits>
inline T string_cast(std::basic_string<CharT, Traits>&& str, bool throw_if_not_whole_string = false)
{ return string_cast<T>(basic_string_view<CharT, Traits>{std::move(str)}, throw_if_not_whole_string); }


/* string_cast operation allows to parse string into data type in generic manner:
 * int i; string_cast("25", i)
 * This function is designed to be used with types which cannot be trivially constructed. */
template<typename T, typename CharT, typename Traits>
void string_cast(basic_string_view<CharT, Traits> str, T &to_value, bool throw_if_not_whole_string = false)
{
    if (str.empty())
    {
        throw bad_cast{std::string{"cannot cast from empty string to type "}.append(demangle(typeid(T).name()))};
    }
    basic_string_view_istream<CharT, Traits> input{str};
    input >> to_value;
    if (!input)
    {
        throw bad_cast{std::string{"failed to cast from string to type "}.append(demangle(typeid(T).name()))};
    }
    if (throw_if_not_whole_string && input.rdbuf()->in_avail() > 0)
    {
        throw bad_cast{std::string{"failed to cast from whole string \""}.append(str.begin(), str.end()).
                append("\" to ").append(demangle(typeid(T).name()))};
    }
}

template<typename CharT = char, typename Traits = std::char_traits<CharT>>
inline const std::basic_string<CharT, Traits> &string_cast(const std::basic_string<CharT, Traits> &str) { return str; }

template<typename CharT = char, typename Traits = std::char_traits<CharT>>
inline std::basic_string<CharT, Traits> &string_cast(std::basic_string<CharT, Traits> &str) { return str; }

template<typename CharT = char, typename Traits = std::char_traits<CharT>>
inline std::basic_string<CharT, Traits> string_cast(std::basic_string<CharT, Traits> &&str) { return std::move(str); }


template<typename T, typename CharT, typename Traits>
T from_string(basic_string_view<CharT, Traits> str, T dflt)
{
    static_assert(std::is_trivially_constructible<T>::value, "Cannot cast into non trivially constructable type.");
    if (str.empty()) return dflt;
    basic_string_view_istream<CharT, Traits> input{str};
    T value;
    input >> std::boolalpha >> value;
    return input ? value : dflt;
}
template<typename T, typename CharT>
inline T from_string(const CharT *str, T dflt) { return from_string<T>(basic_string_view<CharT>{str}, dflt); }

template<typename T, typename CharT, typename Traits>
inline T from_string(const std::basic_string<CharT, Traits>& str, T dflt)
{ return from_string<T>(basic_string_view<CharT, Traits>{str}, dflt); }

template<typename T, typename CharT, typename Traits>
inline T from_string(std::basic_string<CharT, Traits>&& str, T dflt)
{ return from_string<T>(basic_string_view<CharT, Traits>{std::move(str)}, dflt); }

template<typename T, typename CharT = char>
inline std::basic_string<CharT> to_string(const T& value) { return std::basic_string<CharT>{} << value; }

/* operator+ to concatenate std::string and std::string_view */
template<typename CharT, typename Traits, typename Alloc>
inline std::basic_string<CharT, Traits, Alloc> operator+(const std::basic_string<CharT, Traits, Alloc> &ls,
                                                         const basic_string_view<CharT, Traits> rs)
{
    std::basic_string<CharT, Traits, Alloc> str;
    str.reserve(ls.size()+rs.size());
    str.append(ls).append(rs.begin(), rs.end());
    return str;
}
template<typename CharT, typename Traits, typename Alloc>
inline std::basic_string<CharT, Traits, Alloc> operator+(const basic_string_view<CharT, Traits> ls,
                                                         const std::basic_string<CharT, Traits, Alloc> &rs)
{
    std::basic_string<CharT, Traits, Alloc> str;
    str.reserve(ls.size()+rs.size());
    str.append(ls.begin(), ls.end()).append(rs);
    return str;
}

template<typename T, typename CharT, typename Traits, typename Alloc>
std::basic_string<CharT, Traits, Alloc> operator+(const std::basic_string<CharT, Traits, Alloc>& str, const T& value)
{
    return std::basic_string<CharT, Traits>{str} << value;
}
template<typename T, typename CharT, typename Traits>
std::basic_string<CharT, Traits> operator+(basic_string_view<CharT, Traits> str, const T& value)
{
    return std::basic_string<CharT, Traits>{str.to_string()} << value;
}
template<typename T, typename CharT, typename Traits>
std::basic_string<CharT, Traits> operator+(basic_string_view<CharT, Traits> str, const CharT* value)
{
    basic_string_view<CharT, Traits> val_str{value};
    std::basic_string<CharT, Traits> res;
    res.reserve(str.size() + val_str.size());
    res.append(str.data(), str.size()).append(val_str.data(), val_str.size());
    return res;
}
template<typename T, typename CharT>
std::basic_string<CharT> operator+(const CharT* str, const T& value)
{
    return std::basic_string<CharT>{str, std::strlen(str)} << value;
}

template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
std::basic_string<CharT, Traits>& operator<<(std::basic_string<CharT, Traits>& str, const T& value)
{
    basic_inplace_ref_ostream<CharT, Traits>{str} << value;
    return str;
}
template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
std::basic_string<CharT, Traits> operator<<(std::basic_string<CharT, Traits>&& str, const T& value)
{
    basic_inplace_ref_ostream<CharT, Traits>{str} << value;
    return str;
}

template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> operator>>(basic_string_view<CharT, Traits> str, T& value)
{
    basic_string_view_istream<CharT, Traits> in{str};
    in >> value;
    return in.eof() ? basic_string_view<CharT, Traits>{} : str.substr(in.tellg());
}
template<typename T, typename CharT = char, typename Traits = std::char_traits<CharT>>
inline basic_string_view<CharT, Traits> operator>>(const std::basic_string<CharT, Traits>& str, T& value)
{
    basic_string_view_istream<CharT, Traits> in{str};
    in >> value;
    if (in.eof()) return basic_string_view<CharT, Traits>{};
    auto sz = in.tellg();
    return basic_string_view<CharT, Traits>{str.data()+sz, str.length()-sz};
}

/* Just quick int reader. This is much faster than previous operators, as it is not using locales,
 * thus ANSI string is expected */
template <typename IntType>
inline std::enable_if_t<std::is_integral<IntType>::value & std::is_unsigned<IntType>::value, string_view>
operator>>(string_view& in, IntType& value)
{
    value = 0;
    auto b = in.begin();
    auto e = in.end();
    while (b != e && *b >= '0' && *b <= '9')
    {
        value *= 10;
        value += *b - '0';
        ++b;
    }
    return in.substr(b-in.begin());
}
template <typename IntType>
inline std::enable_if_t<std::is_integral<IntType>::value & std::is_signed<IntType>::value, string_view>
operator>>(string_view& in, IntType& value)
{
    if (in.empty()) return in;
    value = 0;
    auto b = in.begin();
    bool minus = false;
    if (*b == '-')
    {
        minus = true;
        ++b;
    }
    auto e = in.end();
    while (b != e && *b >= '0' && *b <= '9')
    {
        value *= 10;
        value += *b - '0';
        ++b;
    }
    if (minus) value = -value;
    return in.substr(b-in.begin());
}

template <unsigned int num, typename IntT, typename Iter, typename IterTraits = std::iterator_traits<Iter>>
struct pad_helper
{
    inline static IntT pad(IntT value, Iter it, typename IterTraits::value_type pad_char)
    {
        value = pad_helper<num-1, IntT, Iter, IterTraits>::pad(value, it+1, pad_char);
        if (value == 0)
        {
            *it = pad_char;
            return 0;
        }
        else if (value < 10)
        {
            *it = '0' + value;
            return 0;
        }
        else
        {
            *it = '0' + (value % 10);
            return value / 10;
        }
    }
};
template <typename IntT, typename Iter, typename IterTraits>
struct pad_helper<1, IntT, Iter, IterTraits>
{
    inline static IntT pad(IntT value, Iter it, typename IterTraits::value_type pad_char)
    {
        if (value == 0)
        {
            *it = pad_char;
            return 0;
        }
        else if (value < 10)
        {
            *it = '0' + value;
            return 0;
        }
        else
        {
            *it = '0' + (value % 10);
            return value / 10;
        }
    }
};

template <unsigned int num, typename IntT, typename Iter, typename IterTraits = std::iterator_traits<Iter>>
inline typename std::enable_if_t<std::is_integral<IntT>::value, void> pad(IntT value, Iter it,
                                                                          typename IterTraits::value_type pad_char = '0')
{ pad_helper<num, IntT, Iter, IterTraits>::pad(value, it, pad_char); }


inline void pad_2(int value, char *buf)
{
    if (value > 99) value %= 100;
    int units = value / 10; /* tens */
    buf[0] = '0' + static_cast<char>(units);
    value -= units * 10; /* remove tens from ms */
    buf[1] = '0' + static_cast<char>(value);
}

inline void pad_3(int value, char *buf)
{
    if (value > 999) value %= 1000;
    int units = value / 100; /* hundreds */
    *buf = '0' + static_cast<char>(units);
    value -= units * 100; /* remove hundreds from ms */
    units = value / 10; /* tens */
    buf[1] = '0' + static_cast<char>(units);
    value -= units * 10; /* remove tens from ms */
    buf[2] = '0' + static_cast<char>(value);
}

template<typename CharT, typename ValueT>
struct pad_fmt
{
    ValueT value;
    int width;
    CharT fill;

    pad_fmt(ValueT val, int w, CharT f) : value(val), width(w), fill(f) {}
};

template<typename CharT, typename Traits, typename ValueT>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out,
                                              const pad_fmt<CharT, ValueT>& pad)
{
    if (pad.fill != 0) out << std::setfill(pad.fill);
    if (pad.width > 0) out << std::setw(pad.width);
    out << pad.value;
    return out;
}

template <typename ValueT, typename CharT = char>
pad_fmt<CharT, ValueT> setpad(ValueT value, int width = 0, CharT fill = '\0')
{
    return pad_fmt<CharT, ValueT>{value, width, fill};
}

} // end of servlet namespace

#endif // SERVLET_STRING_H
