########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND cjson_COMPONENT_NAMES cjson)
list(REMOVE_DUPLICATES cjson_COMPONENT_NAMES)
if(DEFINED cjson_FIND_DEPENDENCY_NAMES)
  list(APPEND cjson_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES cjson_FIND_DEPENDENCY_NAMES)
else()
  set(cjson_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(cjson_PACKAGE_FOLDER_RELEASE "/home/facu/.conan2/p/b/cjson5deb5fce87fb4/p")
set(cjson_BUILD_MODULES_PATHS_RELEASE )


set(cjson_INCLUDE_DIRS_RELEASE "${cjson_PACKAGE_FOLDER_RELEASE}/include")
set(cjson_RES_DIRS_RELEASE )
set(cjson_DEFINITIONS_RELEASE )
set(cjson_SHARED_LINK_FLAGS_RELEASE )
set(cjson_EXE_LINK_FLAGS_RELEASE )
set(cjson_OBJECTS_RELEASE )
set(cjson_COMPILE_DEFINITIONS_RELEASE )
set(cjson_COMPILE_OPTIONS_C_RELEASE )
set(cjson_COMPILE_OPTIONS_CXX_RELEASE )
set(cjson_LIB_DIRS_RELEASE "${cjson_PACKAGE_FOLDER_RELEASE}/lib")
set(cjson_BIN_DIRS_RELEASE )
set(cjson_LIBRARY_TYPE_RELEASE STATIC)
set(cjson_IS_HOST_WINDOWS_RELEASE 0)
set(cjson_LIBS_RELEASE cjson)
set(cjson_SYSTEM_LIBS_RELEASE m)
set(cjson_FRAMEWORK_DIRS_RELEASE )
set(cjson_FRAMEWORKS_RELEASE )
set(cjson_BUILD_DIRS_RELEASE )
set(cjson_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(cjson_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cjson_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cjson_COMPILE_OPTIONS_C_RELEASE}>")
set(cjson_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cjson_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cjson_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cjson_EXE_LINK_FLAGS_RELEASE}>")


set(cjson_COMPONENTS_RELEASE cjson)
########### COMPONENT cjson VARIABLES ############################################

set(cjson_cjson_INCLUDE_DIRS_RELEASE "${cjson_PACKAGE_FOLDER_RELEASE}/include")
set(cjson_cjson_LIB_DIRS_RELEASE "${cjson_PACKAGE_FOLDER_RELEASE}/lib")
set(cjson_cjson_BIN_DIRS_RELEASE )
set(cjson_cjson_LIBRARY_TYPE_RELEASE STATIC)
set(cjson_cjson_IS_HOST_WINDOWS_RELEASE 0)
set(cjson_cjson_RES_DIRS_RELEASE )
set(cjson_cjson_DEFINITIONS_RELEASE )
set(cjson_cjson_OBJECTS_RELEASE )
set(cjson_cjson_COMPILE_DEFINITIONS_RELEASE )
set(cjson_cjson_COMPILE_OPTIONS_C_RELEASE "")
set(cjson_cjson_COMPILE_OPTIONS_CXX_RELEASE "")
set(cjson_cjson_LIBS_RELEASE cjson)
set(cjson_cjson_SYSTEM_LIBS_RELEASE m)
set(cjson_cjson_FRAMEWORK_DIRS_RELEASE )
set(cjson_cjson_FRAMEWORKS_RELEASE )
set(cjson_cjson_DEPENDENCIES_RELEASE )
set(cjson_cjson_SHARED_LINK_FLAGS_RELEASE )
set(cjson_cjson_EXE_LINK_FLAGS_RELEASE )
set(cjson_cjson_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(cjson_cjson_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cjson_cjson_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cjson_cjson_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cjson_cjson_EXE_LINK_FLAGS_RELEASE}>
)
set(cjson_cjson_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cjson_cjson_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cjson_cjson_COMPILE_OPTIONS_C_RELEASE}>")