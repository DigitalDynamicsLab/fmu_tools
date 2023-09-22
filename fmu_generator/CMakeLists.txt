
cmake_minimum_required(VERSION 3.21)

cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction i.e. CMAKE_MSVC_RUNTIME_LIBRARY


project(fmu_generator)
SET(CMAKE_SHARED_LIBRARY_PREFIX "")

# Set the minimum required C++ standard to C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check if the compiler supports C++17
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

if(COMPILER_SUPPORTS_CXX17)
    # Enable C++17 if supported
    set(CMAKE_CXX_STANDARD 17)
endif()


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

mark_as_advanced(FORCE CMAKE_INSTALL_PREFIX)

SET(FMU_MAIN_DIRECTORY ${CMAKE_BINARY_DIR})

set(FMU_MODEL_IDENTIFIER "myFmuIdentifier" CACHE STRING "FMU Model Identifier: should fulfill operating system and C-function naming standards.")
SET(FMU_FILENAME "${FMU_MAIN_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu")
MESSAGE(STATUS "FMU to be created: \"${FMU_FILENAME}\"")
set(ENABLE_GUID_CHECK ON CACHE BOOL "Enable GUID checking.")

SET(FMU_UNPACK_DIRECTORY "${FMU_MAIN_DIRECTORY}/.${FMU_MODEL_IDENTIFIER}/")

set(TESTING_ENABLED_OS "win64" "linux64")

# -------------------------------------------------
# Set Architecture and Operating System Flags
# -------------------------------------------------
SET(UNKNOWN_TARGET_SYSTEM FALSE)
if(WIN32)
    SET(FMI_PLATFORM "win")
elseif(APPLE)
    SET(FMI_PLATFORM "darwin")
elseif(CYGWIN)
    SET(FMI_PLATFORM "cygwin")
elseif(UNIX)
    SET(FMI_PLATFORM "linux")
else()
    SET(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(FMI_PLATFORM "${FMI_PLATFORM}64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(FMI_PLATFORM "${FMI_PLATFORM}32")
else()
    SET(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(UNKNOWN_TARGET_SYSTEM)
    MESSAGE(WARNING "Cannot retrieve target operating system or architecture. Please specify it manually through TARGET_SYSTEM variable (e.g. win64)")
    set(TARGET_SYSTEM "win64" CACHE STRING "Target operating system and architecture (e.g. win64, linux32, ...)")
    MARK_AS_ADVANCED(CLEAR TARGET_SYSTEM)
else()
    MESSAGE(STATUS "Architecture: ${FMI_PLATFORM}")
    MARK_AS_ADVANCED(FORCE TARGET_SYSTEM)
endif()

SET(FMU_RUNTIME_OUTPUT_DIRECTORY "${FMU_MAIN_DIRECTORY}/binaries/${FMI_PLATFORM}/$<0:>")




# -------------------------------------------------
# Generated GUID
# -------------------------------------------------
SET(UUID_ALPHABET "0123456789abcdef")
string(RANDOM LENGTH 8  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE1)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE2)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE3)
string(RANDOM LENGTH 4  ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE4)
string(RANDOM LENGTH 12 ALPHABET ${UUID_ALPHABET} FMU_NAMESPACE5)
SET(FMU_GUID_NAMESPACE_RANDOM "${FMU_NAMESPACE1}-${FMU_NAMESPACE2}-${FMU_NAMESPACE3}-${FMU_NAMESPACE4}-${FMU_NAMESPACE5}")
set(FMU_GUID_NAMESPACE ${FMU_GUID_NAMESPACE_RANDOM} CACHE STRING "FMU Namespace used to generated FMU GUID; replace with your own.")
MESSAGE(STATUS "FMU_GUID_NAMESPACE: ${FMU_GUID_NAMESPACE}")
string(UUID FMU_GUID NAMESPACE ${FMU_GUID_NAMESPACE} NAME ${FMU_MODEL_IDENTIFIER} TYPE SHA1)
MESSAGE(STATUS "FMU_GUID: ${FMU_GUID}")



# -------------------------------------------------
# Target: compiles FMU source code
# -------------------------------------------------

set(USE_EXTERNAL_FMU_COMPONENT_SOURCES OFF CACHE BOOL "Let external users to add problem-specific sources.")
set(USE_CUSTOM_TYPESPLATFORM OFF CACHE BOOL "Use custom types platform definition.")

SET(FMU_SOURCES FmuToolsCommon.h FmuToolsCommon.cpp FmuToolsExport.h FmuToolsExport.cpp TypesVariantsDefault.h)

# MESSAGE(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
# MESSAGE(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

IF(USE_CUSTOM_TYPESPLATFORM)
    MESSAGE(STATUS "Using custom 'fmi2TypesPlatform'.")
    target_compile_definitions(FmuComponent PUBLIC FMITYPESPLATFORM_CUSTOM)
    IF (NOT EXISTS "${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h")
        MESSAGE(STATUS "Created template 'TypesVariantsCustom.h'. Please make your changes there.")
        file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/TypesVariantsTemplate.h" "${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h")
    endif()
    IF (NOT EXISTS "${CMAKE_SOURCE_DIR}/fmi2TypesPlatformCustom.h")
        MESSAGE(STATUS "Created template 'fmi2TypesPlatformCustom.h'. Please make your changes there.")
        file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fmi2_headers/fmi2TypesPlatform.h" "${CMAKE_SOURCE_DIR}/fmi2TypesPlatformCustom.h")
    endif()
    LIST(APPEND FMU_SOURCES ${CMAKE_SOURCE_DIR}/TypesVariantsCustom.h)
ELSE()
    MESSAGE(STATUS "Using default fmi2TypesPlatform.")
    LIST(APPEND FMU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/TypesVariantsDefault.h)
ENDIF()


MARK_AS_ADVANCED(USE_EXTERNAL_FMU_COMPONENT_SOURCES)
IF (NOT USE_EXTERNAL_FMU_COMPONENT_SOURCES)
    MESSAGE(STATUS "Using default sources for FmuComponent target.")
    LIST(APPEND FMU_SOURCES FmuComponent.h FmuComponent.cpp)
ELSE()
    MESSAGE(STATUS "Add custom sources to FmuComponent target through 'target_sources(FmuComponent PRIVATE <yoursourcefiles>)'")
ENDIF()

add_library(FmuComponent SHARED ${FMU_SOURCES})


target_include_directories(FmuComponent PRIVATE "variant")
target_include_directories(FmuComponent PRIVATE "${CMAKE_SOURCES_DIR}")
target_include_directories(FmuComponent PUBLIC "fmi2_headers")
target_include_directories(FmuComponent PRIVATE "rapidxml")
target_compile_definitions(FmuComponent PUBLIC FMU_MODEL_IDENTIFIER="${FMU_MODEL_IDENTIFIER}")
target_compile_definitions(FmuComponent PRIVATE FMU_OS_SUFFIX="${FMI_PLATFORM}")
target_compile_definitions(FmuComponent PRIVATE FMU_GUID="${FMU_GUID}")
IF (ENABLE_GUID_CHECK)
    MESSAGE(STATUS "GUID check enabled.")
    target_compile_definitions(FmuComponent PRIVATE ENABLE_GUID_CHECK=1)
ELSE()
    MESSAGE(STATUS "GUID check disabled.")
    target_compile_definitions(FmuComponent PRIVATE ENABLE_GUID_CHECK=0)
ENDIF()

set_target_properties(FmuComponent PROPERTIES OUTPUT_NAME ${FMU_MODEL_IDENTIFIER})


set_target_properties(FmuComponent PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(FmuComponent PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})

# -------------------------------------------------
# Target: generates modelDescription.xml
# -------------------------------------------------
add_executable(fmu_modeldescription_generator fmu_modeldescription_generator.cpp)
target_compile_definitions(fmu_modeldescription_generator PRIVATE FMU_RUNTIME_OUTPUT_DIRECTORY="${FMU_RUNTIME_OUTPUT_DIRECTORY}")
target_compile_definitions(fmu_modeldescription_generator PRIVATE FMU_MAIN_DIRECTORY="${FMU_MAIN_DIRECTORY}")
target_compile_definitions(fmu_modeldescription_generator PRIVATE FMU_GUID="${FMU_GUID}")
IF (ENABLE_GUID_CHECK)
    target_compile_definitions(fmu_modeldescription_generator PRIVATE ENABLE_GUID_CHECK=1)
ELSE()
    target_compile_definitions(fmu_modeldescription_generator PRIVATE ENABLE_GUID_CHECK=0)
ENDIF()
target_link_libraries(fmu_modeldescription_generator PUBLIC FmuComponent)


set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_modeldescription_generator PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_modeldescription_generator PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})



# -------------------------------------------------
# Create the FMU post-build command
# -------------------------------------------------
add_custom_target(PACK_FMU
	COMMAND $<TARGET_FILE:fmu_modeldescription_generator>
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${FMU_MODEL_IDENTIFIER}.fmu" "modelDescription.xml" "${FMU_RUNTIME_OUTPUT_DIRECTORY}" "--format=zip"
    WORKING_DIRECTORY ${FMU_MAIN_DIRECTORY}
    COMMENT "Creating FMU file: ${FMU_MODEL_IDENTIFIER}.fmu"
)

add_dependencies(PACK_FMU fmu_modeldescription_generator)
add_dependencies(PACK_FMU FmuComponent)


# -------------------------------------------------
# Target that runs the compiled fmu
# -------------------------------------------------
add_executable(fmu_host FmuToolsImport.hpp)
IF (NOT USE_EXTERNAL_FMU_COMPONENT_SOURCES)
    target_sources(fmu_host PRIVATE fmu_host.cpp)
ENDIF()
target_include_directories(fmu_host PUBLIC "fmi2_headers")
target_include_directories(fmu_host PRIVATE "rapidxml")
target_include_directories(fmu_host PRIVATE "miniz-cpp")
target_include_directories(fmu_host PRIVATE "${CMAKE_SOURCES_DIR}")
target_compile_definitions(fmu_host PUBLIC FMU_UNPACK_DIRECTORY="${FMU_UNPACK_DIRECTORY}")
target_compile_definitions(fmu_host PUBLIC FMU_MAIN_DIRECTORY="${FMU_MAIN_DIRECTORY}")
target_compile_definitions(fmu_host PUBLIC FMU_FILENAME="${FMU_FILENAME}")
target_compile_definitions(fmu_host PUBLIC FMU_OS_SUFFIX="${FMI_PLATFORM}")
target_compile_definitions(fmu_host PUBLIC SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "6.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
    target_link_options(fmu_host PRIVATE "LINKER:-as-needed")
    target_link_libraries(fmu_host PRIVATE stdc++fs)
    target_link_libraries(fmu_host PRIVATE ${CMAKE_DL_LIBS})
endif()
add_dependencies(fmu_host PACK_FMU)


# -------------------------------------------------
# Target that runs the compiled and extracted fmu
# -------------------------------------------------
set(BUILD_TESTING ON CACHE BOOL "Enable testing.")

IF (BUILD_TESTING)
    # # -------------------------------------------------
    # # Testing
    # # -------------------------------------------------
    list(FIND TESTING_ENABLED_OS ${FMI_PLATFORM} ENABLE_TESTING_FLAG)
    IF(ENABLE_TESTING_FLAG EQUAL 0)
        SET(TESTING_ENABLED ON)
        MESSAGE(STATUS "Testing enabled.")
        ## Call fmuChecker on the FMU, parse its output
        SET(FMUCHECKER_LOG "${CMAKE_BINARY_DIR}/${FMU_MODEL_IDENTIFIER}_log.txt")
        SET(FMUCHECKER_RESULTS "${CMAKE_BINARY_DIR}/${FMU_MODEL_IDENTIFIER}_results.txt")
        add_executable(test_parseFmuChecker test_parseFmuChecker.cpp)
        target_compile_definitions(test_parseFmuChecker PRIVATE FMUCHECKER_LOG="${FMUCHECKER_LOG}")
        add_dependencies(test_parseFmuChecker PACK_FMU)

        ## call the execution of fmuChecker
        add_custom_command(TARGET test_parseFmuChecker
                        PRE_BUILD
                        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/fmuChecker/fmuCheck.${FMI_PLATFORM}${CMAKE_EXECUTABLE_SUFFIX}" "-e \"${FMUCHECKER_LOG}\" -o \"${FMUCHECKER_RESULTS}\" -k cs -s 1 -h 1e-3 \"${FMU_FILENAME}\""
                        COMMENT "Testing FMU with fmuChecker")

        ## enable the testing
        enable_testing()

        ## the 'fmuChecker_requirements' fixtures is registered
        ## 'build_test_parseFmuChecker' is set as a requirement of type SETUP i.e. called before the FIXTURES_REQUIRED target
        ## 'TestFmuComponent' is the one requiring the other targets
        add_test(NAME TestFmuComponent
                 COMMAND test_parseFmuChecker)

        add_test(build_test_parseFmuChecker
            "${CMAKE_COMMAND}"
            --build "${CMAKE_BINARY_DIR}"
            --target test_parseFmuChecker
        )

        set_tests_properties(build_test_parseFmuChecker PROPERTIES FIXTURES_SETUP fmuChecker_requirements)
        set_tests_properties(TestFmuComponent PROPERTIES FIXTURES_REQUIRED fmuChecker_requirements)
    ELSE()
        MESSAGE(STATUS "Testing not enabled. Available only for: ${TESTING_ENABLED_OS}")
    ENDIF()
ELSE()
    SET(TESTING_ENABLED OFF)
    MESSAGE(STATUS "Testing disabled.")
ENDIF(BUILD_TESTING)


# -------------------------------------------------
# Propagate variable to upper levels
# -------------------------------------------------
if(NOT PROJECT_IS_TOP_LEVEL)
    # make them available to fetcher projects
    SET(FMI_PLATFORM ${FMI_PLATFORM} PARENT_SCOPE)
    SET(FMU_RUNTIME_OUTPUT_DIRECTORY "${FMU_RUNTIME_OUTPUT_DIRECTORY}" PARENT_SCOPE)
    SET(FMU_SOURCES ${FMU_SOURCES} PARENT_SCOPE)
    SET(TESTING_ENABLED ${TESTING_ENABLED} PARENT_SCOPE)
endif()