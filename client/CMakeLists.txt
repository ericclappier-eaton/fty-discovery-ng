cmake_minimum_required(VERSION 3.13)

etn_target(shared ${PROJECT_NAME}-client
    PUBLIC
        fty/discovery/client.h
    SOURCES
        src/client.cpp
    USES
        ${PROJECT_NAME}-common
)
