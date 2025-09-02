# Verify cmake version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(TP1_Project
    VERSION 1.0.0
    DESCRIPTION "A nice TP1 project!"
    LANGUAGES C
)

# Enable testing
include(CTest)

# Flags for compiling
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Wall -pedantic -Werror -Wextra -Wconversion")

# Include headers
include_directories(include)

# Find dependencies
find_package(unity REQUIRED)

# --- cJSON: Try Conan first, fallback to system library ---
find_package(cJSON QUIET)

if (TARGET cjson::cjson)
    message(STATUS "Using cJSON from Conan")
    set(CJSON_TARGET cjson::cjson)
else()
    message(STATUS "Using system cJSON")
    find_library(CJSON_LIBRARY cjson REQUIRED)
    find_path(CJSON_INCLUDE_DIR cjson/cJSON.h REQUIRED)
    add_library(cjson_system INTERFACE)
    target_include_directories(cjson_system INTERFACE ${CJSON_INCLUDE_DIR})
    target_link_libraries(cjson_system INTERFACE ${CJSON_LIBRARY})
    set(CJSON_TARGET cjson_system)
endif()

# Add executable
add_executable(${PROJECT_NAME} src/main.c)

# Include system includes (optional)
target_include_directories(${PROJECT_NAME} PRIVATE /usr/include)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE ${CJSON_TARGET})
target_link_libraries(${PROJECT_NAME} PRIVATE unity::unity)
