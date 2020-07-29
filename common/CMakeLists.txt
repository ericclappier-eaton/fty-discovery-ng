cmake_minimum_required(VERSION 3.13)

etn_target(static ${PROJECT_NAME}-common
    SOURCES
        daemon.h
        daemon.cpp
        message-bus.cpp
        message-bus.h
        message.h
        message.cpp
    USES
        fty-utils
        fty-common-logging-ng
        fty-pack
        fty-common-messagebus
    PRIVATE
)
