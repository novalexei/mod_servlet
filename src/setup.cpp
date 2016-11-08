#include <boost/core/demangle.hpp>

namespace servlet
{

std::string demangle(const char* name)
{
    return boost::core::demangle(name);
}

} // end of servlet namespace
