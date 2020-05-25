cmake_minimum_required(VERSION 3.13)

##############################################################################################################

project(fty-discovery-ng
        VERSION 1.0.0
        DESCRIPTION "New fty discovery"
        )

##############################################################################################################

include(fty-project/cmake/fty-project.cmake)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    SOURCES
        src/daemon.cpp
        src/include.hpp
    USES
        #cxxtools
        #catch2
        tntdb
)
