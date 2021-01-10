cmake_minimum_required(VERSION 3.16)
project(assignment_2 C)

set(CMAKE_C_STANDARD 11)

include_directories(libraw/include)
include_directories(libraw/src)
include_directories(src)

add_executable(assignment_2
        libraw/include/checksums.h
        libraw/include/hexdump.h
        libraw/include/raw.h
        libraw/src/checksum.c
        libraw/src/crc.c
        libraw/src/hexdump.c
        libraw/src/raw.c
        libraw/src/timespec.h
        src/arguments.c
        src/arguments.h
        src/assignment2.c)
