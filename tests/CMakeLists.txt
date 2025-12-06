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

# Buscar prometheus
find_library(PROM_LIB prom)

# Crear una librería estática con el código fuente completo para los tests
# NOTA: Solo debe haber UNA declaración de monitoreo_lib
add_library(monitoreo_lib STATIC
    ${CMAKE_SOURCE_DIR}/src/metrics.c
    ${CMAKE_SOURCE_DIR}/src/prometheus_metrics.c
    ${CMAKE_SOURCE_DIR}/src/memory_manager.c      
    ${CMAKE_SOURCE_DIR}/src/storage.c             
    ${CMAKE_SOURCE_DIR}/src/sandbox.c             
    ${CMAKE_SOURCE_DIR}/src/shell_tp3_commands.c  
)

target_include_directories(monitoreo_lib PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/include
)

target_link_libraries(monitoreo_lib 
    cjson::cjson
    ${CMAKE_DL_LIBS}  # para dlopen/dlsym
)

if(PROM_LIB)
    target_link_libraries(monitoreo_lib ${PROM_LIB})
endif()

# Test básico
add_executable(test_main unit/test_main.c)

target_include_directories(test_main PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_main 
    ${UNITY_TARGET}
    cjson::cjson
)

# Test de métricas
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
    ${CMAKE_SOURCE_DIR}/src/shell_exec.c
)

target_include_directories(test_shell PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/include
)

target_link_libraries(test_shell 
    monitoreo_lib
    ${UNITY_TARGET}
    cjson::cjson
    Threads::Threads
)

if(PROM_LIB)
    target_link_libraries(test_shell ${PROM_LIB})
endif()

# Add tests to CTest
add_test(NAME unit_tests COMMAND test_main)
add_test(NAME metrics_tests COMMAND test_metrics)
add_test(NAME shell_tests COMMAND test_shell)

# Test de shell_exec (comandos externos y pipes)
add_executable(test_shell_exec 
    unit/test_shell_exec.c
    ${CMAKE_SOURCE_DIR}/src/shell.c
    ${CMAKE_SOURCE_DIR}/src/shell_commands.c
    ${CMAKE_SOURCE_DIR}/src/shell_ipc.c
    ${CMAKE_SOURCE_DIR}/src/shell_exec.c
)

target_include_directories(test_shell_exec PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/include
)

target_link_libraries(test_shell_exec 
    monitoreo_lib
    ${UNITY_TARGET}
    cjson::cjson
    Threads::Threads
)

if(PROM_LIB)
    target_link_libraries(test_shell_exec ${PROM_LIB})
endif()

add_test(NAME shell_exec_tests COMMAND test_shell_exec)

# Test de memory manager
add_executable(test_memory_manager unit/test_memory_manager.c)

target_include_directories(test_memory_manager PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_memory_manager 
    monitoreo_lib 
    ${UNITY_TARGET}
    cjson::cjson
)

add_test(NAME memory_manager_tests COMMAND test_memory_manager)

# Test de storage
add_executable(test_storage unit/test_storage.c)

target_include_directories(test_storage PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_storage 
    monitoreo_lib 
    ${UNITY_TARGET}
    cjson::cjson
)

add_test(NAME storage_tests COMMAND test_storage)

# Test de sandbox
add_executable(test_sandbox unit/test_sandbox.c)

target_include_directories(test_sandbox PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_sandbox 
    monitoreo_lib 
    ${UNITY_TARGET}
    cjson::cjson
    ${CMAKE_DL_LIBS}
)

add_test(NAME sandbox_tests COMMAND test_sandbox)
