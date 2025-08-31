# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(cjson_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(cjson_FRAMEWORKS_FOUND_RELEASE "${cjson_FRAMEWORKS_RELEASE}" "${cjson_FRAMEWORK_DIRS_RELEASE}")

set(cjson_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET cjson_DEPS_TARGET)
    add_library(cjson_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET cjson_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${cjson_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${cjson_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### cjson_DEPS_TARGET to all of them
conan_package_library_targets("${cjson_LIBS_RELEASE}"    # libraries
                              "${cjson_LIB_DIRS_RELEASE}" # package_libdir
                              "${cjson_BIN_DIRS_RELEASE}" # package_bindir
                              "${cjson_LIBRARY_TYPE_RELEASE}"
                              "${cjson_IS_HOST_WINDOWS_RELEASE}"
                              cjson_DEPS_TARGET
                              cjson_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "cjson"    # package_name
                              "${cjson_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${cjson_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT cjson #############

        set(cjson_cjson_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(cjson_cjson_FRAMEWORKS_FOUND_RELEASE "${cjson_cjson_FRAMEWORKS_RELEASE}" "${cjson_cjson_FRAMEWORK_DIRS_RELEASE}")

        set(cjson_cjson_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET cjson_cjson_DEPS_TARGET)
            add_library(cjson_cjson_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET cjson_cjson_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${cjson_cjson_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${cjson_cjson_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${cjson_cjson_DEPENDENCIES_RELEASE}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'cjson_cjson_DEPS_TARGET' to all of them
        conan_package_library_targets("${cjson_cjson_LIBS_RELEASE}"
                              "${cjson_cjson_LIB_DIRS_RELEASE}"
                              "${cjson_cjson_BIN_DIRS_RELEASE}" # package_bindir
                              "${cjson_cjson_LIBRARY_TYPE_RELEASE}"
                              "${cjson_cjson_IS_HOST_WINDOWS_RELEASE}"
                              cjson_cjson_DEPS_TARGET
                              cjson_cjson_LIBRARIES_TARGETS
                              "_RELEASE"
                              "cjson_cjson"
                              "${cjson_cjson_NO_SONAME_MODE_RELEASE}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET cjson
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${cjson_cjson_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${cjson_cjson_LIBRARIES_TARGETS}>
                     )

        if("${cjson_cjson_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET cjson
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         cjson_cjson_DEPS_TARGET)
        endif()

        set_property(TARGET cjson APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${cjson_cjson_LINKER_FLAGS_RELEASE}>)
        set_property(TARGET cjson APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${cjson_cjson_INCLUDE_DIRS_RELEASE}>)
        set_property(TARGET cjson APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Release>:${cjson_cjson_LIB_DIRS_RELEASE}>)
        set_property(TARGET cjson APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${cjson_cjson_COMPILE_DEFINITIONS_RELEASE}>)
        set_property(TARGET cjson APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${cjson_cjson_COMPILE_OPTIONS_RELEASE}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET cjson::cjson APPEND PROPERTY INTERFACE_LINK_LIBRARIES cjson)

########## For the modules (FindXXX)
set(cjson_LIBRARIES_RELEASE cjson::cjson)
