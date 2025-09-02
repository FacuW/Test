# Verify cmake version
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

# Create project
project(ShellProject
    VERSION 1.0.0
    DESCRIPTION "A nice shell project!"
    LANGUAGES C
)

# Enable testing
include(CTest)

# Flags for compiling
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Wall -pedantic -Werror -Wextra -Wconversion")

# Include headers
include_directories(include)

# Find dependences
find_package(cJSON REQUIRED)
find_package(unity REQUIRED)

# add_executable --> .exe/binary
# add_library --> .o/.so/-sll STATIC SHARED
add_executable(${PROJECT_NAME} src/main.c)

target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/cjson)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE cjson)
target_link_libraries(${PROJECT_NAME} PRIVATE unity)