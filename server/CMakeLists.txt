cmake_minimum_required(VERSION 3.13)

########################################################################################################################

etn_target(static ${PROJECT_NAME}-static
    SOURCES
        src/discovery.cpp
        src/discovery.h
        src/config.h

        src/jobs/protocols.cpp
        src/jobs/protocols.h
        src/jobs/mibs.cpp
        src/jobs/mibs.h
        src/jobs/assets.cpp
        src/jobs/assets.h
        src/jobs/auto-discovery.cpp
        src/jobs/auto-discovery.h
        src/jobs/create-asset.cpp
        src/jobs/create-asset.h
        src/jobs/scan-start.cpp
        src/jobs/scan-start.h
        src/jobs/scan-stop.cpp
        src/jobs/scan-stop.h
        src/jobs/scan-status.cpp
        src/jobs/scan-status.h

        src/jobs/impl/snmp.cpp
        src/jobs/impl/snmp.h
        src/jobs/impl/xml-pdc.cpp
        src/jobs/impl/xml-pdc.h
        src/jobs/impl/neon.cpp
        src/jobs/impl/neon.h
        src/jobs/impl/ping.h
        src/jobs/impl/mibs.cpp
        src/jobs/impl/mibs.h
        src/jobs/impl/uuid.cpp
        src/jobs/impl/uuid.h
        src/jobs/impl/nut/mapper.cpp
        src/jobs/impl/nut/mapper.h
        src/jobs/impl/nut/process.cpp
        src/jobs/impl/nut/process.h
    INCLUDE_DIRS
        src
    USES
        fty-utils
        fty_common_logging
        fty-pack
        ${PROJECT_NAME}-common
        pthread
        netsnmp
        neon
        stdc++fs
        fty_security_wallet
        czmq
        fty_common
        fty_common_socket
        crypto
        uuid
        yaml-cpp
        libfty-asset
        fty-asset-libng
        tntdb
        cxxtools
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

etn_configure_file(
    conf/config-discovery.conf.in

    TARGET      ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}
)
