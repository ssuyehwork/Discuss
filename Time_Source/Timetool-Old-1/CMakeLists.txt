cmake_minimum_required(VERSION 3.16)
project(Time_tool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "G:/Qt/6.10.2/msvc2022_64_static")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR})

if(MSVC)
    add_compile_options(
        $<$<CONFIG:Release>:/MT>
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/GL>
    )
    add_link_options(
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Svg)

# 包含 main.cpp、Windows 图标资源 (.rc) 以及 Qt 内嵌资源 (.qrc)
add_executable(Time_tool WIN32 
    src/main.cpp
    resources/app.rc
    resources/resources.qrc
)

target_link_libraries(Time_tool PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(Time_tool PRIVATE
        User32 Gdi32 Shell32 Ole32
        Comdlg32 Winspool Winmm Imm32
        Dwmapi Uxtheme Ws2_32
        Setupapi Netapi32 Userenv
        Version Crypt32
    )
endif()
