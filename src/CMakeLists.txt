add_library(pquv SHARED pquv.c util.c)
target_link_libraries(pquv PUBLIC PostgreSQL::PostgreSQL)
target_include_directories(pquv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
install(TARGETS pquv EXPORT libpquv-targets)
install(EXPORT libpquv-targets NAMESPACE libpquv:: DESTINATION share/libpquv)
install(FILES include/pquv.h DESTINATION include)
