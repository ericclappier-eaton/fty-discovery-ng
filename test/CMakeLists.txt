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
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
)
