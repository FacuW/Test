# Buscar paquetes necesarios
find_package(cJSON REQUIRED)
find_package(Threads REQUIRED)

# Buscar Unity - manejo de diferentes nombres posibles
find_package(unity QUIET)
if(NOT unity_FOUND)
    find_package(Unity QUIET)
    if(NOT Unity_FOUND)
        find_package(ThrowTheSwitch-Unity QUIET)
        if(NOT ThrowTheSwitch-Unity_FOUND)
            message(FATAL_ERROR "Unity testing framework not found. Make sure unity is listed in conanfile.txt")
        else()
            set(UNITY_TARGET ThrowTheSwitch-Unity::ThrowTheSwitch-Unity)
        endif()
    else()
        set(UNITY_TARGET Unity::Unity)
    endif()
else()
    set(UNITY_TARGET unity::unity)
endif()

# Crear una librería estática con el código fuente para los tests
# Esto evita duplicar la compilación del código fuente
add_library(monitoreo_lib STATIC
    ${CMAKE_SOURCE_DIR}/src/metrics.c
)

target_include_directories(monitoreo_lib PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(monitoreo_lib 
    cjson::cjson
)

# Test básico
add_executable(test_main unit/test_main.c)

target_include_directories(test_main PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_main 
    ${UNITY_TARGET}
    cjson::cjson
)

# Test de métricas (incluye el código fuente)
add_executable(test_metrics unit/test_metrics.c)

target_include_directories(test_metrics PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_metrics 
    monitoreo_lib
    ${UNITY_TARGET}
    cjson::cjson
)

# Test del shell
add_executable(test_shell 
    unit/test_shell.c
    ${CMAKE_SOURCE_DIR}/src/shell.c
    ${CMAKE_SOURCE_DIR}/src/shell_commands.c
    ${CMAKE_SOURCE_DIR}/src/shell_ipc.c
)

target_include_directories(test_shell PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_shell 
    monitoreo_lib
    ${UNITY_TARGET}
    cjson::cjson
    Threads::Threads
)

# Add tests to CTest
add_test(NAME unit_tests COMMAND test_main)
add_test(NAME metrics_tests COMMAND test_metrics)
add_test(NAME shell_tests COMMAND test_shell)
