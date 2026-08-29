cmake_minimum_required(VERSION 3.21)
project(fheroes2-save-editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(QT_NO_KEYWORDS)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

include(FetchContent)
# Don't build zlib's example binaries (they would register in CTest).
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    zlib
    URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(zlib)

# === Shared core (consumed by other projects too, e.g. fheroes2-screenshot) ===
#
# These file lists are the single source of truth for the shared code. The
# fheroes2-screenshot project syncs them into its vendor/ folder using its
# scripts/sync-core.sh. If you move/rename/add core files, update the lists
# here and re-sync there (see AGENTS.md).

# fh2core — save parsing (no Qt).
set(FH2CORE_SOURCES
    src/constants.cpp
    src/gettextmo.cpp
    src/savefile.cpp
    src/worldparse.cpp
)
set(FH2CORE_HEADERS
    src/constants.h
    src/gettextmo.h
    src/savefile.h
    src/worldparse.h
    src/codepages.h
)

# fh2resources — game resource loading/rendering helpers (Qt6::Gui).
set(FH2RESOURCES_SOURCES
    src/aggicn.cpp
    src/assets.cpp
    src/gamefont.cpp
)
set(FH2RESOURCES_HEADERS
    src/aggicn.h
    src/assets.h
    src/gamefont.h
    src/textutil.h
)

# Core library (no Qt) — shared by the application, the tests and external consumers.
add_library(fh2core STATIC ${FH2CORE_SOURCES})
target_include_directories(fh2core PUBLIC src)
target_link_libraries(fh2core PUBLIC zlibstatic)

# Resource library (Qt6::Gui) — shared by the application and external consumers.
add_library(fh2resources STATIC ${FH2RESOURCES_SOURCES})
target_include_directories(fh2resources PUBLIC src)
target_link_libraries(fh2resources PUBLIC Qt6::Gui)

add_executable(fheroes2-save-editor
    src/main.cpp
    src/buttonfont.cpp
    src/gameui.cpp
    src/gamedata.cpp
    src/dialogs.cpp
    src/mainwindow.cpp
    src/heropanel.cpp
    src/translations.cpp
    resources.qrc
)

target_link_libraries(fheroes2-save-editor PRIVATE Qt6::Widgets fh2core fh2resources)

# === Optional poster integration (fheroes2-screenshot's fh2poster library) ===
# The "EXPORT POSTER..." toolbar button. Requires the sibling project
# ~/Projects/fheroes2-screenshot; its fh2poster library reuses this project's
# fh2core/fh2resources targets (the shared core, see AGENTS.md).
#
# OFF by default: the poster export is a development feature. Release builds
# must stay off (release.yml passes the flag explicitly).
set(FH2_WITH_POSTER "off" CACHE STRING "Build with the poster export: on | off")
set_property(CACHE FH2_WITH_POSTER PROPERTY STRINGS on off)

set(FH2_POSTER_DIR "${CMAKE_CURRENT_LIST_DIR}/../fheroes2-screenshot/libs/fh2poster"
    CACHE PATH "Path to the fheroes2-screenshot fh2poster library")

if(FH2_WITH_POSTER STREQUAL "on")
    if(NOT EXISTS "${FH2_POSTER_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "FH2_WITH_POSTER=on but ${FH2_POSTER_DIR} is missing; set -DFH2_POSTER_DIR=... or -DFH2_WITH_POSTER=off")
    endif()
    add_subdirectory(${FH2_POSTER_DIR} fh2poster)
    target_link_libraries(fheroes2-save-editor PRIVATE fh2poster)
    target_compile_definitions(fheroes2-save-editor PRIVATE FH2_WITH_POSTER=1)
    message(STATUS "poster export: enabled (fh2poster from ${FH2_POSTER_DIR})")
else()
    message(STATUS "poster export: disabled")
endif()

if(APPLE)
    set_target_properties(fheroes2-save-editor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "fheroes2 Save Editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.fheroes2.saveeditor"
    )
    target_sources(fheroes2-save-editor PRIVATE packaging/icon.icns)
    set_source_files_properties(packaging/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(fheroes2-save-editor PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")
endif()

if(WIN32)
    set_target_properties(fheroes2-save-editor PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(fheroes2-save-editor PRIVATE packaging/win/icon.rc)
endif()

qt_finalize_executable(fheroes2-save-editor)

# Editor translations (.po files) — shipped inside the app bundle (macOS) or
# next to the binary (Windows); on Linux dev builds the source tree is used.
if(APPLE)
    add_custom_command(TARGET fheroes2-save-editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:fheroes2-save-editor>/Resources/translations
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/files/lang $<TARGET_BUNDLE_CONTENT_DIR:fheroes2-save-editor>/Resources/translations
        COMMENT "Copying editor translations into the app bundle")
elseif(WIN32)
    add_custom_command(TARGET fheroes2-save-editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/files/lang $<TARGET_FILE_DIR:fheroes2-save-editor>/files/lang
        COMMENT "Copying editor translations next to the executable")
endif()

# Core tests on real saves (run: FH2_SAVE_DIR=<saves folder> ctest --test-dir build).
# The tests/ folder is developer-local (kept out of the repository) — it only
# gets wired into the build when it exists.
include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_SOURCE_DIR}/tests/core_test.cpp")
    add_executable(core_test tests/core_test.cpp)
    target_link_libraries(core_test PRIVATE fh2core)
    target_include_directories(core_test PRIVATE src)
    add_test(NAME core_test COMMAND core_test)
endif()
