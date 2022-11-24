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
        src/scan-start.h
        src/scan-start.cpp
        src/scan-stop.h
        src/scan-stop.cpp
        src/scan-status.h
        src/scan-status.cpp
    USES
        tntnet
        cxxtools
        fty-cmake-rest
        fty_common_logging
        fty_common
        fty_common_rest
        ${PROJECT_NAME}-common
    TARGET_DESTINATION
        ${CMAKE_INSTALL_FULL_LIBDIR}
)

########################################################################################################################

# mappings for tntnet -> etc/
etn_configure_file(
    conf/42_discovery.xml.in
    TARGET      ${PROJECT_NAME}-rest
    DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/tntnet/bios.d
)

########################################################################################################################
