cmake_minimum_required(VERSION 3.13)

etn_test(${PROJECT_NAME}-test
    DATA
        assets/*
        root/*
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
        conf/configuration.json
        conf/data.json
    SOURCES
        main.cpp
        assets.cpp
        protocols.cpp
        mibs.cpp
        auto-disco.cpp
        test-common.h
        discovery-config-manager.cpp
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
        fty_security_wallet_server
        mlm
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/conf/config-discovery.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


etn_coverage(${PROJECT_NAME}-test)
