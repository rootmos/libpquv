add_executable(driver driver.c common.c sanity.c)
target_link_libraries(driver pquv)
add_test(NAME tests COMMAND driver)
set_tests_properties (tests PROPERTIES DEPENDS driver)
