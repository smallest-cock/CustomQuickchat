cmake_minimum_required(VERSION 4.0)

project(CustomQuickchat VERSION 1.9.7)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")  # `/MT`... cant use "MultiThreadedDLL" (/MD) bc vcpkg static libs won't work.. and we want static vcpkg libs
set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Toolset version") # 142 = RL's version

# set output directory to ./plugins
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .dll (executables)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .dll (if seen as a shared lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .lib and .pdb (static or debug info)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_options
    /std:c++20
    /EHsc
    /MP

    /Zi # generate debug info
    /O2 # optimize
    /bigobj # handle the massive rlsdk files
)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(COMPONENTS_DIR "${SRC_DIR}/components")

set(COMPONENTS_SRC
    ${COMPONENTS_DIR}/Instances.cpp
    ${COMPONENTS_DIR}/LobbyInfo.cpp
)
if(USE_SPEECH_TO_TEXT)
    list(APPEND COMPONENTS_SRC
        ${COMPONENTS_DIR}/WebsocketManager.cpp
        ${COMPONENTS_DIR}/SpeechToText.cpp
    )
endif()

set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(BMSDK_DIR "${EXTERNAL_DIR}/BMSDK")
set(RLSDK_DIR "${EXTERNAL_DIR}/RLSDK/RLSDK_w_pch_includes")
set(IMGUI_DIR "${EXTERNAL_DIR}/BakkesmodPluginTemplate/IMGUI")
set(MODUTILS_DIR "${EXTERNAL_DIR}/ModUtils")

set(RLSDK_SRC
    ${RLSDK_DIR}/GameDefines.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/AkAudio_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Core_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Engine_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Extras.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/GFxUI_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/IpDrv_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/OnlineSubsystemEOS_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/ProjectX_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/TAGame_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/WinDrv_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/XAudio2_classes.cpp
)

set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imguivariouscontrols.cpp
    ${IMGUI_DIR}/imgui_additions.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp
    ${IMGUI_DIR}/imgui_rangeslider.cpp
    ${IMGUI_DIR}/imgui_searchablecombo.cpp
    ${IMGUI_DIR}/imgui_stdlib.cpp
    ${IMGUI_DIR}/imgui_timeline.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

set(MODUTILS_SRC
    ${MODUTILS_DIR}/gui/GuiTools.cpp
    ${MODUTILS_DIR}/util/Utils.cpp
    ${MODUTILS_DIR}/wrappers/GFxWrapper.cpp
)

add_link_options(/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)

option(USE_SPEECH_TO_TEXT "Enable speech-to-text support" OFF)

if(USE_SPEECH_TO_TEXT)
    add_compile_definitions(USE_SPEECH_TO_TEXT)
endif()
if (THROW_ON_DOUBLE_HOOK)
    add_compile_definitions(THROW_ON_DOUBLE_HOOK)
endif()

add_compile_options(${compile_options})
add_compile_definitions(UNICODE _UNICODE)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR
    "This project requires a 64-bit build.\n"
    "Make sure your toolchain is configured to use a 64-bit compiler."
  )
endif()

add_library(${PROJECT_NAME}
    SHARED
    ${IMGUI_SRC}
    ${RLSDK_SRC}
    ${MODUTILS_SRC}
    ${COMPONENTS_SRC}
    ${SRC_DIR}/${PROJECT_NAME}.cpp
    ${SRC_DIR}/HelperFuncs.cpp
    ${SRC_DIR}/CvarChangeCallbacks.cpp
    ${SRC_DIR}/SettingsGUI.cpp
    ${SRC_DIR}/Functions.cpp
    ${SRC_DIR}/Events.cpp
    ${SRC_DIR}/Commands.cpp
    ${SRC_DIR}/Structs.cpp
    ${SRC_DIR}/TextEffects.cpp
    ${SRC_DIR}/Events.cpp
    ${SRC_DIR}/Commands.cpp
    ${SRC_DIR}/GuiBase.cpp
    ${SRC_DIR}/pch.cpp
    ${SRC_DIR}/${PROJECT_NAME}.rc
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE ${SRC_DIR}/pch.h
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/external
    PRIVATE ${CMAKE_SOURCE_DIR}/external/BMSDK/include
)
if(USE_SPEECH_TO_TEXT)
    target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_SOURCE_DIR}/external/asio/asio/include
        PRIVATE ${CMAKE_SOURCE_DIR}/external/websocketpp
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external/BMSDK/lib/pluginsdk.lib
)

# use selective export handling instead of exporting all symbols
set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

if(NOT "$ENV{CI}" STREQUAL "true")
    set(INSTALLED_BMSDK_DIR "$ENV{APPDATA}/bakkesmod/bakkesmod/bakkesmodsdk" CACHE PATH "Installed BakkesMod SDK directory")

    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND "${INSTALLED_BMSDK_DIR}/bakkesmod-patch.exe" "$<TARGET_FILE:${PROJECT_NAME}>" 
        COMMENT "Running bakkesmod-patch.exe after building ${PROJECT_NAME}"
    )
endif()