#ifndef SERVLET_EXCEPTION_H
#define SERVLET_EXCEPTION_H

/**
 * @file exception.h
 * @brief Exception used in the mod_servlet
 *
 * Initially I planned to have exceptions with stack traces, but it appears
 * that under GCC whcen compiling with optimization (-O* flags) stack is not
 * accessible. Until I find how to overcome the problem I'll use plain exceptions.
 *
 * @todo Implement exception with stack traces which work in lib compiled with optimization.
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <typeinfo>

namespace servlet
{

/**
 * Configuration exception
 */
struct config_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Security exception
 */
struct security_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Input/output exception
 */
struct io_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Exception thrown on attempt to access <code>nullptr</code> object if this is
 * possible to catch this attempt.
 */
struct null_pointer_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Exception thrown on passing invalid argument.
 */
struct invalid_argument_exception : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};
/**
 * Exception thrown on bad cast. Standard <code>std::bad_cast</code> is not really
 * sutisfying because it cannot be created with message, so it is not informative.
 */
struct bad_cast : public std::runtime_error, public std::bad_cast
{
    using std::runtime_error::runtime_error;
};

/**
 * Overload of output streaming operator for exception.
 *
 * The output has the following format: <code>"exception type: ex.what()"</code>
 *
 * If the exception has nested exceptions those will be printed as well.
 * @param out Output stream to write the exception to.
 * @param ex Exception to stream.
 * @return Output stream.
 */
std::ostream &operator<<(std::ostream &out, const std::exception &ex);

} // end of servlet namespace

#endif // SERVLET_EXCEPTION_H
