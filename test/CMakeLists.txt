cmake_minimum_required(VERSION 3.13)

etn_target(exe ${PROJECT_NAME}-test
    CONFIGS
        conf/discovery.conf
        conf/logger.conf
    SOURCES
        main.cpp
    USES
        ${PROJECT_NAME}-static
        Catch2::Catch2
)
