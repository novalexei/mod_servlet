/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_IO_FILTER_H
#define MOD_SERVLET_IO_FILTER_H

#include <servlet/lib/io.h>

/**
 * @file io_filter.h
 * @brief Definitions for input and output filters and related types.
 */

namespace servlet
{

/**
 * Abstract interface for generic sink
 * @tparam CharT character type
 * @see sink
 */
template <typename CharT>
struct basic_sink
{
    /**
     * Virtual constructor
     */
    virtual ~basic_sink() noexcept = default;
    /**
     * Writes the first <code>n</code> characters of array <code>s</code>
     * into the sink.
     * @param s Pointer to an array of at least <code>n</code> elements
     * @param n Number of characters to write.
     * @return Number of successefully written characters
     */
    virtual std::streamsize write(CharT* s, std::streamsize n) = 0;
    /**
     * Flush sink if it can be flushed. Otherwise do  nothing.
     */
    virtual void flush() {}
};
/**
 * Abstract interface for generic source
 * @tparam CharT character type
 * @see source
 */
template <typename CharT>
struct basic_source
{
    /**
     * Virtual constructor
     */
    virtual ~basic_source() noexcept = default;
    /**
     * Extracts at most <code>n</code> characters from the source into
     * the provided <code>s</code> array.
     * @param s Pointer to an array where the extracted characters are stored.
     * @param n Number of characters to extract.
     * @return Number of characters actually extracted.
     */
    virtual std::streamsize read(CharT* s, std::streamsize n) = 0;
};
/**
 * Abstract interface for output filter.
 * @tparam CharT character type
 */
template <typename CharT>
struct basic_out_filter
{
    /**
     * Virtual destructor
     */
    virtual ~basic_out_filter() noexcept = default;
    /**
     * Filters first <code>n</code> characters from the buffer <code>s</code>
     * and passes the filtered characters to <code>dst</code>
     * @param s Pointer to an array of at least <code>n</code> elements
     * @param n Number of characters to filter.
     * @param dst Destination sink to write the filtered output to.
     * @return Number of actually written characters
     */
    virtual std::streamsize write(CharT* s, std::streamsize n, basic_sink<CharT>& dst) = 0;
};
/**
 * Abstract interface for input filter.
 * @tparam CharT character type
 */
template <typename CharT>
struct basic_in_filter
{
    /**
     * Virtual destructor
     */
    virtual ~basic_in_filter() noexcept = default;
    /**
     * Extracts the characters from the source, filters the input and
     * copies at most <code>n</code> characters to the provided
     * <code>s</code> array.
     * @param s Pointer to an array of at least <code>n</code> elements to
     *          which output of this filter will be copied.
     * @param n Number of characters to copy to the output array <code>s</code>
     * @param src Source from which filter acquires the data.
     * @return Number of actually copied characters
     */
    virtual std::streamsize read(CharT* s, std::streamsize n, basic_source<CharT>& src) = 0;
};

/**
 * Type definition for input filter with <code>char</code>.
 */
using in_filter = basic_in_filter<char>;
/**
 * Type definition for output filter with <code>char</code>.
 */
using out_filter = basic_out_filter<char>;

template<typename CharT>
class out_filter_adapter : public basic_sink<CharT>
{
public:
    out_filter_adapter(basic_sink<CharT>& dst, basic_out_filter<CharT>* filter, bool filter_owner) :
            _sink{dst}, _filter{filter}, _filter_owner{filter_owner} {}
    ~out_filter_adapter() noexcept override { if (_filter_owner) delete _filter; }

    std::streamsize write(CharT* s, std::streamsize n) override { return _filter->write(s, n, _sink); }
private:
    basic_sink<CharT>& _sink;
    basic_out_filter<CharT>* _filter;
    bool _filter_owner;
};

/**
 * Implementation of filtered sink.
 *
 * This implementation supports multiple filters arranged as a chain.
 * @tparam CharT character type
 * @see basic_sink
 */
template <typename CharT>
class basic_filtered_sink : public basic_sink<CharT>
{
public:
    /**
     * Constructs the object with the sink to which all filtered data
     * will be passed.
     *
     * When this object is destroyed it will delete the sink as well.
     * @param sink Last sink to which all the filtered data will be passed
     */
    explicit basic_filtered_sink(basic_sink<CharT>* sink) : _sink{sink} { _front_sink = _sink; }
    /**
     * Destroys this object
     */
    ~basic_filtered_sink() noexcept override { delete _sink; }

    std::streamsize write(CharT* s, std::streamsize n) override { return _front_sink->write(s, n); }
    void flush() override { _sink->flush(); }

    /**
     * Adds new filter to the end of the filter chain.
     * @param filter Ponter to filter to add to the filter chain
     * @param delete_after_use if this flag is <code>true</code> than the filter
     *                         will be deleted on destruction of this object
     */
    void add_filter(basic_out_filter<CharT>* filter, bool delete_after_use = true)
    {
        if (_filters.empty()) _filters.emplace_back(*_sink, filter, delete_after_use);
        else _filters.emplace_back(_filters.back(), filter, delete_after_use);
        _front_sink = &_filters.back();
    }
private:
    basic_sink<CharT>* _sink;
    std::vector<out_filter_adapter<CharT>> _filters;
    basic_sink<CharT>* _front_sink;
};

template<typename CharT>
class in_filter_adapter : public basic_source<CharT>
{
public:
    in_filter_adapter(basic_source<CharT>& src, basic_in_filter<CharT>* filter, bool filter_owner) :
            _source{src}, _filter{filter}, _filter_owner{filter_owner} {}
    ~in_filter_adapter() noexcept override { if (_filter_owner) delete _filter; }

    std::streamsize read(CharT* s, std::streamsize n) override { return _filter->read(s, n, _source); }
private:
    basic_source<CharT>& _source;
    basic_in_filter<CharT>* _filter;
    bool _filter_owner;
};

/**
 * Implementation of filtered source.
 *
 * This implementation supports multiple filters arranged as a chain.
 * @tparam CharT character type
 * @see basic_source
 */
template <typename CharT>
class basic_filtered_source : public basic_source<CharT>
{
public:
    /**
     * Constructs the object with the source from which all the data to
     * be filtered will be acquired.
     *
     * When this object is destroyed it will delete the source as well.
     * @param src Last sink to which all the filtered data will be passed
     */
    explicit basic_filtered_source(basic_source<CharT>* src) : _source{src} { _front_source = _source; }
    ~basic_filtered_source() noexcept override { delete _source; }

    std::streamsize read(CharT* s, std::streamsize n) override { return _front_source->read(s, n); }

    /**
     * Adds new filter to the end of the filter chain.
     * @param filter Ponter to filter to add to the filter chain
     * @param delete_after_use if this flag is <code>true</code> than the filter
     *                         will be deleted on destruction of this object
     */
    void add_filter(basic_in_filter<CharT>* filter, bool delete_after_use = true)
    {
        if (_filters.empty()) _filters.emplace_back(*_source, filter, delete_after_use);
        else _filters.emplace_back(_filters.back(), filter, delete_after_use);
        _front_source = &_filters.back();
    }
private:
    basic_source<CharT>* _source;
    std::vector<in_filter_adapter<CharT>> _filters;
    basic_source<CharT>* _front_source;
};

/**
 * Implementation of sink which will pass all the data to a
 * <code>std::basic_ostream</code>.
 * @tparam CharT character type
 * @see basic_sink
 */
template <typename CharT>
class basic_stream_sink : public basic_sink<CharT>
{
public:
    /**
     * Constructs new object with a given <code>std::basic_ostream</code>
     * to write data to.
     * @param out out stream to send the data to.
     */
    basic_stream_sink(std::basic_ostream<CharT>& out) : _out{out} {}
    std::streamsize write(CharT* s, std::streamsize n) override
    {
        _out.write(s, n);
        return _out ? n : 0;
    }
private:
    std::basic_ostream<CharT>& _out;
};

/**
 * Implementation of source which will acquire all the data from a
 * <code>std::basic_istream</code>.
 * @tparam CharT character type
 * @see basic_sink
 */
template <typename CharT>
class basic_stream_source : public basic_source<CharT>
{
public:
    /**
     * Constructs new object with a given <code>std::basic_istream</code>
     * to read data from.
     * @param in input stream to read the data from.
     */
    basic_stream_source(std::basic_istream<CharT>& in) : _in{in} {}
    std::streamsize read(CharT* s, std::streamsize n) override
    {
        _in.read(s, n);
        return _in.gcount();
    }
private:
    std::basic_istream<CharT>& _in;
};

/**
 * Definition of basic_stream_sink with <code>char</code> type.
 */
using stream_sink = basic_stream_sink<char>;
/**
 * Definition of basic_stream_source with <code>char</code> type.
 */
using stream_source = basic_stream_source<char>;

/**
 * Definition for generic filtered output stream.
 * It is implemented as basic_outstream with basic_filtered_sink.
 */
template <typename CharT, typename Buffering = buffer_1k>
using basic_filtered_outstream = basic_outstream<basic_filtered_sink<CharT>, Buffering, CharT>;
/**
 * Definition for generic filtered input stream.
 * It is implemented as basic_oinstream with basic_filtered_source.
 */
template <typename CharT, typename Buffering = buffer_1k>
using basic_filtered_instream = basic_instream<basic_filtered_source<CharT>, Buffering, CharT>;

/**
 * Definition of basic_filtered_outstream with <code>char</code> type.
 */
using filtered_outstream = basic_filtered_outstream<char>;
/**
 * Definition of basic_filtered_instream with <code>char</code> type.
 */
using filtered_instream = basic_filtered_instream<char>;

} // end of servlet namespace

#endif // MOD_SERVLET_IO_FILTER_H
