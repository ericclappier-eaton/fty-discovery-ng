cmake_minimum_required(VERSION 3.13)
project(fty-discovery-ng-topproject CXX)

# Common cmake settings
include(fty-project/cmake/fty-project.cmake)

add_subdirectory(${PROJECT_SOURCE_DIR}/exec)