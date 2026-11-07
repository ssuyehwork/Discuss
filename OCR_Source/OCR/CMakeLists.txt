cmake_minimum_required(VERSION 3.16)

project(OCR VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 遵循“三权分立”与“T 统一编译路径”工程闭环：
# 目标二进制统一输出至 OCR_Build/OCR 目录
set(TARGET_APP_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../OCR_Build/OCR")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${TARGET_APP_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TARGET_APP_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TARGET_APP_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TARGET_APP_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${TARGET_APP_OUTPUT_DIR}")

# 寻找 Qt6 依赖组件
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/utf-8)
    # WinRT OCR C++/WinRT 支持选项
    add_compile_options(/permissive- /Zc:twoPhase-)
endif()

set(SOURCES
    src/main.cpp
    # core
    src/core/ClipboardMonitor.cpp
    src/core/ClipboardMonitor.h
    src/core/DatabaseManager.h
    src/core/ItemType.cpp
    src/core/ItemType.h
    src/core/OCRManager.cpp
    src/core/OCRManager.h
    # ui
    src/ui/ColorPickerWindow.cpp
    src/ui/ColorPickerWindow.h
    src/ui/FlowLayout.cpp
    src/ui/FlowLayout.h
    src/ui/FramelessDialog.cpp
    src/ui/FramelessDialog.h
    src/ui/FramelessDialogBase.h
    src/ui/IconHelper.h
    src/ui/OCRResultWindow.cpp
    src/ui/OCRResultWindow.h
    src/ui/OCRWindow.cpp
    src/ui/OCRWindow.h
    src/ui/PixelRulerOverlay.cpp
    src/ui/PixelRulerOverlay.h
    src/ui/ScreenshotTool.cpp
    src/ui/ScreenshotTool.h
    src/ui/StringUtils.h
    src/ui/SvgIcons.h
    src/ui/ToolTipOverlay.cpp
    src/ui/ToolTipOverlay.h
    src/ui/UiHelper.h
    # resources
    resources/resources.qrc
    resources/app.manifest
)

if(WIN32)
    list(APPEND SOURCES resources/app_icon.rc)
endif()

add_executable(OCR WIN32 ${SOURCES})

# 显式指定 Target 级别的静态运行时
if(MSVC)
    set_property(TARGET OCR PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(OCR PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TARGET_APP_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TARGET_APP_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TARGET_APP_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${TARGET_APP_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${TARGET_APP_OUTPUT_DIR}"
)

target_link_libraries(OCR PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(OCR PRIVATE
        user32 gdi32 shell32 ole32
        dwmapi uxtheme windowsapp
    )
    set_target_properties(OCR PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
