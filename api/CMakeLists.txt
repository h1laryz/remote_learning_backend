cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(remote_learning_backend_api CXX)

find_package(jwt-cpp CONFIG REQUIRED)

# Adding userver dependency
find_package(userver REQUIRED COMPONENTS core postgresql)
find_package(AWSSDK REQUIRED COMPONENTS core s3)

userver_setup_environment()

# Common sources
file(GLOB_RECURSE OBJ_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.cpp)
add_library(${PROJECT_NAME}_objs OBJECT ${OBJ_SRC})
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::postgresql jwt-cpp::jwt-cpp ${AWSSDK_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Functional Tests
include(UserverTestsuite)

userver_testsuite_add_simple()

# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
  message(STATUS "Set install prefix: $ENV{PREFIX}")
  file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
  set(CMAKE_INSTALL_PREFIX ${PREFIX_PATH})
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/ COMPONENT ${PROJECT_NAME})

add_subdirectory(benchmark)
add_subdirectory(unit_tests)