cmake_minimum_required(VERSION 3.13)

etn_test_target(${PROJECT_NAME}-static
    DATA
        assets/*
        root/*
    CONFIGS
        conf/discovery.conf
        conf/configuration.json
        conf/data.json
        conf/config-discovery.conf
    SOURCES
        main.cpp
        assets.cpp
        protocols.cpp
        mibs.cpp
        address.cpp
        auto-disco.cpp
        test-common.cpp
        test-common.h
        discovery-config-manager.cpp
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
        mlm
)
