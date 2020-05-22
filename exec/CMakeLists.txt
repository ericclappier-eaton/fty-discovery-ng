cmake_minimum_required(VERSION 3.13)

##############################################################################################################

project(fty-discovery-ng
        VERSION 1.0.0
        DESCRIPTION "New fty discovery"
        LANGUAGES CXX)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    SOURCES
        src/daemon.cpp
        include/include.hpp
    USES
        #cxxtools
        #catch2
        tntdb
)

