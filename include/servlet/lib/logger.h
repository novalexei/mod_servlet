/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef SERVLET_LOGGER_H
#define SERVLET_LOGGER_H

/**
 * @file logger.h
 * @brief Contains definitions for the logging framework
 */

#include <experimental/string_view>
#include <atomic>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <iomanip>
#include <vector>

#include <servlet/lib/io_string.h>

namespace servlet {

using std::experimental::string_view;

namespace logging {

/**
 * Abstract interface for logging output.
 */
class log_output
{
public:
    virtual ~log_output() noexcept {}

    /**
     * Writes a given <code>string_view</code> to the output
     * represented by this object.
     * @param str <code>string_view</code> to write
     */
    virtual void write_string(string_view str) = 0;
    /**
     * Flushes the output represented by this object.
     */
    virtual void flush() = 0;

    /**
     * Loads configuration from the properties provided. Only the properties with
     * a given <code>conf_prefix</code> prefix are related to this object.
     *
     * For example, if this object required property with name <code>"log.file"</code>
     * and this method is called with <code>conf_prefix</code> of <code>"basic."</code>
     * than the property with the name <code>"basic.log.file"</code> should be used to
     * configure this object.
     *
     * Argument <code>base_dir</code> might be passed to indicate the base directory
     * path to resolve relative paths which can be read from properties.
     *
     * @param props Properties to use to configure this object,
     * @param conf_prefix Prefix for the propertiy keys
     * @param base_dir Base directory to use to resolve relative file paths.
     */
    virtual void load_config(std::map<std::string, std::string, std::less<>>& props,
                             const std::string& conf_prefix, const std::string &base_dir) = 0;
};

/**
 * Abstract interface for <code>log_output</code> factory.
 */
struct log_output_factory
{
    virtual ~log_output_factory() noexcept {}
    /**
     * Creates new <code>log_output</code> object and returns pointer to it.
     * @return pointer to new <code>log_output</code> object
     */
    virtual log_output* new_log_output() = 0;
};

/**
 * Enumeration of supported synchronization policies.
 */
enum class SYNC_POLICY
{
    /**
     * Indicates that the logging will be used only in single threaded
     * envoronment and no synchronization is needed.
     */
    SINGLE_THREAD,
    /**
     * Indicates that the loggin will be used in multithreaded environment
     * and synchronization between thread necessary.
     *
     * This policy will use <code>std::mutex</code> to synchronize between threads.
     * This is a default policy.
     */
    SYNC,
    /**
     * Indicates that the loggin will be used in multithreaded environment
     * and synchronization between thread necessary.
     *
     * This policy will spawn one thread which will control the output. This method
     * sometimes prefered as it will never lock working threads, but it is heavier
     * method as it uses lock-free collections and requires extra control thread to
     * handle the output.
     */
    ASYNC
};

/**
 * Enumeration of supported logging levels
 */
enum class LEVEL
{
    /**
     * Critical error level
     */
    CRITICAL,
    /**
     * Error level
     */
    ERROR,
    /**
     * Warning level
     */
    WARNING,
    /**
     * Information level
     */
    INFO,
    /**
     * Configuration level
     */
    CONFIG,
    /**
     * Debug level
     */
    DEBUG,
    /**
     * Trace level
     */
    TRACE
};

/**
 * Abstract interface for logging prefix printer.
 *
 * The instance of <code>prefix_printer</code> object is responsible
 * for the prefix printed on each line in the logging outptu (e.g. timestamp,
 * thread id, log level, logger name, etc.)
 */
class prefix_printer
{
public:
    virtual ~prefix_printer() noexcept {}

    /**
     * Prints prefix to the log output.
     * @param level Logging level used by logger for which the prefix is printed.
     * @param name name of logger for which is prefix printed.
     * @param out <code>std::ostream</code> to print the prefix to.
     */
    virtual void print_prefix(LEVEL level, const std::string &name, std::ostream &out) const = 0;
    /**
     * Loads configuration from the properties provided. Only the properties with
     * a given <code>conf_prefix</code> prefix are related to this object.
     *
     * For example, if this object required property with name <code>"time.format"</code>
     * and this method is called with <code>conf_prefix</code> of <code>"simple."</code>
     * than the property with the name <code>"simple.time.format"</code> should be used to
     * configure this object.
     *
     * @param props Properties to use to configure this object,
     * @param conf_prefix Prefix for the propertiy keys
     */
    virtual void load_config(std::map<std::string, std::string, std::less<>>& props, const std::string& conf_prefix) = 0;
};

/**
 * Abstract interface for <code>prefix_printer</code> factory.
 */
struct prefix_printer_factory
{
    virtual ~prefix_printer_factory() noexcept {}
    /**
     * Creates new <code>prefix_printer</code> object and returns pointer to it.
     * @return pointer to new <code>prefix_printer</code> object
     */
    virtual prefix_printer* new_prefix_printer() = 0;
};

class logger;

/**
 * Logger with specific logging <code>LEVEL</code> to use.
 *
 * This logger has overloaded streaming output operators which will output
 * the object into the underlying stream. It also has the special treatment
 * for international characters and UTF strings. It convers <code>UTF-16</code>
 * and <code>UTF-32</code> strings into <code>UTF_8</code> and prints them.
 *
 * For example the following output will be hadled correctly:
 *
 * ~~~~~{.cpp}
 *  auto lg = servlet::get_logger();
 *  std::u16string u16str{u"\u4f60\u597d"};
 *  std::u32string u32str{U"の場合"};
 *  lg->warning() << str << std::endl;
 *  lg->warning() << "utf-16 string: " << u16str << std::endl;
 *  lg->warning() << "utf-16 array: " << u"鵝滿是快烙滴好耳痛" << std::endl;
 *  lg->warning() << "utf-32 string: " << u32str << std::endl;
 *  lg->warning() << "utf-32 array: " << U"野家のコピペ" << std::endl;
 * ~~~~~
 *
 * @see logger.log
 * @see logger.critical
 * @see logger.error
 * @see logger.warning
 * @see logger.info
 * @see logger.config
 * @see logger.debug
 * @see logger.trace
 */
class level_logger
{
public:
    ~level_logger() noexcept;

    /**
     * Generic streaming output operator. The <code>value</code> will
     * be passed to actual output stream if the logging level is sufficient
     * for the object to be printed.
     * @tparam T type of the value to send to a stream.
     * @param value object to print.
     * @return reference to self
     */
    template<typename T>
    inline level_logger &operator<<(const T &value);

    /**
     * Streaming output operator for manipulators (e.g. <code>std::endl</code>).
     * @param manip mainpulator to print.
     * @return reference to self
     */
    level_logger &operator<<(std::ostream &(*manip)(std::ostream &));

    /**
     * Streaming output operator for <code>char</code>. This method does nothing
     * if the logging level is not sufficient.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &operator<<(char ch);
    /**
     * Streaming output operator for <code>char16_t</code>. This method does nothing
     * if the logging level is not sufficient.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &operator<<(char16_t ch);
    /**
     * Writes C-style array of character of type <code>char16_t</code> to the output.
     * This method does nothing if the logging level is not sufficient.
     *
     * This string is handled as string of <code>UTF-16</code> characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str C-array of characters to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const char16_t* str);
    /**
     * Writes string of character of type <code>char16_t</code> to the output.
     * This method does nothing if the logging level is not sufficient.
     *
     * This string is handled as string of <code>UTF-16</code> characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str <code>UTF-16</code> string to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const std::u16string& str);
    /**
     * Streaming output operator for <code>char32_t</code>. This method does nothing
     * if the logging level is not sufficient.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &operator<<(char32_t ch);
    /**
     * Writes C-style array of character of type <code>char32_t</code> to the output.
     * This method does nothing if the logging level is not sufficient.
     *
     * This string is handled as string of <code>UTF-32</code> characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str C-array of characters to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const char32_t* str);
    /**
     * Writes string of character of type <code>char32_t</code> to the output.
     * This method does nothing if the logging level is not sufficient.
     *
     * This string is handled as string of <code>UTF-32</code> characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str <code>UTF-32</code> string to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const std::u32string& str);
    /**
     * Streaming output operator for <code>wchar_t</code>. This method does nothing
     * if the logging level is not sufficient.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &operator<<(wchar_t ch);
    /**
     * Writes C-style array of character of type <code>wchar_t</code> to the output.
     * This method does nothing if the logging level is not sufficient.
     *
     * This string is handled as string of wide characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str C-array of characters to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const wchar_t* str);
    /**
     * Writes string of wide characters to the output. This method does nothing if
     * the logging level is not sufficient.
     *
     * This string is handled as string of wide characters. They are
     * converted to <code>UTF-8</code> before being passed to the output stream.
     * @param str <code>wchar_t</code> string to write to the output stream
     * @return reference to self.
     */
    level_logger &operator<<(const std::wstring& str);

    /**
     * Flushes underlying stream.
     * @return reference to self
     */
    level_logger &flush() { _out->flush(); return *this; }

    /**
     * Puts a character of type <code>wchar</code> to output if the
     * logging level is sufficient. Otherwise does nothing.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &put(char ch) { return (*this) << ch; }
    /**
     * Puts a character of type <code>char16_t</code> to output if the
     * logging level is sufficient. Otherwise does nothing.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &put(char16_t ch) { return (*this) << ch; }
    /**
     * Puts a character of type <code>char32_t</code> to output if the
     * logging level is sufficient. Otherwise does nothing.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &put(char32_t ch) { return (*this) << ch; }
    /**
     * Puts a character of type <code>wchar_t</code> to output if the
     * logging level is sufficient. Otherwise does nothing.
     * @param ch character to print.
     * @return reference to self
     */
    level_logger &put(wchar_t ch) { return (*this) << ch; }

    /**
     * Writes first <code>n</code> characters of array <code>s</code> to
     * the output stream if the logging level is sufficient.
     * @param s Array of at least <code>n</code> characters.
     * @param n Number of characters to write.
     * @return reference to self.
     */
    level_logger &write (const char* s, std::streamsize n);
private:
    friend class logger;
    level_logger(LEVEL level, logger &log) : _log_level{level}, _logger{log} {}

    LEVEL _log_level;
    logger &_logger;
    inplace_ostream *_out = nullptr;
};

class locked_stream
{
public:
    virtual ~locked_stream() noexcept {}

    virtual inplace_ostream *get_buffer(const std::locale& loc) = 0;
    virtual void return_buffer(inplace_ostream *buf) = 0;
};

class log_registry;

/**
 * Logger class.
 *
 * Every logger can have the following attributes:
 *
 * <ol>
 *   <li><code>name</code> - name of the logger.</li>
 *   <li><code>locale</code> - locale which is used for the log output stream associated with this logger</li>
 *   <li><code>level</code> - logging level for this logger</li>
 *   <li><code>prefix_printer</code> - prefix printer to use with this logger</li>
 *   <li><code>log_output</code> - log output to which this logger will send its messages</li>
 * </ol>
 *
 * All this attributes (except <code>name</code>) can be changed.
 *
 * @see log_output
 * @see prefix_printer
 * @see LEVEL
 * @see level_logger
 */
class logger
{
public:
    ~logger() noexcept = default;

    /**
     * Returns name of this logger.
     * @return logger's name
     */
    const std::string& name() const { return _name; }

    /**
     * Creates new level_logger with logging level <code>LEVEL::CRITICAL</code>
     * @return new level_logger
     */
    level_logger critical() { return level_logger{LEVEL::CRITICAL, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::ERROR</code>
     * @return new level_logger
     */
    level_logger error() { return level_logger{LEVEL::ERROR, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::WARNING</code>
     * @return new level_logger
     */
    level_logger warning() { return level_logger{LEVEL::WARNING, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::INFO</code>
     * @return new level_logger
     */
    level_logger info() { return level_logger{LEVEL::INFO, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::CONFIG</code>
     * @return new level_logger
     */
    level_logger config() { return level_logger{LEVEL::CONFIG, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::DEBUG</code>
     * @return new level_logger
     */
    level_logger debug() { return level_logger{LEVEL::DEBUG, *this}; }
    /**
     * Creates new level_logger with logging level <code>LEVEL::TRACE</code>
     * @return new level_logger
     */
    level_logger trace() { return level_logger{LEVEL::TRACE, *this}; }

    /**
     * Creates new level_logger with a provided logging level.
     * @return new level_logger
     */
    level_logger log(LEVEL level) { return level_logger{level, *this}; }
    /**
     * Tests if the requested level is loggable.
     * @param level Logging level to test for loggability
     * @return <code>true</code> if a given level is loggable,
     *         <code>false</code> otherwise.
     */
    bool is_loggable(LEVEL level) const { return level <= _log_level.load(); }

    /**
     * Sets new new minimum logging level for this logger. Only log messages with
     * loggin level higher than <code>log_level</code> will be logged. The
     * rest messages will be discarded.
     * @param log_level New logging level for this logger.
     */
    void set_log_level(LEVEL log_level) { _log_level = log_level; }
    /**
     * Sets new <code>prefix_printer</code> for this logger.
     * @param new_pp new <code>prefix_printer</code> to be used by this logger
     */
    void set_prefix_printer(std::shared_ptr<prefix_printer> new_pp) { std::atomic_store(&_formatter, new_pp); }
    /**
     * Sets new <code>log_output</code> for this logger.
     * @param new_out New <code>log_output</code> to be used with this logger.
     * @param policy Synchronization policy to be assumed by this logger.
     */
    void set_log_output(log_output* new_out, SYNC_POLICY policy = SYNC_POLICY::SYNC);

    /**
     * Imbue locale.
     *
     * Associates <code>loc</code> to this logger as the new locale object
     * to be used with locale-sensitive operations.
     * @param loc New locale for the logger.
     */
    void imbue(const std::locale& loc) { _loc = loc; }

private:
    friend class level_logger;
    friend class log_registry;

    logger(std::shared_ptr<prefix_printer> pref, std::shared_ptr<locked_stream> lock_stream,
           LEVEL log_level, std::string name, const std::locale& loc) :
            _formatter{pref}, _lock_stream{lock_stream}, _name{std::move(name)}, _log_level{log_level}, _loc{loc} {}

    void set_locked_stream(std::shared_ptr<locked_stream> lock_stream)
    { std::atomic_store(&_lock_stream, lock_stream); }

    std::shared_ptr<prefix_printer> _formatter;
    std::shared_ptr<locked_stream> _lock_stream;

    std::atomic<LEVEL> _log_level;
    const std::string _name;
    std::locale _loc;
};

/**
 * Registry object to configure and hold available loggers.
 *
 * This object generates new <code>logger</code> objects when requested and
 * configures them as prescribed by the configuration of this registry object.
 * The configuration can be done either programatically (via methods
 * set_log_level, imbue, set_base_directory, set_synchronization_policy,
 * set_prefix_printer, set_log_output), or by properties file (methods
 * read_configuration). This object supports the following configuration parameters:
 *
 * <ul>
 * <li><code>output.handler</code> - specifies output handler to be used. It supports
 * the following ahdler types:
 * <ol>
 *   <li><code>console</code> - redirects logging output to <code>std::cout</code></li>
 *   <li><code>file</code> - redirects logging output to file. Uses the following
 *     configuration parameters:
 *   <ol>
 *     <li><code>file.log.file</code> - name of the logging file. It can be specified with
 *       either absolute path or relative, which can be resolved from <code>base_dir</code>
 *       Name of the file also can use date and index elements. For example the pattern.
 *       <tt>"log-%y-%m-%d.%NNN%.out"</tt> will resove to <tt>"log-16-22-11.001.out"</tt>
 *     <ol>
 *       <li><code>"%y"</code> - resolves to last two digits of a current year;</li>
 *       <li><code>"%Y"</code> - resolves to four digits of a current year;</li>
 *       <li><code>"%m"</code> - resolves to two digits of a current month;</li>
 *       <li><code>"%d"</code> - resolves to two digits of a current day;</li>
 *       <li><code>"%NN%"</code> - resolves to the number of this file padded to number of digits
 *         equals to number of 'N' caracters in the pattern. This element can be used only
 *         if size rotation for the file is requested (see <code>size-file</code> and
 *         <code>date-size-file</code>;</li>
 *     </ol>
 *     </li>
 *   </ol>
 *   <li><code>size-file</code> - redirects logging output to file and uses rotation by size.
 *     Plus to the <code>log.file</code> configuration parameters with <code>"size-file"</code>
 *     prefix also uses following:
 *     <ol>
 *       <li><code>size-file.rotation size</code> - maximum size of the file after which this file
 *         will be closed and new one opened with different index. Default rotation size is 16 Mb.</li>
 *     </ol>
 *   </li>
 *   <li><code>date-file</code> - redirects logging output to file and uses rotation by date
 *     (00:00:00 of each day). Requires <code>log.file</code> property with
 *     <code>"date-file"</code> prefix.</li>
 *   <li><code>date-size-file</code> - redirects logging output to file and uses rotation by both
 *     date and size.</li>
 * </ol>
 * Default <code>output.handler</code> is <code>console</code>
 * </li>
 * <li><code>prefix.printer</code> - specifies prefix printer to be used. Currently the following
 *   printer types are supported:
 *   <ol>
 *   <li><code>simple</code> - simple prefix printer which prints the line prefix accordingly to
 *     the following parameters:
 *     <ol>
 *     <li><code>simple.log.prefix.format</code> - string to be in the prefix. For example:
 *       <tt>"[%TIME%] [%NAME%] [%LEVEL%]: "</tt>. The follwing pattern elements can be used
 *       in the format:
 *       <ol>
 *       <li><code>\%TIME\%</code> - current timestamp (as prescribed by <code>"timestamp.format"</code>
 *         parameter);</li>
 *       <li><code>\%THREAD\%</code> - current thread id;</li>
 *       <li><code>\%LEVEL\%</code> - current logging level;</li>
 *       <li><code>\%NAME\%</code> - logger name;</li>
 *       </ol>
 *       The default format is <tt>"%TIME% | %THREAD% | %LEVEL% | %NAME% | "</tt>
 *     </li>
 *     <li><code>simple.timestamp.format</code> - format of the timestamp. It follows the
 *       specifications of <code>std::strftime</code> function with the addition of <code>\%ss</code>
 *       specifier which will be substituted with milliseconds. The default timestamp format is:
 *       <tt>"%y/%m/%d %H:%M:%S.%ss"</tt>. It is highly recommended to use default format as
 *       it performs mych better than custom formats.</li>
 *     </ol>
 *   </ol>
 * </li>
 * <li><code>sync.policy</code> - logging synchronization policy. It can be one of:
 *   <code>SINGLE_THREAD</code>, <code>SYNC</code> or <code>ASYNC</code>. For more informations
 *   see <code>SYNC_POLICY</code>. Default value is <code>SYNC</code></li>
 * <li><code>.level</code> - root logging level. This is a global logging level, which will be
 *   applied to all loggers which dont have logger specific configuration. Default logging level
 *   is <code>WARNING</code></li>
 * <li><code>logger-name.level</code> - logging level for logger with name
 *   <code>"logger-name"</code>.</li>
 * <li><code>locale</code> - valid locale to be used to print locale specific elements. For
 *   example <code>"uk_UA.utf8"</code></li>
 * </ul>
 *
 * Configuration can be updated runtime and provided to the registry. All parameters (except
 * <code>sync.policy</code> which cannot be changed runtime) will be updated.
 */
class log_registry
{
public:
    /**
     * Default file rotation size - 16 Mb.
     */
    static constexpr std::size_t DEFAULT_FILE_ROTATION_SIZE = 1024*1024*16;
    /**
     * Default size of asynchronous queue used if synchronization policy is
     * <code>SYNC_POLICY::ASYNC</code>
     */
    static constexpr std::size_t DEFAULT_ASYNC_QUEUE_SIZE = 1024;

    /**
     * Type definition for properties map.
     */
    typedef std::map<std::string, std::string, std::less<>> properties_type;

    /**
     * Creates registry object with default settings.
     */
    log_registry();

    /**
     * Searches for registered logger with a given name. If not found new logger with
     * this name will be created, registered with this registry and returned.
     * @param name Logger name.
     * @return Registered instance of a logger with a given name.
     */
    std::shared_ptr<logger> log(const std::string& name = "root");

    /**
     * Reads configuration file and parses properties as described in class documentation.
     * @param config_file_name Name of configuration file.
     * @param base_dir Base directory to use to resolve relative paths.
     * @param update_loggers If <code>true</code> all registered loggers will be updated
     *                       with new configuration parameters.
     */
    void read_configuration(const std::string& config_file_name,
                            const std::string& base_dir, bool update_loggers = true);
    /**
     * Reads configuration properties from provided properies map. Properties are
     * described in class documentation.
     * @param props Properties map.
     * @param base_dir Base directory to use to resolve relative paths.
     * @param update_loggers If <code>true</code> all registered loggers will be updated
     *                       with new configuration parameters.
     */
    void read_configuration(properties_type props, const std::string& base_dir, bool update_loggers = true);

    /**
     * Returns logging level for a logger with a given name. If no logger
     * present, global logging level will be returned.
     * @param name logger name
     * @return loggin glevel of requested logger.
     */
    LEVEL get_log_level(const std::string& name);
    /**
     * Sets new global logging level.
     * @param level Level to set.
     */
    void set_log_level(LEVEL level) { _log_level = level; }

    /**
     * Imbue locale.
     *
     * Associates <code>loc</code> to this registry as the new locale object
     * to be used with locale-sensitive operations. All new loggers created by
     * this registry will receive this locale.
     * @param loc New locale for the registry.
     */
    void imbue(const std::locale& loc) { _loc = loc; }

    /**
     * Sets base directory. This directory is used to resolve any relative path
     * parameters in the configuration.
     * @param base_dir New base directory.
     */
    void set_base_directory(const std::string& base_dir);
    /**
     * Sets base directory. This directory is used to resolve any relative path
     * parameters in the configuration.
     * @param base_dir New base directory.
     */
    void set_base_directory(std::string&& base_dir);

    /**
     * Returns current synchronization policy used by this registry.
     * @return this registry synchronization policy
     */
    SYNC_POLICY get_synchronization_policy() const { return _sync_policy; }
    /**
     * Sets new synchronization policy. This method will have any effect only if
     * this registry is not yet configured and no loggers are created.
     * @param sync_policy new synchronization polisy to set.
     * @throws configu_exception is thrown if loggers has already been created.
     */
    void set_synchronization_policy(SYNC_POLICY sync_policy);

    /**
     * Sets new <code>prefix_printer</code> for this registry.
     * @param pp new <code>prefix_printer</code> to be used by this logger
     */
    void set_prefix_printer(prefix_printer *pp)
    { std::atomic_store(&_prefix_printer, std::shared_ptr<prefix_printer>(pp)); }

    /**
     * Sets new <code>log_output</code> for this registry.
     * @param log_out New <code>log_output</code> to be used with this registry.
     */
    void set_log_output(log_output *log_out);

    /**
     * Traverses all created loggers and resets their configuration with new configuration
     * parameters.
     * @param update_prefix_printer if <code>true</code> updates <code>prefix_printer</code>
     *                              for loggers.
     * @param update_log_output if <code>true</code> updates <code>log_output</code>
     *                          for loggers.
     * @param update_locale if <code>true</code> updates locales for loggers.
     */
    void reset_loggers_config(bool update_prefix_printer, bool update_log_output, bool update_locale);

protected:
    /**
     * This method allows to provide custom <code>prefix_printer</code>'s in inherited registries.
     * For example if there is a custom <code>color_prefix_printer</code> it can be added in inherited
     * class as follows:
     *
     * ~~~~~{.cpp}
     * class my_log_registry
     * {
     *   my_log_registry()
     *   {
     *       add_prefix_printer_factory("color", new color_prefix_printer_factory{});
     *   }
     * };
     * ~~~~~
     * And after this it can read configuration with prefix "color."
     * @param name New <code>prefix_printer_factory</code> name.
     * @param fac <code>prefix_printer</code> factory.
     */
    void add_prefix_printer_factory(const std::string& name, prefix_printer_factory* fac)
    { _prefix_printer_factories.emplace(name, fac); }
    /**
     * This method allows to provide custom <code>log_output</code>'s in inherited registries.
     * For example if there is a custom <code>system_log_output</code> it can be added in inherited
     * class as follows:
     *
     * ~~~~~{.cpp}
     * class my_log_registry
     * {
     *   my_log_registry()
     *   {
     *       add_log_output_factory("sys", new system_log_output_factory{});
     *   }
     * };
     * ~~~~~
     * And after this it can read configuration with prefix "sys."
     * @param name New <code>log_output_factory</code> name.
     * @param fac <code>log_output</code> factory.
     */
    void add_log_output_factory(const std::string& name, log_output_factory* fac)
    { _log_output_factories.emplace(name, fac); }

private:
    logger* create_new_logger(std::string name);
    std::shared_ptr<log_output> get_or_create_output(bool force_create = false);
    std::shared_ptr<prefix_printer> get_or_create_prefix_printer(bool force_create = false);
    std::shared_ptr<locked_stream> get_or_create_locked_stream(bool force_create = false);

    std::map<std::string, std::shared_ptr<logger>, std::less<>> _loggers;

    std::locale _loc;
    std::string _base_dir;
    properties_type _properties;

    std::atomic<LEVEL> _log_level{LEVEL::WARNING};
    std::atomic<SYNC_POLICY> _sync_policy{SYNC_POLICY::SYNC};

    std::shared_ptr<log_output> _log_out;
    std::shared_ptr<locked_stream> _locked_stream;
    std::shared_ptr<prefix_printer> _prefix_printer;
    std::recursive_mutex _config_mx;

    std::map<std::string, std::unique_ptr<prefix_printer_factory>, std::less<>> _prefix_printer_factories;
    std::map<std::string, std::unique_ptr<log_output_factory>, std::less<>> _log_output_factories;
};

#ifdef MOD_SERVLET
/**
 * Thread specific <code>log_registry</code>
 */
extern thread_local std::shared_ptr<log_registry> THREAD_REGISTRY;
#endif

/**
 * Returns global registry.
 * @return Global <code>log_registry</code> instance.
 */
log_registry& registry();
/**
 * Loads configuration from file to global <code>log_registry</code>
 * @param cfg_file_name - name of the properties file.
 * @param base_dir - base directory to resolve relative file paths while configuring.
 */
void load_config(const char* cfg_file_name, const std::string& base_dir = "");

/**
 * Returns logger with a given name from global <code>log_registry</code>
 * @param name Name of the requested logger
 * @return Logger with a given name
 * @see log_registry.log
 */
inline std::shared_ptr<logger> get_logger(const std::string& name) { return registry().log(name); }
/**
 * Returns "root" logger from global <code>log_registry</code>
 * @return "root" logger.
 * @see log_registry.log
 */
inline std::shared_ptr<logger> get_logger() { return registry().log(); }

/**
 * Returns a logger from a given registry with the demangled name of the class.
 * Such loggers can be used as class loggers in following way:
 *
 * ~~~~~{.cpp}
 * class my_class
 * {
 *   static std::shared_ptr<logger> class_lg;
 * };
 * static std::shared_ptr<logger> my_class::class_lg = get_class_logger(typeid(my_class).name());
 * ~~~~~
 * @param registry <code>log_registry</code> to request the logger from.
 * @param class_name <code>typeid</code> name of the class.
 * @return Logger with class name
 */
std::shared_ptr<logger> get_class_logger(log_registry& registry, const char * class_name);

/**
 * Returns class logger from the global <code>log_registry</code>
 * @param class_name <code>typeid</code> name of the class.
 * @return logger with the class name
 * @see registry
 * @see get_class_logger(log_registry& , const char *)
 */
inline std::shared_ptr<logger> get_class_logger(const char * class_name)
{ return get_class_logger(registry(), class_name); }

/* Necessary implementations */

inline level_logger::~level_logger() noexcept { if (_out) _logger._lock_stream->return_buffer(_out); }

template<typename T>
level_logger &level_logger::operator<<(const T &value)
{
    if (!_logger.is_loggable(_log_level)) return *this;
    if (!_out)
    {
        _out = _logger._lock_stream->get_buffer(_logger._loc);
        _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    }
    else if ((*_out)->view().empty()) _logger._formatter->print_prefix(_log_level, _logger._name, *_out);
    (*_out) << value;
    if ((*_out)->view().back() == '\n')
    {
        _logger._lock_stream->return_buffer(_out);
        _out = nullptr;
    }
    return *this;
}

} // end of servlet::logging namespace

} // end of servlet namespace

#endif // SERVLET_LOGGER_H
