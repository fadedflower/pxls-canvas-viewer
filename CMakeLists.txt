cmake_minimum_required(VERSION 3.11)
project(pxls-canvas-viewer VERSION 1.0)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Dependencies
add_executable(${PROJECT_NAME}
        src/PxlsLogDB.cpp
        src/PxlsCanvas.cpp
        src/PxlsOverlay.cpp
        src/main.cpp
        third_party/tinyfiledialogs/tinyfiledialogs.c
)
#set(raylib_VERBOSE 1)
add_subdirectory(third_party/raylib)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

find_package(SQLite3 REQUIRED)
include_directories(${SQLite3_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SQLite3_LIBRARIES})

find_package(Boost CONFIG)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})

# Disable building tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(third_party/json)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json)

add_subdirectory(third_party/date)
target_link_libraries(${PROJECT_NAME} PRIVATE date)

include_directories(third_party/raygui/src)
include_directories(third_party/tinyfiledialogs)
# copy necessary resources
file(COPY_FILE ${CMAKE_SOURCE_DIR}/third_party/raygui/styles/dark/style_dark.rgs ${CMAKE_BINARY_DIR}/style.rgs)
file(COPY_FILE ${CMAKE_SOURCE_DIR}/resources/palette.json ${CMAKE_BINARY_DIR}/palette.json)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
endif()
