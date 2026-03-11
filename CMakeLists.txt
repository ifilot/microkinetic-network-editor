cmake_minimum_required(VERSION 3.16)
project(microkinetic-network-editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/network_view.cpp
    src/network_io.cpp
    src/color_wheel_widget.cpp
    src/color_picker_dialog.cpp
    src/log_window.cpp
    src/logging.cpp
    resources.qrc
)

set(HEADERS
    src/main_window.h
    src/network_view.h
    src/network_io.h
    src/color_wheel_widget.h
    src/color_picker_dialog.h
    src/log_window.h
    src/logging.h
    src/config.h
)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(yaml-cpp REQUIRED)

add_executable(mne ${SOURCES} ${HEADERS})

target_include_directories(mne PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(mne PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    yaml-cpp::yaml-cpp
)

set_target_properties(mne PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if (WIN32)
    target_sources(mne PRIVATE mne.rc)
endif()