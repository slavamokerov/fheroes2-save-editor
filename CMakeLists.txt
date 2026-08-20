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

# Core library (no Qt) — shared by the application and the tests.
add_library(fh2core STATIC
    src/constants.cpp
    src/gettextmo.cpp
    src/savefile.cpp
)

add_executable(fheroes2-save-editor
    src/main.cpp
    src/aggicn.cpp
    src/assets.cpp
    src/gamefont.cpp
    src/buttonfont.cpp
    src/gameui.cpp
    src/gamedata.cpp
    src/dialogs.cpp
    src/mainwindow.cpp
    src/heropanel.cpp
    src/translations.cpp
)

target_link_libraries(fheroes2-save-editor PRIVATE Qt6::Widgets fh2core)
target_link_libraries(fh2core PUBLIC zlibstatic)

if(APPLE)
    set_target_properties(fheroes2-save-editor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "fheroes2 Save Editor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.fheroes2.saveeditor"
    )
endif()

if(WIN32)
    set_target_properties(fheroes2-save-editor PROPERTIES WIN32_EXECUTABLE TRUE)
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
