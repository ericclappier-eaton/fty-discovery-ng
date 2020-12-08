########################################################################################################################

etn_target(shared ${PROJECT_NAME}-rest
    SOURCES
        conf/42_discovery.xml.in

        src/mibs.h
        src/mibs.cpp
        src/protocols.cpp
        src/protocols.h
        src/asset.cpp
        src/asset.h
    USES
        ${PROJECT_NAME}-common
        fty-cmake-rest
        tntnet
        fty_common_logging
        fty_common_rest
    TARGET_DESTINATION /usr/lib/bios
)

########################################################################################################################

# discovery.xml for tntnet
etn_configure_file(
    conf/42_discovery.xml.in

    TARGET      ${PROJECT_NAME}-rest
    DESTINATION /etc/tntnet/bios.d
)

########################################################################################################################
