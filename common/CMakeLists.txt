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
        commands.cpp
        discovery-task.h
        discovery-config.h
        discovery-config.cpp
        discovery-config-manager.h
        discovery-config-manager.cpp
    USES
        fty-utils
        fty-pack
        fty_common_logging
        fty_common_messagebus
    PRIVATE
)
