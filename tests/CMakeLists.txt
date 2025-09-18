# Buscar paquetes necesarios
find_package(cjson REQUIRED)

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

# Unit tests
add_executable(test_main unit/test_main.c)

# Configurar include directories
target_include_directories(test_main PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# Enlazar librerías
target_link_libraries(test_main 
    ${UNITY_TARGET}
    cjson::cjson
)

# Add tests to CTest
add_test(NAME unit_tests COMMAND test_main)