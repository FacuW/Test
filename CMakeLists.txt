cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  "monitoreo_sistema"
  VERSION 1.0.0
  DESCRIPTION "Sistema de monitoreo básico y observabilidad"
  LANGUAGES C
)

include(CTest)
enable_testing()

# Configuración de Conan
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
  include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

# Buscar paquetes instalados por Conan
find_package(cjson QUIET)

# Define el estándar de C
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include the `include` directory, where the headers are located
include_directories(include)

# Agregar submódulos como subdirectorios
# Solo usamos Unity como submódulo, cJSON lo obtenemos de Conan
add_subdirectory(lib/unity)

# Add the `src` directory, where the C source files are located
file(GLOB_RECURSE SOURCES "src/*.c")

# Compilador flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=gnu11")

# Crear el ejecutable principal
add_executable(${PROJECT_NAME} ${SOURCES})

# Incluir directorios de headers de las librerías
target_include_directories(${PROJECT_NAME} PUBLIC lib/unity/src)

# Linkear librerías
if(cjson_FOUND)
  # Si cJSON se encontró a través de Conan, usamos esa versión
  message(STATUS "Using cJSON from Conan")
  target_link_libraries(${PROJECT_NAME} cjson::cjson)
else()
  # Si no se encontró cJSON a través de Conan, usamos el submódulo
  message(STATUS "Using cJSON from submodule")
  # Configuración para cJSON - deshabilitar tests y otras opciones
  set(ENABLE_CJSON_TEST OFF CACHE BOOL "Disable cJSON tests" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
  set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "Disable cJSON custom flags" FORCE)
  set(ENABLE_CJSON_UTILS OFF CACHE BOOL "Disable cJSON utils" FORCE)
  set(ENABLE_TARGET_EXPORT OFF CACHE BOOL "Disable cJSON target export" FORCE)
  set(ENABLE_CJSON_UNINSTALL OFF CACHE BOOL "Disable cJSON uninstall" FORCE)
  
  add_subdirectory(lib/cjson)
  target_include_directories(${PROJECT_NAME} PUBLIC lib/cjson)
  target_link_libraries(${PROJECT_NAME} cjson)
endif()

# Configuración para tests
if (BUILD_TESTS)
  add_subdirectory(tests)
endif()

# Flags de debug y coverage
set(CMAKE_C_FLAGS_DEBUG "-g --coverage -fsanitize=address,leak,undefined")
