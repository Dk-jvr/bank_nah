cmake_minimum_required(VERSION 3.12)
project(pg_service_template CXX)

include(third_party/userver/cmake/SetupEnvironment.cmake)
include(GNUInstallDirs)

add_subdirectory(third_party/userver)


# Common sources
add_library(${PROJECT_NAME}_objs OBJECT
    src/POST/registration/registration.hpp
    src/POST/registration/registration.cpp
    src/GET/get_balance/get_balance.hpp
    src/GET/get_balance/get_balance.cpp
    src/POST/changing_balance/changing_balance.hpp
    src/POST/changing_balance/changing_balance.cpp
    src/public_components/enums.hpp
    src/POST/money_order/money_order.hpp
    src/POST/money_order/money_order.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver-postgresql)


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)


# Unit Tests
add_executable(${PROJECT_NAME}_unittest
    src/hello_test.cpp
)
target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver-utest)
add_google_tests(${PROJECT_NAME}_unittest)


# Benchmarks
add_executable(${PROJECT_NAME}_benchmark
    src/hello_benchmark.cpp
)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver-ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)


# Functional Tests
include(UserverTestsuite)

userver_testsuite_add_simple(
    REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/requirements.txt"
)


# Install
if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

set(CONFIG_FALLBACK_PATH
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}/dynamic_config_fallback.json")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.template.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.yaml"
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.docker.template.yaml"
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.docker.yaml"
)

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})
