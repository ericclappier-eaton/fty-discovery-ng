cmake_minimum_required(VERSION 3.13)

etn_target(static ${PROJECT_NAME}-common
    SOURCES
        daemon.h
        daemon.cpp
        message-bus.cpp
        message-bus.h
        message.h
        message.cpp
        commands.h
        discovery-task.h
    USES
        fty-utils
        fty-pack
        fty_common_logging
        fty_common_messagebus
    PRIVATE
)
