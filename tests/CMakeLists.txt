
include_directories( ${gtest_SOURCE_DIR}/include)

set(TESTS uri_parse_test uri_set_test just_test uri_props_test uri_resnorm_test)

foreach (test ${TESTS})
#    message(WARNING "----------------- Configuring tests: ${CMAKE_BINARY_DIR}")
    add_executable(${test} ${test}.cpp)
    add_dependencies(${test} mod_servlet gtest_main)
    target_link_libraries(${test} mod_servlet gtest_main)
    # we need to ignore ap_* symbols here.
    set_target_properties(${test} PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=ignore-all")
    set_target_properties(${test} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${mod_servlet_BINARY_DIR}/tests)
    add_test(${test} ${mod_servlet_BINARY_DIR}/tests/${test})
endforeach (test)
