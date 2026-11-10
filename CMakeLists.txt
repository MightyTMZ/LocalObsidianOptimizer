cmake_minimum_required(VERSION 4.3)
project(obsidian_optimizer)

set(CMAKE_CXX_STANDARD 20)

add_executable(obsidian_optimizer main.cpp
        VaultScanner.cpp
        VaultScanner.h
        MediaScanner.cpp
        MediaScanner.h
)
