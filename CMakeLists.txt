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
)

target_link_libraries(QLT PRIVATE Qt5::Widgets)

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBGPIOD QUIET libgpiod)
endif()

if (LIBGPIOD_FOUND)
    add_executable(servo_bitbang
        servo_bitbang_main.cpp
        BitBangUart.cpp
        BitBangUart.h
        Scs0009Servo.cpp
        Scs0009Servo.h
    )

    target_include_directories(servo_bitbang PRIVATE ${LIBGPIOD_INCLUDE_DIRS})
    target_compile_options(servo_bitbang PRIVATE ${LIBGPIOD_CFLAGS_OTHER})
    target_link_directories(servo_bitbang PRIVATE ${LIBGPIOD_LIBRARY_DIRS})
    target_link_libraries(servo_bitbang PRIVATE ${LIBGPIOD_LIBRARIES})
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(QLT PRIVATE m)
    if (TARGET servo_bitbang)
        target_link_libraries(servo_bitbang PRIVATE m pthread rt)
    endif()
endif()
