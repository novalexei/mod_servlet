/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#include <boost/core/demangle.hpp>

namespace servlet
{

std::string demangle(const char* name)
{
    return boost::core::demangle(name);
}

} // end of servlet namespace
