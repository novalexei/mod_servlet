#include "time.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define SRVLT_WIN_TIME
#include <windows.h>
#elif _POSIX_C_SOURCE >= 1 || defined(_XOPEN_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_POSIX_SOURCE) || defined (__linux__)
#define SRVLT_POSIX_TIME
#include <unistd.h>
#endif

namespace servlet
{

std::tm get_tm(std::time_t epoch)
{
#ifdef SRVLT_WIN_TIME
    std::tm ptv;
    localtime_s(&ptv, &epoch);
#elif defined(SRVLT_POSIX_TIME)
    std::tm ptv;
    localtime_r(&epoch, &ptv);
#else
    /* Possible data race */
    static std::mutex time_mutex{};
    std::lock_guard<std::mutex> lg{time_mutex};
    std::tm ptv = *std::localtime(&epoch);
#endif
    return ptv;
}

std::tm get_gmtm(std::time_t epoch)
{
#ifdef SRVLT_WIN_TIME
    std::tm ptv;
    gmtime_s(&ptv, &epoch);
#elif defined(SRVLT_POSIX_TIME)
    std::tm ptv;
    gmtime_r(&epoch, &ptv);
#else
    /* Possible data race */
    static std::mutex time_mutex{};
    std::lock_guard<std::mutex> lg{time_mutex};
    std::tm ptv = *std::gmtime(&epoch);
#endif
    return ptv;
}

} // end of servlet namespace
