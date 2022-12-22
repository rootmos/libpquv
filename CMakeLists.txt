cmake_minimum_required(VERSION 3.0)
project(libpquv C)

add_compile_options(-pedantic -Wall -Wextra)

find_package(PostgreSQL REQUIRED)

add_subdirectory(src)
enable_testing()
add_subdirectory(test)
