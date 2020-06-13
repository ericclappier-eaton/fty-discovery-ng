cmake_minimum_required(VERSION 3.13)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

project(fty-discovery-ng
    VERSION 1.0.0
    DESCRIPTION "New fty discovery"
)

##############################################################################################################

etn_target(static ${PROJECT_NAME}-static
    SOURCES
        src/daemon.cpp
        src/daemon.h
        src/discovery.cpp
        src/discovery.h
        src/config.h
        src/message.cpp
        src/message.h
        src/message-bus.cpp
        src/message-bus.h

        src/jobs/discover.cpp
        src/jobs/discover.h

        src/jobs/protocols/snmp.cpp
        src/jobs/protocols/snmp.h
        src/jobs/protocols/xml.cpp
        src/jobs/protocols/xml.h
        src/jobs/protocols/socket.cpp
        src/jobs/protocols/socket.h
        src/jobs/protocols/neon.cpp
        src/jobs/protocols/neon.h
    USES
        fty-utils
        fty-common-logging-ng
        fty-pack
        fty-common-messagebus
        pthread
        netsnmp
        neon
    PRIVATE
)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SOURCES
        src/main.cpp
    USES
        ${PROJECT_NAME}-static
)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}-test
    CONFIGS
        test/conf/discovery.conf
        test/conf/logger.conf
    SOURCES
        test/main.cpp
    INCLUDE_DIRS
        src
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
)

##############################################################################################################
