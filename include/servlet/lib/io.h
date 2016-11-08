/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_IO_H
#define SERVLET_IO_H

#include <type_traits>
#include <iostream>
#include <vector>

#include <experimental/type_traits>

/**
 * @file io.h
 * @brief Definitions for custom implementation of <code>std::istream</code>
 * and <code>std::ostream</code> objects.
 *
 * This recembles to the project boost::iostreams, but provides new time ot sink and
 * source - buffer provider, which provides the buffer directly to <code>inbuf</code>
 * and <code>outbuf</code> which eliminates unnecessary copying.
 *
 * @see <a href="http://www.boost.org/doc/libs/1_62_0/libs/iostreams/doc/index.html">
 * <i>boost::iostreams</i></a>,
 */

namespace servlet {

/**
 * Template class holder of the buffer size.
 */
template<std::size_t BufSize>
struct buffering
{
    /**
     * Size of buffer as constant expression.
     */
    static constexpr std::size_t buf_size = BufSize;
};

/**
 * Type definition for zero buffer.
 */
typedef buffering<0>    non_buffered;
/**
 * Type definition for 1Kb buffer.
 */
typedef buffering<1024> buffer_1k;
/**
 * Type definition for 2Kb buffer.
 */
typedef buffering<2048> buffer_2k;
/**
 * Type definition for 4Kb buffer.
 */
typedef buffering<4096> buffer_4k;
/**
 * Type definition for 8Kb buffer.
 */
typedef buffering<8192> buffer_8k;

/**
 * Sink tag.
 *
 * This tag is used for the regular sinks provided to
 * <code>servlet::basic_outstream</code>. This is default tag and it
 * is not required to define it. The object with this tag is expected
 * to have the following (optional) <code>category</code> definition:
 *
 * ~~~~~{.cpp}
 * typedef sink category;
 * ~~~~~
 *
 * and the following methods:
 *
 * ~~~~~{.cpp}
 * std::streamsize write(const CharT* s, std::streamsize n);
 * void flush();
 * ~~~~~
 *
 * Method <code>write</code> will attempt to write <code>n</code> characters
 * from provided buffer <code>s</code> and return the number of butes
 * successefully written.
 *
 * Method <code>flush</code> will flush the underlying stream or do nothing
 * if the stream cannot be flushed.
 */
struct sink {};
/**
 * Source tag.
 *
 * This tag is used for the regular sources provided to
 * <code>servlet::basic_instream</code>. This is default tag and it
 * is not required to define it. The object with this tag is expected
 * to have the following (optional) <code>category</code> definition:
 *
 * ~~~~~{.cpp}
 * typedef source category;
 * ~~~~~
 *
 * and the following method:
 *
 * ~~~~~{.cpp}
 * std::streamsize read(const CharT* s, std::streamsize n);
 * ~~~~~
 *
 * Method <code>read</code> will read from the underlying stream into
 * buffer <code>s</code> up to <code>n</code> characters and return the
 * actual number of characters read.
 */
struct source {};
/**
 * Source tag.
 *
 * This tag is used to indicate that this object can be used as buffer
 * provider for both <code>servlet::basic_instream</code> and
 * <code>servlet::basic_outstream</code>. The object with this tag is
 * expected to have the following <code>category</code> definition:
 *
 * ~~~~~{.cpp}
 * typedef buffer_provider category;
 * ~~~~~
 *
 * and the following methods (<code>flush</code> is required only for
 * <code>servlet::basic_outstream</code>):
 *
 * ~~~~~{.cpp}
 * std::pair<CharT*, std::size_t> get_buffer();
 * void flush();
 * ~~~~~
 *
 * Method <code>get_buffer</code> returns <code>std::pair</code> containing
 * buffer for the stream buffer to use and number of charcters available
 * for the stream to use.
 *
 * Method <code>flush</code> is required only if the buffer_provider is used
 * with <code>servlet::basic_outstream</code> and it flushed underlying stream
 * if needed.
 */
struct buffer_provider {};

template<typename Sink, typename Buffering, typename CharT, typename Traits, typename Category = void>
class basic_outbuf;

template<typename Sink, typename Buffering, typename CharT, typename Traits, typename Category>
class basic_outbuf : public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT                     char_type;
    typedef Traits                    traits_type;
    typedef typename Traits::int_type int_type;
    typedef typename Traits::pos_type pos_type;
    typedef typename Traits::off_type off_type;

    typedef std::basic_ostream<char_type, traits_type>  _super_type;

    template<class... Args>
    basic_outbuf(Args &&... args) : _sink{std::forward<Args>(args)...}, _buffer{new CharT[Buffering::buf_size]}
    {
        this->setp(_buffer, _buffer + Buffering::buf_size - 1);
    }

    basic_outbuf(const basic_outbuf& other) = delete;
    basic_outbuf(basic_outbuf&& ) = delete;

    ~basic_outbuf() noexcept override { sync(); delete[] _buffer; }

    basic_outbuf& operator=(const basic_outbuf& ) = delete;
    basic_outbuf& operator=(basic_outbuf&& ) = delete;

    Sink& operator*() { return _sink; }
    Sink* operator->() { return &_sink; }

    const Sink& operator*() const { return _sink; }
    const Sink* operator->() const { return &_sink; }

    void reset() { this->setp(_buffer, _buffer + Buffering::buf_size - 1); }

protected:
    int_type overflow(int_type ch) override
    {
        _buffer[Buffering::buf_size-1] = static_cast<char>(ch);
        this->setp(_buffer, _buffer + Buffering::buf_size - 1);
        return _sink.write(_buffer, Buffering::buf_size) < Buffering::buf_size ? traits_type::eof() : ch;
    }

    int sync() override
    {
        std::size_t size = this->pptr() - _buffer;
        if (size > 0)
        {
            if (_sink.write(_buffer, static_cast<std::streamsize>(size)) < size) return -1;
        }
        _sink.flush();
        this->setp(_buffer, _buffer + Buffering::buf_size - 1);
        return 0;
    }

private:
    Sink _sink;
    CharT *_buffer;
};

template<typename Sink, typename CharT, typename Traits>
class basic_outbuf<Sink, non_buffered, CharT, Traits,
                   typename std::enable_if_t<std::is_same<typename Sink::category, buffer_provider>::value>> :
        public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT                     char_type;
    typedef Traits                    traits_type;
    typedef typename Traits::int_type int_type;
    typedef typename Traits::pos_type pos_type;
    typedef typename Traits::off_type off_type;

    typedef std::basic_ostream<char_type, traits_type>  _super_type;

    template<class... Args>
    basic_outbuf(Args &&... args) : _sink{std::forward<Args>(args)...} {}

    basic_outbuf(const basic_outbuf& other) = delete;
    basic_outbuf(basic_outbuf&& ) = delete;

    ~basic_outbuf() noexcept override { sync(); }

    basic_outbuf& operator=(const basic_outbuf& ) = delete;
    basic_outbuf& operator=(basic_outbuf&& ) = delete;

    Sink& operator*() { return _sink; }
    Sink* operator->() { return &_sink; }

    const Sink& operator*() const { return _sink; }
    const Sink* operator->() const { return &_sink; }

    void reset() { this->setp(this->pbase(), this->epptr()); }

protected:
    int_type overflow(int_type ch) override
    {
        *this->pptr() = static_cast<char>(ch);
        std::pair<CharT*, std::size_t> buffer = _sink.get_buffer();
        if (!buffer.first || buffer.second <= 0) return traits_type::eof();
        this->setp(buffer.first, buffer.first + buffer.second - 1);
        return ch;
    }

    int sync() override
    {
        _sink.flush(this->pptr() - this->pbase());
        return 0;
    }

private:
    Sink _sink;
};

template<typename Sink, typename CharT, typename Traits, typename Category>
class basic_outbuf<Sink, non_buffered, CharT, Traits, Category> : public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT                     char_type;
    typedef Traits                    traits_type;
    typedef typename Traits::int_type int_type;
    typedef typename Traits::pos_type pos_type;
    typedef typename Traits::off_type off_type;

    template <class... Args>
    basic_outbuf(Args&&... args) : _sink{std::forward<Args>(args)...} {}

    basic_outbuf(const basic_outbuf& ) = default;
    basic_outbuf(basic_outbuf&& ) = default;

    basic_outbuf& operator=(const basic_outbuf& ) = default;
    basic_outbuf& operator=(basic_outbuf&& ) = default;

    void reset() { }

    Sink& operator*() { return _sink; }
    Sink* operator->() { return &_sink; }

    const Sink& operator*() const { return _sink; }
    const Sink* operator->() const { return &_sink; }

protected:
    int_type overflow(int_type ch) override
    {
        char_type tmp_ch = static_cast<char_type>(ch);
        return _sink.write(&tmp_ch, 1) == 1 ? ch : traits_type::eof();
    }

    int sync() override { _sink.flush(); return 0; }

    std::streamsize xsputn(const char_type* s, std::streamsize n) override { return _sink.write(s, n); }

private:
    Sink _sink;
};


/**
 * Implementation of output stream object.
 *
 * This class inherits from <code>std::basic_ostream</code> and can
 * be used as one.
 *
 * This object uses the <code>Sink</code> provided as a template parameter
 * to write data to. The <code>Sink</code> can be either sink or
 * buffer_provider.
 *
 * @tparam Sink sink object to use to write data.
 * @tparam Buffering Buffer size to be used.
 * @tparam CharT character type for this stream
 * @tparam Traits cahracter traits type to be used in this stream.
 *
 * @see sink
 * @see buffer_provider
 */
template<typename Sink, typename Buffering, typename CharT, typename Traits = std::char_traits<CharT>>
class basic_outstream : public std::basic_ostream<CharT, Traits>
{
    typedef basic_outbuf<Sink, Buffering, CharT, Traits> _outbuf_type;
    typedef std::basic_ostream<CharT, Traits>            _ostream_type;
public:
    /**
     * Cacracter type to be used by this stream.
     */
    typedef CharT 					       char_type;
    /**
     * Type of character traits class to be used by this stream
     */
    typedef Traits 					       traits_type;
    /**
     * traits_type::int_type
     */
    typedef typename traits_type::int_type int_type;
    /**
     * traits_type::pos_type
     */
    typedef typename traits_type::pos_type pos_type;
    /**
     * traits_type::off_type
     */
    typedef typename traits_type::off_type off_type;

    /**
     * Main constructor.
     *
     * This generic constructor will pass received arguments to its buffer, which
     * in turn will pass them to <code>Sink</code>. Thus, generally, these arguments
     * should be the ones to create the <code>Sink</code>
     * @tparam Args types of the arguments to forward to <code>Sink</code> constructor
     * @param args Arguments to be forwarded to construct the <code>Sink</code>
     */
    template <class... Args>
    explicit basic_outstream(Args&&... args) : _ostream_type{}
    {
        this->rdbuf(new _outbuf_type{std::forward<Args>(args)...});
    }
    /**
     * Move constructor
     * @param other <code>basic_outstream</code> which contents and state will be acquired
     *              by created object.
     */
    basic_outstream(basic_outstream&& other) : _ostream_type{}
    {
        this->rdbuf(other.rdbuf());
        other.rdbuf(nullptr);
    }
    /**
     * Move assignment operator
     * @param other <code>basic_outstream</code> which contents and state will be acquired
     *              by created object.
     * @return reference to self
     */
    basic_outstream& operator=(basic_outstream&& other)
    {
        this->rdbuf(other.rdbuf());
        other.rdbuf(nullptr);
        return *this;
    }

    /**
     * Destructor.
     */
    ~basic_outstream() noexcept override { delete this->rdbuf(); }

    /**
     * Provides access to the reference to the <code>Sink</code> instance
     * associated with this stream.
     * @return reference to the <code>Sink</code> instance.
     */
    Sink& operator*() { return buf()->operator*(); }
    /**
     * Provides access to the pointer to the <code>Sink</code> instance
     * associated with this stream.
     * @return pointer to the <code>Sink</code> instance.
     */
    Sink* operator->() { return buf()->operator->(); }

    /**
     * Provides access to the constant reference to the <code>Sink</code>
     * instance associated with this stream.
     * @return const reference to the <code>Sink</code> instance.
     */
    const Sink& operator*() const { return buf()->operator*(); }
    /**
     * Provides access to the constant pointer to the <code>Sink</code>
     * instance associated with this stream.
     * @return const pointer to the <code>Sink</code> instance.
     */
    const Sink* operator->() const { return buf()->operator->(); }

//    void reset_buffer() { return buf()->reset(); }

private:
    inline _outbuf_type *buf() { return static_cast<_outbuf_type*>(this->rdbuf()); }
    inline const _outbuf_type *buf() const { return static_cast<const _outbuf_type*>(this->rdbuf()); }
};

template<typename Source, typename Buffering, typename CharT, typename Traits, typename Enable = void>
class basic_inbuf;

template<typename Source, typename Buffering, typename CharT, typename Traits, typename Enable>
class basic_inbuf : public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT                     char_type;
    typedef Traits                    traits_type;
    typedef typename Traits::int_type int_type;
    typedef typename Traits::pos_type pos_type;
    typedef typename Traits::off_type off_type;

    template <class... Args>
    basic_inbuf(Args&&... args) : _source{std::forward<Args>(args)...}, _buffer{new char_type[Buffering::buf_size]} {}

    ~basic_inbuf() noexcept override { delete[] _buffer; }

    basic_inbuf(const basic_inbuf& ) = delete;
    basic_inbuf(basic_inbuf&& other) = delete;

    basic_inbuf& operator=(const basic_inbuf& ) = delete;
    basic_inbuf& operator=(basic_inbuf&& ) = delete;

    Source& operator*() { return _source; }
    Source* operator->() { return &_source; }

    const Source& operator*() const { return _source; }
    const Source* operator->() const { return &_source; }

    void reset() { }

protected:
    int_type underflow() override
    {
        std::streamsize new_size = _source.read(_buffer, Buffering::buf_size);
        if (new_size == 0) return traits_type::eof();
        this->setg(_buffer, _buffer, _buffer + new_size);
        return traits_type::to_int_type(*_buffer);
    }

    int_type pbackfail(int_type ch) override
    {
        if (this->egptr() <= this->eback()) return traits_type::eof();
        this->gbump(-1);
        return ch;
    }

private:
    Source _source;
    CharT *_buffer;
};

template<typename Source, typename CharT, typename Traits>
class basic_inbuf<Source, non_buffered, CharT, Traits,
                  typename std::enable_if_t<std::is_same<typename Source::category, buffer_provider>::value>> :
        public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT                     char_type;
    typedef Traits                    traits_type;
    typedef typename Traits::int_type int_type;
    typedef typename Traits::pos_type pos_type;
    typedef typename Traits::off_type off_type;

    template <class... Args>
    basic_inbuf(Args&&... args) : _source{std::forward<Args>(args)...} {}

    ~basic_inbuf() noexcept = default;

    basic_inbuf(const basic_inbuf& ) = delete;
    basic_inbuf(basic_inbuf&& other) = delete;

    basic_inbuf& operator=(const basic_inbuf& ) = delete;
    basic_inbuf& operator=(basic_inbuf&& ) = delete;

    Source& operator*() { return _source; }
    Source* operator->() { return &_source; }

    const Source& operator*() const { return _source; }
    const Source* operator->() const { return &_source; }

    void reset() { this->setg(this->eback(), this->eback(), this->egptr()); }

protected:
    int_type underflow() override
    {
        std::pair<CharT*, std::size_t> buffer = _source.get_buffer();
        if (!buffer.first || buffer.second <= 0) return traits_type::eof();
        this->setg(buffer.first, buffer.first, buffer.first + buffer.second);
        return traits_type::to_int_type(*this->eback());
    }

    int_type pbackfail(int_type ch) override
    {
        if (this->egptr() <= this->eback()) return traits_type::eof();
        this->gbump(-1);
        return ch;
    }

private:
    Source _source;
};

/**
 * Implementation of input stream object.
 *
 * This class inherits from <code>std::basic_istream</code> and can
 * be used as one.
 *
 * This object uses the <code>Source</code> provided as a template parameter
 * to write data to. The <code>Source</code> can be either source or
 * buffer_provider.
 *
 * @tparam Source source type to use to read data from.
 * @tparam Buffering Buffer size to be used.
 * @tparam CharT character type for this stream
 * @tparam Traits cahracter traits type to be used in this stream.
 *
 * @see source
 * @see buffer_provider
 */
template<typename Source, typename Buffering, typename CharT, typename Traits = std::char_traits<CharT>>
class basic_instream : public std::basic_istream<CharT, Traits>
{
    typedef basic_inbuf<Source, Buffering, CharT, Traits> _inbuf_type;
    typedef std::basic_istream<CharT, Traits>             _istream_type;
public:
    /**
     * Cacracter type to be used by this stream.
     */
    typedef CharT 					       char_type;
    /**
     * Type of character traits class to be used by this stream
     */
    typedef Traits 					       traits_type;
    /**
     * traits_type::int_type
     */
    typedef typename traits_type::int_type int_type;
    /**
     * traits_type::pos_type
     */
    typedef typename traits_type::pos_type pos_type;
    /**
     * traits_type::off_type
     */
    typedef typename traits_type::off_type off_type;

    /**
     * Main constructor.
     *
     * This generic constructor will pass received arguments to its buffer, which
     * in turn will pass them to <code>Source</code>. Thus, generally, these arguments
     * should be the ones to create the <code>Source</code>
     * @tparam Args types of the arguments to forward to <code>Source</code> constructor
     * @param args Arguments to be forwarded to construct the <code>Source</code>
     */
    template <typename... Args>
    explicit basic_instream(Args&&... args) : _istream_type{}
    {
        this->rdbuf(new _inbuf_type{std::forward<Args>(args)...});
    }
    /**
     * Sopying is prohibited.
     */
    basic_instream(const basic_instream& ) = delete;
    /**
     * Move constructor
     * @param other <code>basic_instream</code> which contents and state will be acquired
     *              by created object.
     */
    basic_instream(basic_instream&& other) : _istream_type{}
    {
        this->rdbuf(other.rdbuf());
        other.rdbuf(nullptr);
    }

    /**
     * Move assignment operator
     * @param other <code>basic_instream</code> which contents and state will be acquired
     *              by created object.
     * @return reference to self
     */
    basic_instream& operator=(basic_instream&& other)
    {
        this->rdbuf(other.rdbuf());
        other.rdbuf(nullptr);
        return *this;
    }

    /**
     * Destructor
     */
    ~basic_instream() noexcept override { delete this->rdbuf(); }

    /**
     * Provides access to the reference to the <code>Source</code> instance
     * associated with this stream.
     * @return reference to the <code>Source</code> instance.
     */
    Source& operator*() { return buf()->operator*(); }
    /**
     * Provides access to the pointer to the <code>Source</code> instance
     * associated with this stream.
     * @return pointer to the <code>Source</code> instance.
     */
    Source* operator->() { return buf()->operator->(); }

    /**
     * Provides access to the constant reference to the <code>Source</code>
     * instance associated with this stream.
     * @return const reference to the <code>Source</code> instance.
     */
    const Source& operator*() const { return buf()->operator*(); }
    /**
     * Provides access to the constant pointer to the <code>Source</code>
     * instance associated with this stream.
     * @return const pointer to the <code>Source</code> instance.
     */
    const Source* operator->() const { return buf()->operator->(); }

//    void reset_buffer() { return buf()->reset(); }
private:
    inline _inbuf_type* buf() { return static_cast<_inbuf_type*>(this->rdbuf()); }
    inline const _inbuf_type* buf() const { return static_cast<const _inbuf_type*>(this->rdbuf()); }
};

/**
 * Type definition for <code>basic_outstream</code> with <code>char</code> type.
 */
template <typename Sink, typename Buffering = non_buffered>
using outstream = basic_outstream<Sink, Buffering, char>;
/**
 * Type definition for <code>basic_instream</code> with <code>char</code> type.
 */
template <typename Source, typename Buffering = non_buffered>
using instream = basic_instream<Source, Buffering, char>;

} // end of servlet namespace

#endif // SERVLET_IO_H
