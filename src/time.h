/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_TIME_H
#define SERVLET_TIME_H

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <ios>
#include <mutex>
#include <memory>
#include "string.h"
#include "os.h"

namespace servlet
{

template<typename CharT>
struct printable_time_point
{
    printable_time_point(tm _ptm, const CharT* _fmt, const CharT* _fmt_end, bool _owner) :
            ptm(_ptm), fmt(_fmt), fmt_end(_fmt_end), owner(_owner) {}

    printable_time_point(const printable_time_point<CharT> &ptp) :
            ptm(ptp.ptm), fmt{new CharT[ptp.fmt_end - ptp.fmt]}, fmt_end{fmt + (ptp.fmt_end - ptp.fmt)}, owner(true) {}

    printable_time_point(printable_time_point<CharT> &&ptp) :
            ptm(ptp.ptm), fmt(ptp.fmt), fmt_end(ptp.fmt_end), owner(ptp.owner)
    {
        ptp.owner = false;
    }

    std::tm ptm;
    const CharT *fmt, *fmt_end;
    bool owner;

    ~printable_time_point() noexcept { if (owner) delete[] fmt; }
};

inline std::tm local_now()
{
    auto epoch = std::chrono::system_clock::now().time_since_epoch();
    const std::time_t s = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(epoch).count());
    return get_tm(s);
}
inline std::tm gmt_now()
{
    auto epoch = std::chrono::system_clock::now().time_since_epoch();
    const std::time_t s = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(epoch).count());
    return get_gmtm(s);
}

inline std::string format_time(const char* fmt, std::tm tmv, std::size_t buf_size = 64)
{
    std::unique_ptr<char[]> data{new char[buf_size]};
    return std::string{data.get(), std::strftime(data.get(), buf_size-1, fmt, &tmv)};
}

/* This class supports formatted output of std::time_point.
 * The format used is a standard std::put_time and std::strftime with the addition of
 * specifier "%ss" which will be replaced by milliseconds in current second. */
template<typename CharT, typename Traits = std::char_traits<CharT>>
class basic_time_point_format
{
public:
    typedef std::basic_string<CharT, Traits> string_type;

    /* Default format "%y/%m/%d %H:%M:%S.%ss" */
    basic_time_point_format() : _format{"%y/%m/%d %H:%M:%S.%ss"}, _ms_index{_format.find(MS_SPECIFIER)} {}
    basic_time_point_format(const string_type &format) : _format{format}, _ms_index{_format.find(MS_SPECIFIER)} {}
    basic_time_point_format(string_type &&format) : _format{std::move(format)}, _ms_index{_format.find(MS_SPECIFIER)} {}

    template<typename Clock, typename Duration>
    printable_time_point<CharT> operator()(const std::chrono::time_point<Clock, Duration> &tp) const
    {
        auto epoch = tp.time_since_epoch();
        const std::time_t s = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(epoch).count());
        std::tm ptm = get_tm(s);
        if (_ms_index == string_type::npos) return {ptm, _format.data(), _format.data()+_format.size(), false};
        CharT *fmt_ptr = new CharT[_format.size()];
        std::copy(_format.begin(), _format.end(), fmt_ptr);
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
        int msl = static_cast<int>(ms / 1000 != s ? 0 : ms % 1000);
        pad_3(msl, fmt_ptr+_ms_index);
        return {ptm, fmt_ptr, fmt_ptr+_format.size(), true};
    };

private:
    static char MS_SPECIFIER[];
    string_type _format;
    typename string_type::size_type _ms_index;
};

template<typename CharT, typename Traits>
char basic_time_point_format<CharT, Traits>::MS_SPECIFIER[] = "%ss";

typedef basic_time_point_format<char> time_point_format;

inline std::time_t _epoch(int tmyear, int mon, int tmday, int isdst)
{
    std::tm tmv{};
    tmv.tm_year  = tmyear;
    tmv.tm_mon   = mon;
    tmv.tm_mday  = tmday;
    tmv.tm_isdst = isdst;
    return std::mktime(&tmv);
}
template <typename Clock = std::chrono::system_clock, typename Duration = typename Clock::duration>
std::chrono::time_point<Clock, Duration> date(int year, int month, int day)
{
    std::tm tmv{};
    tmv.tm_year  = year - 1900;
    tmv.tm_mon   = month - 1;
    tmv.tm_mday  = day;
    std::time_t s = std::mktime(&tmv);
    if (tmv.tm_isdst > 0)
    {
        std::tm tmv1{};
        tmv1.tm_year  = tmv.tm_year;
        tmv1.tm_mon   = tmv.tm_mon;
        tmv1.tm_mday  = tmv.tm_mday;
        tmv1.tm_isdst = tmv.tm_isdst;
        s = std::mktime(&tmv1);
    }
    return Clock::from_time_t(s);
}
template <typename Clock, typename Duration>
std::chrono::time_point<Clock, Duration> today(std::chrono::time_point<Clock, Duration> tp)
{
    std::tm tmv{};
    std::tm ptm = get_tm(Clock::to_time_t(tp));
    tmv.tm_year  = ptm.tm_year;
    tmv.tm_mon   = ptm.tm_mon;
    tmv.tm_mday  = ptm.tm_mday;
    tmv.tm_isdst = ptm.tm_isdst;
    std::time_t s = std::mktime(&tmv);
    return Clock::from_time_t(s);
}
inline std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> today()
{
    return today(std::chrono::system_clock::now());
}
template <typename Clock, typename Duration>
std::chrono::time_point<Clock, Duration> tomorrow(std::chrono::time_point<Clock, Duration> tp)
{
    std::tm ptm = get_tm(Clock::to_time_t(tp));
    return date<Clock, Duration>(ptm.tm_year+1900, ptm.tm_mon+1, ptm.tm_mday+1);
}
inline std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> tomorrow()
{
    return tomorrow(std::chrono::system_clock::now());
}
template <typename Clock, typename Duration>
std::chrono::time_point<Clock, Duration> yesterday(std::chrono::time_point<Clock, Duration> tp)
{
    std::tm ptm = get_tm(Clock::to_time_t(tp));
    return date<Clock, Duration>(ptm.tm_year+1900, ptm.tm_mon+1, ptm.tm_mday-1);
}
inline std::chrono::time_point<std::chrono::system_clock, typename std::chrono::system_clock::duration> yesterday()
{
    return yesterday(std::chrono::system_clock::now());
}

/* Prints time_point in default format. The default format is "%y/%m/%d %H:%M:%S.%ss" */
template<typename CharT, typename Traits, typename Clock, typename Duration>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& out,
                                              const std::chrono::time_point<Clock, Duration>& tp)
{
    /* Default format I parse myself, as it is way faster */
//    static basic_time_point_format<CharT, Traits> DEFAULT_FORMAT{"%y/%m/%d %H:%M:%S.%ss"};
//    out << DEFAULT_FORMAT(tp);
    const auto epoch = tp.time_since_epoch();
    const std::time_t s = std::time_t(std::chrono::duration_cast<std::chrono::seconds>(epoch).count());
    const std::tm ptm = get_tm(s);
    const int ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count() % 1000);
    char buf[22];

    pad_2(ptm.tm_year, buf);
    buf[2] = '/';
    pad_2(ptm.tm_mon+1, buf + 3);
    buf[5] = '/';
    pad_2(ptm.tm_mday, buf + 6);
    buf[8] = ' ';
    pad_2(ptm.tm_hour, buf + 9);
    buf[11] = ':';
    pad_2(ptm.tm_min, buf + 12);
    buf[14] = ':';
    pad_2(ptm.tm_sec, buf + 15);
    buf[17] = '.';
    pad_3(ms, buf + 18);
    buf[21] = 0;
    out << buf;

    return out;
};

template<typename CharT, typename Traits>
std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &out,
                                              const printable_time_point<CharT>& ptp)
{
    typename std::basic_ostream<CharT, Traits>::sentry sent(out);
    if (sent)
    {
        std::ios_base::iostate err = std::ios_base::goodbit;
        try
        {
            typedef std::ostreambuf_iterator<CharT, Traits> Iter;

            const std::time_put<CharT, Iter> &tl = std::use_facet<std::time_put<CharT, Iter>>(out.getloc());
            if (tl.put(Iter(out.rdbuf()), out, out.fill(), &ptp.ptm, ptp.fmt, ptp.fmt_end).failed())
            {
                err |= std::ios_base::badbit;
            }
        }
        catch(...)
        {
            out.setstate(std::ios_base::badbit);
        }
        if (err) out.setstate(err);
    }
    return out;
}

} // end of servlet namespace

#endif // SERVLET_TIME_H
