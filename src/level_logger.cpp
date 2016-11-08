/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <codecvt>

#include <servlet/lib/logger.h>

namespace servlet { namespace logging {

level_logger &level_logger::operator<<(std::ostream &(*manip)(std::ostream &))
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    (*_out) << manip;
    if ((*_out)->view().back() == '\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::operator<<(char ch)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    (*_out).put(ch);
    if (ch == '\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::operator<<(char16_t ch)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> utf8_conv;
    (*_out) << utf8_conv.to_bytes(ch);
    if (ch == u'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::operator<<(const char16_t* str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> utf8_conv;
    char16_t ch;
    while ((ch = *str++) != u'\0') (*_out) << utf8_conv.to_bytes(ch);
    if ((*--str) == u'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}
level_logger &level_logger::operator<<(const std::u16string& str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> utf8_conv;
    for (auto &ch : str) (*_out) << utf8_conv.to_bytes(ch);
    if (str.back() == u'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::operator<<(char32_t ch)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_conv;
    (*_out) << utf8_conv.to_bytes(ch);
    if (ch == U'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}
level_logger &level_logger::operator<<(const char32_t* str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_conv;
    char32_t ch;
    while ((ch = *str++) != U'\0') (*_out) << utf8_conv.to_bytes(ch);
    if ((*--str) == U'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}
level_logger &level_logger::operator<<(const std::u32string& str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8_conv;
    for (auto &ch : str) (*_out) << utf8_conv.to_bytes(ch);
    if (str.back() == U'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::operator<<(wchar_t ch)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8_conv;
    (*_out) << utf8_conv.to_bytes(ch);
    if (ch == L'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}
level_logger &level_logger::operator<<(const wchar_t* str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8_conv;
    wchar_t ch;
    while ((ch = *str++) != L'\0') (*_out) << utf8_conv.to_bytes(ch);
    if ((*--str) == L'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}
level_logger &level_logger::operator<<(const std::wstring& str)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8_conv;
    for (auto &ch : str) (*_out) << utf8_conv.to_bytes(ch);
    if (str.back() == L'\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

level_logger &level_logger::write(const char* str, std::streamsize n)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    (*_out).write(str, n);
    if (str[n-1] == '\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

} // end of servlet::logging namespace

} // end of servlet namespace
