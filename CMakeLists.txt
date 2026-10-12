cmake_minimum_required(VERSION 3.20)

project(cpp_oj LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(unofficial-libmariadb CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

add_library(oj_common STATIC
    src/common/config.cc
    src/common/database.cc
    src/common/password.cc
)

target_include_directories(oj_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(oj_common PUBLIC
    yaml-cpp::yaml-cpp
    unofficial::libmariadb
    unofficial-sodium::sodium
)

add_executable(web_server
    src/judge/sample_runner.cc
    src/web_server/admin_service.cc
    src/web_server/auth_middleware.cc
    src/web_server/auth_service.cc
    src/web_server/problem_service.cc
    src/web_server/session.cc
    src/web_server/main.cpp
)

target_include_directories(web_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/web_server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/judge
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(web_server PRIVATE
    oj_common
    httplib::httplib
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

enable_testing()

add_executable(oj_tests
    tests/unit/config_test.cc
    tests/unit/logging_test.cc
    tests/unit/password_test.cc
    tests/unit/problem_service_test.cc
    tests/unit/sample_runner_test.cc
    tests/unit/session_test.cc
    src/judge/sample_runner.cc
    src/web_server/session.cc
)

target_include_directories(oj_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/web_server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/judge
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(oj_tests PRIVATE
    OJ_TEST_COMPILER="${CMAKE_CXX_COMPILER}"
)

target_link_libraries(oj_tests PRIVATE
    oj_common
    httplib::httplib
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME oj_tests COMMAND oj_tests)

add_executable(database_integration_tests
    tests/integration/database_test.cc
)

target_include_directories(database_integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(database_integration_tests PRIVATE
    oj_common
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME database_integration_tests COMMAND database_integration_tests)
