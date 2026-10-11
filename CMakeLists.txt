cmake_minimum_required(VERSION 3.20)

project(cpp_oj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(httplib CONFIG REQUIRED)

add_executable(web_server
    src/web_server/main.cpp
)

target_include_directories(web_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/web_server
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(web_server PRIVATE
    httplib::httplib
)
