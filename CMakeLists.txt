cmake_minimum_required(VERSION 3.20)
project(crash_woc_mac C CXX)

set(CMAKE_CXX_STANDARD 20)

# The shared runtime, next to this repository by default
set(PS2_RUNTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ps2-native-runtime CACHE PATH "ps2-native-runtime's folder")
add_subdirectory(${PS2_RUNTIME_DIR} ps2-native-runtime)

# Your own disc image: Crash Bandicoot: The Wrath of Cortex, USA v1.01 (SLUS-20238), a .cue/.bin or an .iso
set(WOC_DISC "" CACHE FILEPATH "Your Wrath of Cortex (USA v1.01) disc image")
if(NOT WOC_DISC)
    message(FATAL_ERROR "Set WOC_DISC to your disc image: cmake -DWOC_DISC=/path/to/game.cue ...")
endif()

ps2_recompile(woc_code CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/config/woc.toml DISC ${WOC_DISC} EXECUTABLE SLUS_202.38 FILES 48)

add_executable(crash-woc src/main.cpp src/nusound.cpp src/typing.cpp src/pctext.cpp src/menus.cpp src/skip.cpp src/levels.cpp
                         src/options.cpp src/menutest.cpp)
target_link_libraries(crash-woc PRIVATE woc_code ps2runtime)
