cmake_minimum_required(VERSION 3.13)

########################################################################################################################

project(fty-discovery-ng
    VERSION 1.0.0
    DESCRIPTION "New fty discovery"
)

########################################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
########################################################################################################################

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(rest)

########################################################################################################################

## .conf file -> lib/tmpfiles.d/
configure_file("${PROJECT_SOURCE_DIR}/server/conf/${PROJECT_NAME}.conf.in" "${PROJECT_BINARY_DIR}/server/conf/${PROJECT_NAME}.conf" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/server/conf/${PROJECT_NAME}.conf" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/tmpfiles.d/")
