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
        # discovery-config.cpp
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
        fty_security_wallet_server
        mlm
)

etn_coverage(${PROJECT_NAME}-test)
