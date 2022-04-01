cmake_minimum_required(VERSION 3.16)
project(cLZJH C)

set(CMAKE_C_STANDARD 11)

add_executable(cLZJH main.c node.h node.c lzjh_compress.h lzjh_compress.c lzjh_decompress.h lzjh_decompress.c general.h general.c)

add_compile_options(-Wall)