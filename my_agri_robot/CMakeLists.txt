cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(my_agri_robot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Application sources
target_sources(app PRIVATE
    src/main.cpp

    # App tasks
    src/app_tasks/motor_task.cpp
    src/app_tasks/cmd_task.cpp

    # Motor control subsystem
    src/subsystems/motor_control/motor_hw.cpp
    src/subsystems/motor_control/motor_drv.cpp
    src/subsystems/motor_control/motor_sm.cpp
    src/subsystems/motor_control/motor_app.cpp

    # Comms subsystem
    src/subsystems/comms/cmd_hw.cpp
    src/subsystems/comms/cmd_receiver.cpp
    src/subsystems/comms/cmd_transfer.cpp
    src/subsystems/comms/cmd_parser.cpp
)

target_include_directories(app PRIVATE
    src/include
    src/app_tasks
    src/subsystems/motor_control
    src/subsystems/comms
)
