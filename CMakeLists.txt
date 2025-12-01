cmake_minimum_required(VERSION 3.16)
project(FFXVUnlocker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MemoryEditor.cpp
    src/PatternScanner.cpp
    src/HttpServer.cpp
)

# Header files
set(HEADERS
    include/MainWindow.h
    include/MemoryEditor.h
    include/PatternScanner.h
    include/HttpServer.h
    include/Patches.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
    resources/app.manifest
    resources/app.rc
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Network
    ws2_32      # Winsock for HTTP server
    psapi       # Process API for memory operations
)

# Option to skip post-build copy (for CI builds that handle this separately)
option(SKIP_POST_BUILD_COPY "Skip post-build DLL copying" OFF)

if(NOT SKIP_POST_BUILD_COPY)
    # Copy qt.conf to build directory (tells Qt to look for plugins in ./plugins)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/resources/qt.conf"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qt.conf"
    )

    # Copy required Qt DLLs to build directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../bin/Qt6Core.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../bin/Qt6Gui.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../bin/Qt6Widgets.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../bin/Qt6Network.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying Qt DLLs"
    )

    # Copy MinGW runtime DLLs (local development path)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/Qt/Tools/mingw1310_64/bin/libgcc_s_seh-1.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/Qt/Tools/mingw1310_64/bin/libstdc++-6.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/Qt/Tools/mingw1310_64/bin/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        COMMENT "Copying MinGW runtime DLLs"
    )

    # Create plugins directory structure and copy Qt plugins
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/styles"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../plugins/platforms/qwindows.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${Qt6_DIR}/../../../plugins/styles/qmodernwindowsstyle.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/styles/"
        COMMENT "Copying Qt plugins to plugins folder"
    )
endif()
