cmake_minimum_required(VERSION 3.13)

etn_test(${PROJECT_NAME}-test
    DATA
        assets/*
        root/*
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SOURCES
        main.cpp
        assets.cpp
        protocols.cpp
        mibs.cpp
        test-common.h
        # discovery-config.cpp
        discovery-config-manager.cpp
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/conf/config-discovery.conf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


etn_coverage(${PROJECT_NAME}-test)
