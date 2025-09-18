# Buscar paquetes necesarios
find_package(unity REQUIRED)
find_package(cjson REQUIRED)

# Unit tests
add_executable(test_main unit/test_main.c)

# Configurar include directories usando variables de CMake
target_include_directories(test_main PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# Enlazar librerías
target_link_libraries(test_main 
    unity::unity
    cjson::cjson
)

# Add tests to CTest
add_test(NAME unit_tests COMMAND test_main)
