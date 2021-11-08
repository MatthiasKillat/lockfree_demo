cmake_minimum_required(VERSION 3.5)
project(lockfree_demo)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("-O3")

include_directories( include )

add_executable(demo
  demo_main.cpp
)

add_subdirectory(test)


