########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(cJSON_FIND_QUIETLY)
    set(cJSON_MESSAGE_MODE VERBOSE)
else()
    set(cJSON_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cJSONTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${cjson_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(cJSON_VERSION_STRING "1.7.18")
set(cJSON_INCLUDE_DIRS ${cjson_INCLUDE_DIRS_RELEASE} )
set(cJSON_INCLUDE_DIR ${cjson_INCLUDE_DIRS_RELEASE} )
set(cJSON_LIBRARIES ${cjson_LIBRARIES_RELEASE} )
set(cJSON_DEFINITIONS ${cjson_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${cjson_BUILD_MODULES_PATHS_RELEASE} )
    message(${cJSON_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


