/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IO_STRING_H
#define MOD_SERVLET_IO_STRING_H

#include <servlet/lib/io.h>

namespace servlet
{

/**
 * Implementation of sink to <code>std::string</code>
 * Besides to having similar functionality to
 * <code>std::basic_ostringstream</code> this class also allows to access
 * directly the destination <code>std::string</code>
 * @tparam CharT character type for this stream
 * @tparam Traits cahracter traits type to be used in this stream.
 * @see sink
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
class string_sink
{
public:
    /**
     * Type of the underlying string.
     */
    typedef std::basic_string<CharT, Traits>                    string_type;
    /**
     * <code>string_view</code> for <code>string_type</code>
     */
    typedef std::experimental::basic_string_view<CharT, Traits> string_view_type;

    /**
     * Constructs object with optional argument to specify maximum size of the
     * destination <code>string_type</code>. By default this size is unlimited.
     * @param max_size Size limit for destination <code>string</code>
     */
    string_sink(std::size_t max_size = std::string::npos) : _max_size{max_size} {}

    /**
     * Writes first <code>n</code> character from the array <code>s</code> into
     * destination <code>string</code>
     * @param s Pointer to an array of at least <code>n</code> elements
     * @param n Number of characters to write.
     * @return Number of successefully written characters
     */
    std::streamsize write(const CharT* s, std::streamsize n)
    {
        if (_max_size <= _chars_written) return 0;
        if (_chars_written + n > _max_size)
        {
            std::streamsize ln = _max_size - _chars_written;
            _buffer.append(s, ln);
            _chars_written = _max_size;
            return ln;
        }
        _buffer.append(s, n);
        _chars_written += n;
        return n;
    }
    /**
     * Sets <code>flushed</code> flag to <code>true</code>
     */
    void flush() { _flushed = true; }

    /**
     * Resets the sink.
     */
    void reset() { _buffer.clear(); _flushed = false; _chars_written = 0; }
    /**
     * Returns <code>string_view</code> to the destination <code>std::string</code>
     * @return <code>view</code> of the destination string.
     */
    string_view_type view() const { return string_view_type{_buffer}; }
    /**
     * Returns the reference to the destination <code>std::string</code>
     * @return reference to the destination string
     */
    string_type& str() { return _buffer; }
    /**
     * Return flag which indicates whether this sink was flushed since last reset
     * @return <code>true</code> if this sink was flushed since last reset
     */
    bool was_flushed() const { return _flushed; }
    /**
     * Return the number of character which were written into this sink
     * since last reset
     * @return the number of characters written to this sink since last reset.
     */
    std::size_t characters_written() const { return _chars_written; }

private:
    string_type _buffer;
    std::size_t _max_size;
    std::size_t _chars_written = 0;
    bool _flushed = false;
};

/**
 * Definition if <code>basic_outstream</code> with <code>string_sink</code>
 */
template<typename CharT, typename Traits = std::char_traits<CharT>>
using basic_inplace_ostream = basic_outstream<string_sink<CharT, Traits>, non_buffered, CharT, Traits>;
/**
 * Definition of <code>basic_inplace_ostream</code> with <code>char</code> type.
 */
typedef basic_inplace_ostream<char> inplace_ostream;

} // end of servlet namespace

#endif //MOD_SERVLET_IO_STRING_H
