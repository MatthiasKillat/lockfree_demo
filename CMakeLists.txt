cmake_minimum_required(VERSION 3.5)
project(lockfree_demo)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("-O3")

include_directories( include )

# add_executable(sync_test
#   sync_test_main.cpp
# )
# target_link_libraries(sync_test pthread)

# add_executable(demo
#   demo_main.cpp
# )
# target_link_libraries(demo pthread)

add_subdirectory(test)


