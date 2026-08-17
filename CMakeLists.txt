cmake_minimum_required(VERSION 3.16)

project(QLT VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

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

include_directories(3rd/SCServo/include/scservo)

target_link_libraries(QLT PRIVATE Qt5::Widgets)

if (UNIX AND NOT APPLE)
    target_link_libraries(QLT PRIVATE m)
    if (TARGET servo_bitbang)
        target_link_libraries(servo_bitbang PRIVATE m pthread rt)
    endif()
endif()
