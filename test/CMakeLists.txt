add_executable(driver test.c)
target_link_libraries(driver pguv)
add_test(NAME tests COMMAND driver)
set_tests_properties (tests PROPERTIES DEPENDS driver)
