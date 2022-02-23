# fty-discovery-ng
New Discovery which discovers power devices (ups, epdu, ats) on automatic and manual mode.


Discovery build contains logical 4 parts.

* Rest submodule for tntnet
* Discovery agent
* Common discovery static lib
* Unit tests

## How to build
Discovery based on cmake, so build is standart for any cmake build.
```CMake
    mkdir build
    cd build
    cmake ..
    make
```
or for 21 century :)
```CMake
    mkdir build
    cd build
    cmake -G Ninja ..
    ninja
```

Dependencies used in current version:

* netsnmp
* neon
* tntdb
* cidr
* czmq
* stdc++fs
* crypto
* uuid
* yaml-cpp
* cxxtools
* fty-utils
* fty-pack
* fty-cmake-rest
* fty_common
* fty_common_logging
* fty_common_messagebus
* fty_common_logging
* fty_security_wallet
* fty_common_socket
* fty-asset-libng
* Catch2::Catch2 (in case if you want unit testing)

## How to build and use unit tests
To enable unit tests build just reconfigure your cmake build
```CMake
cd build
cmake -DBUILD_TESTING=ON ..
make
```
To run unit test go to `build/test` subdirectory and run `fty-discovery-ng-tests`

## How to run agent
```
systemctl start fty-discovery-ng
```

## Structure of the project

* common - common static library for agent, rest and for tests
* rest - tntnet submodule
* server - discovery agent
    * conf - agent configuration
    * mibs - snmp mibs database
    * src - sources
* test - unit testing


## How to add device to discover

This is not finished yet. But current implementation suppose to do:

* Add MIB file to MIB database. (server/mibs)
* In server/src/jobs/impl/mibs.cpp
    * add MIB to knownMibs mapping
    * add MIB to mapMibToLegacy to map MIB to nut snmp-ups usage
* Recompile

In future will not necessary to recompile project, just add MIB to MIB database and write your mib into config 
