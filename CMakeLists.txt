cmake_minimum_required(VERSION 3.10)
project(linux_shell)

set(CMAKE_CXX_STANDARD 20)

add_executable(linux_shell shell.c
        shell.h)
