cmake_minimum_required(VERSION 3.5)
project(lockfree_test)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("-O0")

enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(exchange_buffer_test
    main.cpp
    exchange_buffer_test.cpp
)

target_link_libraries(exchange_buffer_test  ${GTEST_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} )

add_executable(exchange_buffer_stresstest
    main.cpp
    exchange_buffer_stresstest.cpp
)

target_link_libraries(exchange_buffer_stresstest  ${GTEST_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} )