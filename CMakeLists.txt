cmake_minimum_required(VERSION 3.16)
project(PortPeek LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Resource & Manifest files
set(RESOURCE_FILES
    res/resource.rc
    res/app.manifest
)

# Source files
set(SOURCE_FILES
    src/main.cpp
    src/tray.cpp
    src/tray.h
    src/ports.cpp
    src/ports.h
    src/process.cpp
    src/process.h
    src/probe.cpp
    src/probe.h
    src/theme.cpp
    src/theme.h
    src/config.cpp
    src/config.h
    src/resource.h
)

add_executable(PortPeek WIN32
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
)

target_include_directories(PortPeek PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(PortPeek PRIVATE
    ws2_32
    iphlpapi
    psapi
    dwmapi
    uxtheme
    comctl32
    shell32
    user32
    gdi32
    advapi32
)

# Compiler optimization settings for ultra-compact size
if(MSVC)
    target_compile_options(PortPeek PRIVATE
        /O1
        /GL
        /Gy
        /utf-8
        /W4
        /WX-
        /EHsc
    )
    target_link_options(PortPeek PRIVATE
        /LTCG
        /OPT:REF
        /OPT:ICF
        /SUBSYSTEM:WINDOWS
        /MANIFEST:EMBED
        /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/res/app.manifest
    )
endif()
