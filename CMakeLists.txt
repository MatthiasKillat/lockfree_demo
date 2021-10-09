cmake_minimum_required(VERSION 3.5)
project(lockfree_read_take)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("-O3")

include_directories( include )

add_executable(broken
  broken_main.cpp
)

add_executable(lockfree
  lockfree_main.cpp
)

add_executable(take_test
  take_test_main.cpp
)
target_link_libraries(take_test pthread)

add_executable(sync_test
  sync_test_main.cpp
)
target_link_libraries(sync_test pthread)

add_executable(demo
  demo_main.cpp
)
target_link_libraries(demo pthread)


