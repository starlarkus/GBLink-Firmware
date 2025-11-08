cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The following resolves the Zephyr location using the ZEPHYR_BASE environment variable.
# The `REQUIRED HINTS $ENV{ZEPHYR_BASE}` is optional in case you're not using `west zephyr-export`
# https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html#installing-cmake-packages-west-zephyr-export
set(BOARD nucleo_f070rb)

set(ZEPHYR_MODULES "/Users/nilskirchhof/Repo/Gen3TradeEmu/modules/hal/stm32" "/Users/nilskirchhof/Repo/Gen3TradeEmu/modules/hal/cmsis")
find_package(Zephyr REQUIRED HINTS "/Users/nilskirchhof/Repo/Gen3TradeEmu/zephyr")


project(
    EmptyApp
    VERSION 0.1
    DESCRIPTION "Empty Zephyr application."
    LANGUAGES C
)

# The target "app" is defined in Zephyr's CMake package and is the default target for Zephyr apps.
target_sources(
    app
    PRIVATE
    src/main.cpp
)

set_compiler_property(PROPERTY diagnostic -fdiagnostics-color=never)


add_subdirectory(src/layers)
add_subdirectory(src/callbacks)
add_subdirectory(src/payloads)
add_subdirectory(src/sections)
add_subdirectory(src/module)