add_library(pquv SHARED pquv.c)
target_link_libraries(pquv pq)
target_include_directories(pquv PUBLIC include)
