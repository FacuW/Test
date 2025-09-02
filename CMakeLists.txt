cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(ShellProject
    LANGUAGES C
    VERSION 1.0.0
    DESCRIPTION "A nice shell project!"
)

# -----------------------------
# Configuración de compilación
# -----------------------------
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_FLAGS_DEBUG "-g3 -Wall -pedantic -Werror -Wextra -Wconversion")

# -----------------------------
# Buscar librerías
# -----------------------------
# cJSON instalado en el sistema
find_library(CJSON_LIB cjson REQUIRED)

# -----------------------------
# Ejecutable principal
# -----------------------------
add_executable(${PROJECT_NAME} src/main.c)
target_include_directories(${PROJECT_NAME} PRIVATE include /usr/local/include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CJSON_LIB})

# -----------------------------
# Ejecutable de tests con Unity
# -----------------------------
add_executable(tests tests/test_main.c external/Unity/src/unity.c)
target_include_directories(tests PRIVATE include external/Unity/src /usr/local/include)
target_link_libraries(tests PRIVATE ${CJSON_LIB})

# -----------------------------
# Opcional: habilitar testing de CTest
# -----------------------------
include(CTest)
enable_testing()
add_test(NAME run_tests COMMAND tests)