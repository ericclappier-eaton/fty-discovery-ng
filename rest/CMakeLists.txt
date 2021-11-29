########################################################################################################################

etn_target(shared ${PROJECT_NAME}-rest
    SOURCES
        src/discovery-rest.h

        src/mibs.h
        src/mibs.cpp
        src/protocols.cpp
        src/protocols.h
        src/asset.cpp
        src/asset.h
        src/config-read.h
        src/config-read.cpp
        src/config-create.h
        src/config-create.cpp
        src/scan.h
        src/scan.cpp
    USES
        ${PROJECT_NAME}-common
        fty-cmake-rest
        tntnet
        cxxtools
        fty_common_logging
        fty_common
        fty_common_rest
    TARGET_DESTINATION /usr/lib/bios
)

########################################################################################################################

# discovery.xml for tntnet
etn_configure_file(
    conf/42_discovery.xml.in

    TARGET      ${PROJECT_NAME}
    DESTINATION /etc/tntnet/bios.d
)

########################################################################################################################
