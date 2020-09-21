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
        src/jobs/protocols/nut.cpp
        src/jobs/protocols/nut.h
    USES
        fty-utils
        fty_common_logging
        fty-pack
        ${PROJECT_NAME}-common
        pthread
        netsnmp
        neon
        nutclient
        stdc++fs
    PRIVATE
)

########################################################################################################################

etn_target(exe ${PROJECT_NAME}
    DATA
        mibs/*
    SOURCES
        ${PROJECT_NAME}.service.in
        conf/discovery.conf.in
        conf/logger.conf.in

        src/main.cpp
    USES
        ${PROJECT_NAME}-static
)

########################################################################################################################

etn_configure_file(
    ${PROJECT_NAME}.service.in

    TARGET      ${PROJECT_NAME}
    DESTINATION /usr/lib/systemd/system/
)

########################################################################################################################

etn_configure_file(
    conf/discovery.conf.in

    TARGET      ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}
)

########################################################################################################################

etn_configure_file(
    conf/logger.conf.in

    TARGET      ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}
)

########################################################################################################################
