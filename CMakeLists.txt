cmake_minimum_required(VERSION 3.20)
project(crash_woc_mac VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

# The runtime: its folder in this repository, or a checkout of its own next to it
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ps2-native-runtime/CMakeLists.txt)
    set(PS2_RUNTIME_DEFAULT ${CMAKE_CURRENT_SOURCE_DIR}/ps2-native-runtime)
else()
    set(PS2_RUNTIME_DEFAULT ${CMAKE_CURRENT_SOURCE_DIR}/../ps2-native-runtime)
endif()
set(PS2_RUNTIME_DIR ${PS2_RUNTIME_DEFAULT} CACHE PATH "ps2-native-runtime's folder")
add_subdirectory(${PS2_RUNTIME_DIR} ps2-native-runtime)

# Your own disc image: Crash Bandicoot: The Wrath of Cortex, USA v1.01 (SLUS-20238), a .cue/.bin or an .iso
set(WOC_DISC "" CACHE FILEPATH "Your Wrath of Cortex (USA v1.01) disc image")
if(NOT WOC_DISC)
    message(FATAL_ERROR "Set WOC_DISC to your disc image: cmake -DWOC_DISC=/path/to/game.cue ...")
endif()

ps2_recompile(woc_code CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/config/woc.toml DISC ${WOC_DISC} EXECUTABLE SLUS_202.38 FILES 48)

add_executable(crash-woc src/main.cpp src/nusound.cpp src/typing.cpp src/pctext.cpp src/menus.cpp src/skip.cpp src/levels.cpp
                         src/options.cpp src/saving.cpp src/loading.cpp
                         src/menutest.cpp)
target_link_libraries(crash-woc PRIVATE woc_code ps2runtime)

if(APPLE)
    # A Mac app (build/crash-woc.app; the release names it "Crash Bandicoot The Wrath of Cortex.app"), with the icon
    # tools/make_icon.py makes from the game's own art on your disc (with the python3 on the PATH: it needs Pillow and NumPy)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(ICON ${CMAKE_CURRENT_BINARY_DIR}/icon/AppIcon.icns)
    add_custom_command(
        OUTPUT ${ICON}
        COMMAND /usr/bin/env python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_icon.py ${WOC_DISC} --out ${CMAKE_CURRENT_BINARY_DIR}/icon
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_icon.py
        COMMENT "Making the app's icon from the disc"
        VERBATIM)
    target_sources(crash-woc PRIVATE ${ICON})
    set_source_files_properties(${ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(crash-woc PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "Crash Bandicoot The Wrath of Cortex"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.wrathofcortex.native"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_ICON_FILE AppIcon)

    # The release: the app made standalone (its libraries in Contents/Frameworks, their licences, signed for this Mac) and
    # zipped: build/dist/Crash Bandicoot The Wrath of Cortex.app and its .zip. `cmake --build build --target release`
    add_custom_target(release
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bundle_app.py --app $<TARGET_BUNDLE_DIR:crash-woc>
                --name "Crash Bandicoot The Wrath of Cortex" --zip
        DEPENDS crash-woc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Making the standalone app (build/dist)"
        VERBATIM)
endif()
