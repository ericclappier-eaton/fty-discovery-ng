cmake_minimum_required(VERSION 3.13)

##############################################################################################################

etn_target(static ${PROJECT_NAME}-static
    SOURCES
        src/discovery.cpp
        src/discovery.h
        src/config.h

        src/jobs/common.h
        src/jobs/common.cpp
        src/jobs/discover.cpp
        src/jobs/discover.h
        src/jobs/configure.cpp
        src/jobs/configure.h
        src/jobs/chaineddevices.cpp
        src/jobs/chaineddevices.h

        src/jobs/protocols/snmp.cpp
        src/jobs/protocols/snmp.h
        src/jobs/protocols/xml-pdc.cpp
        src/jobs/protocols/xml-pdc.h
        src/jobs/protocols/neon.cpp
        src/jobs/protocols/neon.h
    USES
        fty-utils
        fty-common-logging
        fty-pack
        ${PROJECT_NAME}-common
        pthread
        netsnmp
        neon
    PRIVATE
)

########################################################################################################################

etn_target(exe ${PROJECT_NAME}
    DATA
        mibs/*
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SOURCES
        src/main.cpp
    USES
        ${PROJECT_NAME}-static
)

########################################################################################################################

