cmake_minimum_required(VERSION 3.31)
project(iot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS_RELEASE "/Zi /O2")
set(CMAKE_CXX_FLAGS_RELEASE "/Zi /O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
set(OPCDACLIENT_STATIC ON CACHE BOOL "Build static library version")
find_package(Boost REQUIRED COMPONENTS json)
find_package(libmodbus CONFIG REQUIRED)
add_subdirectory(third_party/OPCClientToolKit)
add_executable(iot
        main.cpp
        src/JsonConfig.cpp
        src/JsonConfig.h
        src/Logger.cpp
        src/Logger.h
        src/ThreadPool.cpp
        src/ThreadPool.h
        src/TimerScheduler.cpp
        src/TimerScheduler.h
        src/DeviceManager.cpp
        src/DeviceManager.h
        src/Device.cpp
        src/Device.h
        src/ModbusDevice.cpp
        src/ModbusDevice.h
        src/Group.cpp
        src/Group.h
        src/Variable.cpp
        src/Variable.h
        src/ModbusVariable.cpp
        src/ModbusVariable.h
        src/DataBuffer.cpp
        src/DataBuffer.h
        src/ModbusGroup.cpp
        src/ModbusGroup.h
        src/OpcdaDevice.cpp
        src/OpcdaDevice.h
        src/OpcdaVariable.cpp
        src/OpcdaVariable.h
        src/OpcdaGroup.cpp
        src/OpcdaGroup.h
        src/OpcuaDevice.cpp
        src/OpcuaDevice.h
)
target_compile_options(iot PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>" "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
target_include_directories(iot
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party/OPCClientToolKit)
target_link_libraries(iot
        PRIVATE
        OPCClientToolKit
        Boost::json
        modbus
)
