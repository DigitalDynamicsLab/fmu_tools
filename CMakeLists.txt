
cmake_minimum_required(VERSION 3.21)

cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction i.e. CMAKE_MSVC_RUNTIME_LIBRARY


project(fmu_tools)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# Set the minimum required C++ standard to C++14, allow C++17 if available
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

mark_as_advanced(FORCE CMAKE_INSTALL_PREFIX)

# The FMU model identifier must be set by the caller. Use a cached variable
set(FMU_MODEL_IDENTIFIER "myFmuComponent" CACHE STRING "FMU model identifier.")
set(FMU_RESOURCES_DIRECTORY "${CMAKE_SOURCE_DIR}/demo/my_resources" CACHE PATH "FMU resources directory; will be copied.")

# Optional setting: prefix for all messages issued from this script (default: empty).
if(NOT FMU_MSG_PREFIX)
  set(FMU_MSG_PREFIX "")
endif()

# Optional setting: output location for the FMU directory (default: CMAKE_BINARY_DIR)
if(NOT FMU_MAIN_DIRECTORY)
  set(FMU_MAIN_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# Set directory and filename for the generated FMU
set(FMU_DIRECTORY "${FMU_MAIN_DIRECTORY}/${FMU_MODEL_IDENTIFIER}")
set(FMU_FILENAME "${FMU_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu")
message(STATUS "${FMU_MSG_PREFIX}FMU to be created: \"${FMU_FILENAME}\"")

set(FMU_UNPACK_DIRECTORY "${FMU_DIRECTORY}/.${FMU_MODEL_IDENTIFIER}/")

set(TESTING_ENABLED_OS "win64" "linux64")

# -------------------------------------------------
# Developer notes
# -------------------------------------------------
# The ${FMU_MODEL_IDENTIFIER} target generates the main shared library of the FMU.
# Among the various functions exposed by the library, there is one dedicated to the 'modelDescription.xml' generation.
# This function has to called by a second target (fmu_modeldescription_generator) that instantiates the FmuComponent class of the library
# and makes it generate the 'modelDescription.xml' file.
# This should happen after the main target is built, but before the FMU is zipped.
#
# The testing relies on the fmuChecker tool: an additional target is calling it as pre-build step,
# while during the actual execution of the test, the _output_ of the fmuChecker is parsed to check the FMU compliance.


# -------------------------------------------------
# Set Architecture and Operating System Flags
# -------------------------------------------------
set(UNKNOWN_TARGET_SYSTEM FALSE)
if(WIN32)
    set(FMI_PLATFORM "win")
elseif(APPLE)
    set(FMI_PLATFORM "darwin")
elseif(CYGWIN)
    set(FMI_PLATFORM "cygwin")
elseif(UNIX)
    set(FMI_PLATFORM "linux")
else()
    set(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FMI_PLATFORM "${FMI_PLATFORM}64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(FMI_PLATFORM "${FMI_PLATFORM}32")
else()
    set(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(UNKNOWN_TARGET_SYSTEM)
    message(WARNING "Cannot retrieve target operating system or architecture. Please specify it manually through TARGET_SYSTEM variable (e.g. win64)")
    set(TARGET_SYSTEM "win64" CACHE STRING "Target operating system and architecture (e.g. win64, linux32, ...)")
    mark_as_advanced(CLEAR TARGET_SYSTEM)
else()
    message(STATUS "${FMU_MSG_PREFIX}Architecture: ${FMI_PLATFORM}")
    mark_as_advanced(FORCE TARGET_SYSTEM)
endif()

set(FMU_RUNTIME_OUTPUT_DIRECTORY "${FMU_DIRECTORY}/binaries/${FMI_PLATFORM}/$<0:>")




# -------------------------------------------------
# Generated GUID
# -------------------------------------------------
set(UUID_ALPHABET "0123456789abcdef")
string(RANDOM LENGTH 8  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE1)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE2)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE3)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE4)
string(RANDOM LENGTH 12 ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE5)
set(FMU_GUID_NAMESPACE_RANDOM "${FMU_NAMESPACE1}-${FMU_NAMESPACE2}-${FMU_NAMESPACE3}-${FMU_NAMESPACE4}-${FMU_NAMESPACE5}")
set(FMU_GUID_NAMESPACE ${FMU_GUID_NAMESPACE_RANDOM} CACHE STRING "FMU Namespace used to generated FMU GUID; replace with your own.")
message(STATUS "${FMU_MSG_PREFIX}FMU_GUID_NAMESPACE: ${FMU_GUID_NAMESPACE}")
string(UUID FMU_GUID NAMESPACE ${FMU_GUID_NAMESPACE} NAME ${FMU_MODEL_IDENTIFIER} TYPE SHA1)
message(STATUS "${FMU_MSG_PREFIX}FMU_GUID: ${FMU_GUID}")



# -------------------------------------------------
# Target: compiles FMU source code
# -------------------------------------------------

set(USE_EXTERNAL_FMU_COMPONENT_SOURCES OFF CACHE BOOL "Let external users to add problem-specific sources.")
set(USE_CUSTOM_TYPESPLATFORM OFF CACHE BOOL "Use custom types platform definition.")

set(FMU_SOURCES FmuToolsCommon.h FmuToolsExport.h FmuToolsExport.cpp TypesVariantsDefault.h)
source_group("fmu_tools" FILES ${FMU_SOURCES})


if(USE_CUSTOM_TYPESPLATFORM)
    message(STATUS "${FMU_MSG_PREFIX}Using custom 'fmi2TypesPlatform'.")
    target_compile_definitions(${FMU_MODEL_IDENTIFIER} PUBLIC FMITYPESPLATFORM_CUSTOM)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h")
        file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/TypesVariantsTemplate.h" "${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h")
        message(STATUS "${FMU_MSG_PREFIX}Created template 'TypesVariantsCustom.h'. Please make your changes there.")
    endif()
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/fmi2TypesPlatformCustom.h")
        file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fmi2_headers/fmi2TypesPlatform.h" "${CMAKE_SOURCE_DIR}/fmi2TypesPlatformCustom.h")
        message(STATUS "${FMU_MSG_PREFIX}Created template 'fmi2TypesPlatformCustom.h'. Please make your changes there.")
    endif()
    list(APPEND FMU_SOURCES ${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h)
else()
    message(STATUS "${FMU_MSG_PREFIX}Using default fmi2TypesPlatform.")
    list(APPEND FMU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/TypesVariantsDefault.h)
endif()


mark_as_advanced(USE_EXTERNAL_FMU_COMPONENT_SOURCES)
if (NOT USE_EXTERNAL_FMU_COMPONENT_SOURCES)
    message(STATUS "${FMU_MSG_PREFIX}Using default sources for \"${FMU_MODEL_IDENTIFIER}\" target.")
    list(APPEND FMU_SOURCES demo/myFmuComponent.h demo/myFmuComponent.cpp)
else()
    message(STATUS "${FMU_MSG_PREFIX}Add custom sources to ${FMU_MODEL_IDENTIFIER} target through 'target_sources(${FMU_MODEL_IDENTIFIER} PRIVATE <yoursourcefiles>)'")
endif()

add_library(${FMU_MODEL_IDENTIFIER} SHARED ${FMU_SOURCES})


target_include_directories(${FMU_MODEL_IDENTIFIER} PRIVATE "${CMAKE_SOURCES_DIR}")
target_compile_definitions(${FMU_MODEL_IDENTIFIER} PUBLIC FMU_MODEL_IDENTIFIER="${FMU_MODEL_IDENTIFIER}")
target_compile_definitions(${FMU_MODEL_IDENTIFIER} PRIVATE FMU_GUID="${FMU_GUID}")
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES OUTPUT_NAME ${FMU_MODEL_IDENTIFIER})


set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})

# -------------------------------------------------
# Target: generates modelDescription.xml
# -------------------------------------------------
if (NOT TARGET fmu_modeldescription_generator)
    add_executable(fmu_modeldescription_generator FmuToolsRuntimeLinking.h fmu_modeldescription_generator.cpp)
    target_link_libraries(fmu_modeldescription_generator PRIVATE ${CMAKE_DL_LIBS})
    set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})
    set_target_properties(fmu_modeldescription_generator PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
endif()

add_dependencies(${FMU_MODEL_IDENTIFIER} fmu_modeldescription_generator)

add_custom_command(TARGET ${FMU_MODEL_IDENTIFIER} POST_BUILD
    COMMENT "\nGenerating modelDescription.xml for '${FMU_RUNTIME_OUTPUT_DIRECTORY}/${FMU_MODEL_IDENTIFIER}${CMAKE_SHARED_LIBRARY_SUFFIX}' into '${FMU_DIRECTORY}'"
    COMMAND $<TARGET_FILE:fmu_modeldescription_generator> ${FMU_RUNTIME_OUTPUT_DIRECTORY} ${FMU_MODEL_IDENTIFIER}${CMAKE_SHARED_LIBRARY_SUFFIX} ${FMU_DIRECTORY}
    BYPRODUCTS "${FMU_DIRECTORY}/modelDescription.xml"
    WORKING_DIRECTORY ${FMU_DIRECTORY}
)

if (EXISTS ${FMU_RESOURCES_DIRECTORY})
    #TODO: is there the need to copy the resources folder before creating the zip file?
    message(STATUS "Found resources folder: \"${FMU_RESOURCES_DIRECTORY}\"")
    add_custom_command(TARGET ${FMU_MODEL_IDENTIFIER} POST_BUILD
        COMMENT "\nCopying ${FMU_RESOURCES_DIRECTORY} to ${FMU_DIRECTORY}/resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FMU_RESOURCES_DIRECTORY}" "${FMU_DIRECTORY}/resources"
        BYPRODUCTS "${FMU_DIRECTORY}/resources"
        WORKING_DIRECTORY ${FMU_DIRECTORY}
    )
else()
    message(STATUS "No resources directory specified or non existing.")
endif()

add_custom_command(TARGET ${FMU_MODEL_IDENTIFIER} POST_BUILD
    COMMENT "\nCreating FMU file: ${FMU_MODEL_IDENTIFIER}.fmu"
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${FMU_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu" "${FMU_DIRECTORY}/modelDescription.xml" "${FMU_RUNTIME_OUTPUT_DIRECTORY}" "${FMU_DIRECTORY}/resources" "--format=zip"
    BYPRODUCTS "${FMU_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu"
    WORKING_DIRECTORY ${FMU_DIRECTORY}
)

# -------------------------------------------------
# Target that runs the compiled fmu
# -------------------------------------------------
set(BUILD_FMU_HOST ON CACHE BOOL "Build fmu_host to run FMUs.")

if (BUILD_FMU_HOST)
    set(fmu_host_customname fmu_host_${FMU_MODEL_IDENTIFIER})
    add_executable(${fmu_host_customname})
    if (NOT USE_EXTERNAL_FMU_COMPONENT_SOURCES)
        target_sources(${fmu_host_customname} PRIVATE fmu_host.cpp)
    endif()
    target_include_directories(${fmu_host_customname} PRIVATE "${CMAKE_SOURCES_DIR}")
    target_compile_definitions(${fmu_host_customname} PUBLIC FMU_UNPACK_DIRECTORY="${FMU_UNPACK_DIRECTORY}")
    target_compile_definitions(${fmu_host_customname} PUBLIC FMU_DIRECTORY="${FMU_DIRECTORY}")
    target_compile_definitions(${fmu_host_customname} PUBLIC FMU_FILENAME="${FMU_FILENAME}")
    target_compile_definitions(${fmu_host_customname} PUBLIC FMU_OS_SUFFIX="${FMI_PLATFORM}")
    target_compile_definitions(${fmu_host_customname} PUBLIC SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "6.0")
        target_link_options(${fmu_host_customname} PRIVATE "LINKER:-as-needed")
        target_link_libraries(${fmu_host_customname} PRIVATE stdc++fs)
        target_link_libraries(${fmu_host_customname} PRIVATE ${CMAKE_DL_LIBS})
    endif()
    add_dependencies(${fmu_host_customname} ${FMU_MODEL_IDENTIFIER})
endif()


# -------------------------------------------------
# Target that runs the compiled and extracted fmu
# -------------------------------------------------
set(BUILD_TESTING ON CACHE BOOL "Enable testing.")

if (BUILD_TESTING)
    # # -------------------------------------------------
    # # Testing
    # # -------------------------------------------------
    if(${FMI_PLATFORM} IN_LIST TESTING_ENABLED_OS)
        set(TESTING_ENABLED ON)
        message(STATUS "${FMU_MSG_PREFIX}Testing enabled.")
        
        ## Call fmuChecker on the FMU, parse its output
        set(test_parseFmuChecker_customname test_parseFmuChecker_${FMU_MODEL_IDENTIFIER})
        set(test_buildFmuChecker_customname test_buildFmuChecker_${FMU_MODEL_IDENTIFIER})
        set(build_test_parseFmuChecker_customname build_test_parseFmuChecker_${FMU_MODEL_IDENTIFIER})

        set(FMUCHECKER_LOG "${CMAKE_BINARY_DIR}/${FMU_MODEL_IDENTIFIER}_log.txt")
        set(FMUCHECKER_RESULTS "${CMAKE_BINARY_DIR}/${FMU_MODEL_IDENTIFIER}_results.txt")

        add_executable(${test_parseFmuChecker_customname} ${CMAKE_CURRENT_SOURCE_DIR}/test_parseFmuChecker.cpp)
        target_compile_definitions(${test_parseFmuChecker_customname} PRIVATE FMUCHECKER_LOG="${FMUCHECKER_LOG}")
        add_dependencies(${test_parseFmuChecker_customname} ${FMU_MODEL_IDENTIFIER})

        ## call the execution of fmuChecker
        add_custom_command(TARGET ${test_parseFmuChecker_customname}
                        PRE_BUILD
                        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/fmuChecker/fmuCheck.${FMI_PLATFORM}${CMAKE_EXECUTABLE_SUFFIX}" -e \"${FMUCHECKER_LOG}\" -o \"${FMUCHECKER_RESULTS}\" -k cs -s 1 -h 1e-3 \"${FMU_FILENAME}\"
                        COMMENT "\nTesting FMU with fmuChecker")

        if(PROJECT_IS_TOP_LEVEL)
            ## enable the testing
            enable_testing()
        endif()

        ## the 'fmuChecker_requirements' fixtures is registered
        ## 'build_test_parseFmuChecker_customname' is set as a requirement of type SETUP i.e. called before the FIXTURES_REQUIRED target
        ## 'TestFmuComponent_${FMU_MODEL_IDENTIFIER}' is the one requiring the other targets
        add_test(NAME TestFmuComponent_${FMU_MODEL_IDENTIFIER}
                 COMMAND ${test_parseFmuChecker_customname})

        add_test(${build_test_parseFmuChecker_customname}
            "${CMAKE_COMMAND}"
            --build "${CMAKE_BINARY_DIR}"
            # --target ${test_parseFmuChecker_customname
        )

        set_tests_properties(${build_test_parseFmuChecker_customname} PROPERTIES FIXTURES_SETUP fmuChecker_requirements_${FMU_MODEL_IDENTIFIER})
        set_tests_properties(TestFmuComponent_${FMU_MODEL_IDENTIFIER} PROPERTIES FIXTURES_REQUIRED fmuChecker_requirements_${FMU_MODEL_IDENTIFIER})
    else()
        message(STATUS "${FMU_MSG_PREFIX}Testing not enabled. Available only for: ${TESTING_ENABLED_OS}")
    endif()
else()
    set(TESTING_ENABLED OFF)
    message(STATUS "${FMU_MSG_PREFIX}Testing disabled.")
endif(BUILD_TESTING)


# -------------------------------------------------
# Propagate variable to upper levels
# -------------------------------------------------
# Make variables set in this script available to the fetcher projects
if(NOT PROJECT_IS_TOP_LEVEL)
    set(FMI_PLATFORM ${FMI_PLATFORM} PARENT_SCOPE)
    set(FMU_DIRECTORY ${FMU_DIRECTORY} PARENT_SCOPE)
    set(FMU_FILENAME ${FMU_FILENAME} PARENT_SCOPE)
    set(FMU_RUNTIME_OUTPUT_DIRECTORY "${FMU_RUNTIME_OUTPUT_DIRECTORY}" PARENT_SCOPE)
    set(FMU_SOURCES ${FMU_SOURCES} PARENT_SCOPE)
    set(TESTING_ENABLED ${TESTING_ENABLED} PARENT_SCOPE)
endif()
