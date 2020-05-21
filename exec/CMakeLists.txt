cmake_minimum_required(VERSION 3.13)

#### Global definition

# Project definition
project(fty-discovery-ng
        VERSION 1.0.0
        DESCRIPTION "New fty discovery"
        LANGUAGES CXX)

####  Executable definition

# Get the source files
file(GLOB_RECURSE COMMONLIB_SOURCE_FILES
  src/*.cpp
)

# Create the Executable with the source
add_executable(${PROJECT_NAME}
  ${COMMONLIB_SOURCE_FILES}
)

# Specify the include folder for build and when use as dependencies
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Specify the liexecutablebrary dependencies
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    pthread
)

# Specify the executable compile options
target_compile_options(${PROJECT_NAME}
  PUBLIC
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-std=c++17>
)

####  Executable installation

# Install the executable
