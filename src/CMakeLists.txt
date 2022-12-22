add_library(pquv SHARED pquv.c util.c)
target_link_libraries(pquv PUBLIC PostgreSQL::PostgreSQL)
target_include_directories(pquv PUBLIC include)
