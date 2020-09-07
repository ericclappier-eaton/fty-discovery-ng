cmake_minimum_required(VERSION 3.13)

########################################################################################################################

etn_target(static ${PROJECT_NAME}-static
    SOURCES
        src/discovery.cpp
        src/discovery.h
        src/config.h

        src/jobs/common.h
        src/jobs/common.cpp
        src/jobs/protocols.cpp
        src/jobs/protocols.h
        src/jobs/mibs.cpp
        src/jobs/mibs.h
        src/jobs/assets.cpp
        src/jobs/assets.h

        src/jobs/protocols/snmp.cpp
        src/jobs/protocols/snmp.h
        src/jobs/protocols/xml-pdc.cpp
        src/jobs/protocols/xml-pdc.h
        src/jobs/protocols/neon.cpp
        src/jobs/protocols/neon.h
    USES
        fty-utils
        fty_common_logging
        fty-pack
        ${PROJECT_NAME}-common
        pthread
        netsnmp
        neon
    PRIVATE
)

########################################################################################################################

etn_target(exe ${PROJECT_NAME}
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    DATA
        mibs/*
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SYSTEMD
        ${PROJECT_NAME}.service
    SOURCES
        src/main.cpp
    USES
        ${PROJECT_NAME}-static
)

########################################################################################################################
