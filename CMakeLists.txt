cmake_minimum_required(VERSION 3.13)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

project(fty-discovery-ng
    VERSION 1.0.0
    DESCRIPTION "New fty discovery"
)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SOURCES
        src/main.cpp
        src/daemon.cpp
        src/daemon.h
        src/discovery.cpp
        src/discovery.h
        src/config.h
    USES
        fty-utils
        fty-common-logging
        fty-pack
)
