cmake_minimum_required(VERSION 3.0.0)
project(EXT3 VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)
pkg_search_module(GTEST REQUIRED gtest)

if( OPENSSL_FOUND )
    include_directories(${OPENSSL_INCLUDE_DIRS})
    link_directories(${OPENSSL_LIBRARY_DIRS})
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
endif()

if( GTEST_FOUND )
    include_directories(${GTEST_INCLUDE_DIRS})
    link_directories(${GTEST_LIBRARY_DIRS})
    message(STATUS "Using GTest ${GTEST_VERSION}")
endif()

add_executable(main main.cpp fs.cpp sha256.cpp)
target_link_libraries(main gtest crypto pthread)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

