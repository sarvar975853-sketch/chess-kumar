cmake_minimum_required(VERSION 3.14)
project(ChessKumar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures")

find_package(SFML REQUIRED COMPONENTS Graphics Window System Audio Network)

set(SOURCES
    src/main.cpp
    src/ChessBoard.cpp
    src/ChessAI.cpp
    src/AssetManager.cpp
    src/Menu.cpp
    src/Game.cpp
    src/AudioManager.cpp
    src/OpeningBook.cpp
    src/NetworkManager.cpp
)

set(HEADERS
    src/ChessBoard.h
    src/ChessAI.h
    src/AssetManager.h
    src/Menu.h
    src/Game.h
    src/AudioManager.h
    src/Settings.h
    src/OpeningBook.h
    src/NetworkManager.h
)

if(APPLE)
    set(APP_ICON "${CMAKE_SOURCE_DIR}/assets/chess.icns")
    if(EXISTS "${APP_ICON}")
        set_source_files_properties(${APP_ICON} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE
            ${SOURCES} ${HEADERS} ${APP_ICON}
        )
    else()
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE
            ${SOURCES} ${HEADERS}
        )
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Chess Kumar"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_IDENTIFIER "com.chesskumar.app"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Info.plist.in"
        INSTALL_RPATH "@executable_path/../Frameworks"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SFML_INCLUDE_DIR}
    src
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE AND SFML_FRAMEWORK_DIR)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "${SFML_FRAMEWORK_DIR}"
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND rsync -a --delete --exclude='.DS_Store' --exclude='*.mp3' --exclude='*.svg' --exclude='*.cur' --exclude='app_icon.png' --exclude='chess.icns'
        "${CMAKE_SOURCE_DIR}/assets/"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/"
    COMMENT "Copying assets"
)
