cmake_minimum_required(VERSION 3.16)
project(microkinetic-network-editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

get_property(MNE_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT MNE_IS_MULTI_CONFIG AND CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

set(SOURCES
    src/main.cpp
    src/main_window.cpp
    src/network_view.cpp
    src/network_io.cpp
    src/color_wheel_widget.cpp
    src/color_picker_dialog.cpp
    src/log_window.cpp
    src/logging.cpp
    src/vibrational_analysis.cpp
    src/structures/anaglyph_widget.cpp
    src/structures/atom.cpp
    src/structures/atom_settings.cpp
    src/structures/bond.cpp
    src/structures/model.cpp
    src/structures/model_loader.cpp
    src/structures/primitivebuilder.cpp
    src/structures/shader_program.cpp
    src/structures/shader_program_manager.cpp
    src/structures/structure.cpp
    src/structures/structure_loader.cpp
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
    src/vibrational_analysis.h
    src/structures/anaglyph_widget.h
    src/structures/atom.h
    src/structures/atom_settings.h
    src/structures/bond.h
    src/structures/model.h
    src/structures/model_loader.h
    src/structures/primitivebuilder.h
    src/structures/shader_program.h
    src/structures/shader_program_manager.h
    src/structures/shader_program_types.h
    src/structures/structure.h
    src/structures/structure_loader.h
)

find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets OpenGL)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL)
find_package(yaml-cpp REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(mne ${SOURCES} ${HEADERS})

if (MNE_IS_MULTI_CONFIG)
    set(MNE_BUILD_TYPE "Multi-config (${CMAKE_CONFIGURATION_TYPES})")
    set(MNE_CONFIG_CXX_FLAGS
        "Debug: ${CMAKE_CXX_FLAGS_DEBUG}; Release: ${CMAKE_CXX_FLAGS_RELEASE}; RelWithDebInfo: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}; MinSizeRel: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
else()
    set(MNE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    string(TOUPPER "${CMAKE_BUILD_TYPE}" MNE_BUILD_TYPE_UPPER)
    set(MNE_CONFIG_CXX_FLAGS "${CMAKE_CXX_FLAGS_${MNE_BUILD_TYPE_UPPER}}")
endif()

set(MNE_GLOBAL_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
get_target_property(MNE_TARGET_COMPILE_OPTIONS mne COMPILE_OPTIONS)
if (NOT MNE_TARGET_COMPILE_OPTIONS)
    set(MNE_TARGET_COMPILE_OPTIONS "")
else()
    string(JOIN " " MNE_TARGET_COMPILE_OPTIONS ${MNE_TARGET_COMPILE_OPTIONS})
endif()

configure_file(src/build_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.h @ONLY)

target_include_directories(mne PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)

target_link_libraries(mne PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    yaml-cpp::yaml-cpp
    Eigen3::Eigen
    OpenGL::GL
)

set_target_properties(mne PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if (WIN32)
    target_sources(mne PRIVATE mne.rc)
endif()
install(TARGETS mne RUNTIME DESTINATION . BUNDLE DESTINATION .)
install(DIRECTORY examples DESTINATION .)

# =========================
# Doxygen Documentation Linter
# =========================

find_package(Doxygen REQUIRED)

add_custom_target(lint-docs
    # 1. Run doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile.lint

    # 2. Check warnings.log
    COMMAND ${CMAKE_COMMAND}
        -DLOGFILE=${CMAKE_SOURCE_DIR}/warnings.log
        -P ${CMAKE_SOURCE_DIR}/check_doxygen.cmake

    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)