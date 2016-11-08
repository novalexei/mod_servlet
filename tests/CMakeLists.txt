
enable_testing()

find_package(GTest REQUIRED)

#find_package(Boost COMPONENTS system unit_test_framework timer chrono REQUIRED)

#include_directories(${TEST_SOURCE_DIR}/include ${TEST_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS})

#add_definitions(-DBOOST_TEST_DYN_LINK)

#set(TESTS uri_parse_test uri_set_test just_test uri_props_test uri_resnorm_test)

#foreach (test ${TESTS})
##    message(WARNING "----------------- Configuring tests: ${CMAKE_BINARY_DIR}")
#    add_executable(${test} ${test}.cpp)
#    add_dependencies(${test} mod_servlet)
#    target_link_libraries(${test} mod_servlet ${Boost_SYSTEM_LIBRARY} ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY}
#            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} -lapr-1 -laprutil-1)
#    set_target_properties(${test} PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=ignore-all")
#    set_target_properties(${test} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${mod_servlet_BINARY_DIR}/tests)
#    add_test(${test} ${mod_servlet_BINARY_DIR}/tests/${test})
#endforeach (test)
