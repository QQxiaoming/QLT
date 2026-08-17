cmake_minimum_required(VERSION 3.16)

project(QLT VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_library(si12t STATIC
    3rd/Si12T/Si12T.cpp
)
target_include_directories(si12t PUBLIC 3rd/Si12T)

add_library(py32ioexpander STATIC
    3rd/PY32IOExpander/PY32IOExpander.cpp
)
target_include_directories(py32ioexpander PUBLIC 3rd/PY32IOExpander)

add_executable(QLT
    main.cpp
    RobotFaceWidget.cpp
    RobotFaceWidget.h
    3rd/SCServo/src/HLSCL.cpp
    3rd/SCServo/src/SCS.cpp
    3rd/SCServo/src/SCS0009.cpp
    3rd/SCServo/src/SCSCL.cpp
    3rd/SCServo/src/SCSerial.cpp
    3rd/SCServo/src/SMS_STS.cpp
)

add_executable(scservo_test
    3rd/SCServo/example/Ping.cpp
    3rd/SCServo/src/HLSCL.cpp
    3rd/SCServo/src/SCS.cpp
    3rd/SCServo/src/SCS0009.cpp
    3rd/SCServo/src/SCSCL.cpp
    3rd/SCServo/src/SCSerial.cpp
    3rd/SCServo/src/SMS_STS.cpp
)

add_executable(si12t_test
    3rd/Si12T/example/si12t_test.cpp
)

add_executable(py32ioexpander_test
    3rd/PY32IOExpander/example/py32ioexpander_test.cpp
)

include_directories(3rd/SCServo/include/scservo)
include_directories(3rd/Si12T)

target_link_libraries(QLT PRIVATE Qt5::Widgets si12t)
target_link_libraries(si12t_test PRIVATE si12t)
target_link_libraries(py32ioexpander_test PRIVATE py32ioexpander)

if (UNIX AND NOT APPLE)
    target_link_libraries(QLT PRIVATE m)
    if (TARGET servo_bitbang)
        target_link_libraries(servo_bitbang PRIVATE m pthread rt)
    endif()
endif()
