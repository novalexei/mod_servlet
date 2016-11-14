cmake_minimum_required(VERSION 2.8.8)
project(mod_servlet)

#set(CMAKE_CURRENT_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

#set(CMAKE_CXX_COMPILER ~/gcc/6.1.0/bin/g++)

INCLUDE(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-std=c++1z HAVE_CPP17)
if (HAVE_CPP17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -DMOD_SERVLET")
else()
    message(FATAL_ERROR "No C++ 17 support (Compiler does not define -std=c++1z). Use \"-DCMAKE_CXX_COMPILER=path.to.compiler\"")
endif()

set(APR_INCLUDE "/usr/include/apr-1")
set(APACHE_MODULES "${APACHE_ROOT}/modules")

option(mod_servlet_BUILD_TESTS "Build the mod_servlet tests." OFF)

find_package(Boost 1.56.0 REQUIRED)

include_directories( ${CMAKE_SOURCE_DIR}/include )

if (NOT EXISTS ${APACHE_ROOT})
    message(FATAL_ERROR "No Apache include directory found. Use \"-DAPACHE_INCLUDE=path.\"")
endif()
if (NOT EXISTS ${APR_INCLUDE})
    message(FATAL_ERROR "No Apache Portable Runtime (APR) include directory found. Use \"-DAPR_INCLUDE=path.\"")
endif()

include_directories( ${APACHE_ROOT}/include )
include_directories( ${APR_INCLUDE} )
include_directories( ${Boost_INCLUDE_DIRS} )

set(SOURCE_FILES src/mod_servlet.cpp include/servlet/servlet.h include/servlet/request.h
        include/servlet/response.h src/config.cpp src/config.h include/servlet/lib/io.h src/lockfree.h
        include/servlet/lib/logger.h include/servlet/lib/optional.h src/properties.h src/string.h
        src/time.h include/servlet/uri.h include/servlet/lib/exception.h src/exception.cpp src/logger.cpp
        src/properties.cpp src/pattern_map.h src/dispatcher.h src/dispatcher.cpp include/servlet/cookie.h
        src/cookie.cpp src/response.cpp src/request.cpp include/servlet/session.h include/servlet/lib/linked_map.h
        src/session.cpp src/servlet.cpp include/servlet/context.h src/context.h include/servlet/filter.h
        src/filter.cpp src/filter_chain.h src/default_servlet.cpp src/multipart.cpp src/content_type.cpp
        src/time.cpp src/setup.cpp src/request.h src/response.h src/multipart.h src/session.h
        include/servlet/uri.h src/uri.cpp src/uri_parse.cpp include/servlet/ssl.h src/ssl.h src/ssl.cpp
        src/logger_format.h src/level_logger.cpp src/logger_format.cpp src/map_ex.h include/servlet/lib/any_map.h
        include/servlet/lib/lru_map.h include/servlet/lib/io_filter.h
        include/servlet/lib/io_string.h src/web_inf_parse.cpp)

#message(WARNING ${Boost_VERSION})

if (mod_servlet_BUILD_TESTS)
    message(STATUS "Configuring tests")
    enable_testing()
    add_subdirectory(deps/googletest)
    add_subdirectory(tests)
endif()

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})
if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif()
    set(doxyfile_in ${CMAKE_SOURCE_DIR}/doxy.conf)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
endif()

add_library(mod_servlet SHARED ${SOURCE_FILES})
# to avoid "lib" prefix in mod_servlet.so
set_target_properties(mod_servlet PROPERTIES PREFIX "")
target_link_libraries(mod_servlet -lstdc++fs)

install(TARGETS mod_servlet LIBRARY DESTINATION ${APACHE_MODULES})
