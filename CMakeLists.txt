cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  "monitoreo_sistema"
  VERSION 1.0.0
  DESCRIPTION "Sistema de monitoreo básico y observabilidad"
  LANGUAGES C
)

include(CTest)
enable_testing()

# Configuración de Conan - Usar variable en lugar de ruta hardcodeada
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake" CACHE STRING "Path to Conan toolchain")
if(EXISTS "${CONAN_TOOLCHAIN_PATH}")
    include("${CONAN_TOOLCHAIN_PATH}")
endif()

# Buscar paquetes instalados por Conan
find_package(cJSON REQUIRED)

# Buscar pthread (requerido por prometheus y shell)
find_package(Threads REQUIRED)

# Define el estándar de C
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Incluir TODOS los archivos fuente (incluyendo prometheus_metrics.c y shell)
file(GLOB_RECURSE SOURCES "src/*.c")

# Compilador flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=gnu11")

# Crear el ejecutable principal
add_executable(${PROJECT_NAME} ${SOURCES})

# Incluir directorios de headers
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    /usr/local/include
)

# Linkear librerías básicas
target_link_libraries(${PROJECT_NAME} cjson::cjson)

# Buscar y linkear prometheus
find_library(PROM_LIB prom)
if(PROM_LIB)
    target_link_libraries(${PROJECT_NAME} ${PROM_LIB})
    message(STATUS "Found prometheus library: ${PROM_LIB}")
else()
    message(FATAL_ERROR "Prometheus client library not found. Make sure you have compiled and installed prometheus-client-c")
endif()

# Linkear pthread (requerido por prometheus y shell)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

# Configuración para tests
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Flags de debug y coverage
set(CMAKE_C_FLAGS_DEBUG "-g --coverage -fsanitize=address,leak,undefined")
