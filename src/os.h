/*
Copyright (c) 2016 Alexei Novakov
https://github.com/novalexei

Distributed under the Boost Software License, Version 1.0.
http://boost.org/LICENSE_1_0.txt
*/
#ifndef MOD_SERVLET_OS_H
#define MOD_SERVLET_OS_H

#include <ctime>

/* Some OS dependent calls */

namespace servlet
{

/* It seems that calls to localtime_s and localtime_r are much faster than to std::localtime */
std::tm get_tm(std::time_t epoch);
std::tm get_gmtm(std::time_t epoch);

int get_pid();

} // end of servlet namespace

#endif // MOD_SERVLET_OS_H
