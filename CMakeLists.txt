cmake_minimum_required(VERSION 3.9)
project(ProjetoSO)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)

add_executable(ProjetoSO
        config.txt
        main.c
        makefile
        structs.h
        urgency.log)
