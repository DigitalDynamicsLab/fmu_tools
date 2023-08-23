
cmake_minimum_required(VERSION 3.15)


project(fmu_generator_standalone)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
mark_as_advanced(FORCE CMAKE_INSTALL_PREFIX)

set(FMU_MODEL_IDENTIFIER "fmu_instance" CACHE STRING "FMU Model Identifier")

add_library(fmu_instance SHARED FmuToolsCommon.hpp fmu_entity.h fmu_entity.cpp fmu_instance.h fmu_instance.cpp)
target_include_directories(fmu_instance PUBLIC "fmi2_headers")
target_include_directories(fmu_instance PUBLIC "rapidxml")
target_compile_definitions(fmu_instance PUBLIC ${FMU_MODEL_IDENTIFIER})
set_target_properties(fmu_instance PROPERTIES OUTPUT_NAME ${FMU_MODEL_IDENTIFIER})

# set_property(TARGET fmu_instance PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# -------------------------------------------------
# Set Architecture and Operating System Flags
# -------------------------------------------------
SET(UNKNOWN_TARGET_SYSTEM FALSE)
if(WIN32)
    SET(OS_SUFFIX "win")
elseif(APPLE)
    SET(OS_SUFFIX "darwin")
elseif(CYGWIN)
    SET(OS_SUFFIX "cygwin")
elseif(UNIX)
    SET(OS_SUFFIX "linux")
else()
    SET(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(OS_SUFFIX "${OS_SUFFIX}64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(OS_SUFFIX "${OS_SUFFIX}32")
else()
    SET(UNKNOWN_TARGET_SYSTEM TRUE)
endif()

if(UNKNOWN_TARGET_SYSTEM)
    MESSAGE(WARNING "Cannot retrieve target operating system or architecture. Please specify it manually through TARGET_SYSTEM variable (e.g. win64)")
    set(TARGET_SYSTEM "win64" CACHE STRING "Target operating system and architecture (e.g. win64, linux32, ...)")
    MARK_AS_ADVANCED(CLEAR TARGET_SYSTEM)
else()
    MESSAGE(STATUS "Dedicated FMU folder: ${OS_SUFFIX}")
    MARK_AS_ADVANCED(FORCE TARGET_SYSTEM)
endif()


# -------------------------------------------------
# Set output directories
# -------------------------------------------------

SET(FMU_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/binaries/${OS_SUFFIX}/$<0:>")
set_target_properties(fmu_instance PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_instance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_instance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_instance PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_instance PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_instance PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})

# -------------------------------------------------
# Create the FMU post-build command
# -------------------------------------------------
add_custom_command(
    TARGET fmu_instance
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/modelDescription.xml" "${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${FMU_MODEL_IDENTIFIER}.fmu" "modelDescription.xml" "${FMU_RUNTIME_OUTPUT_DIRECTORY}" "--format=zip"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating FMU file: ${FMU_MODEL_IDENTIFIER}.fmu"
)


# -------------------------------------------------
# Target that runs the compiled fmu
# -------------------------------------------------
add_executable(fmu_host_standalone fmu_host_standalone.cpp FmuToolsCommon.hpp FmuToolsStandalone.hpp)
target_include_directories(fmu_host_standalone PUBLIC "fmi2_headers")
target_include_directories(fmu_host_standalone PUBLIC "rapidxml")
target_compile_definitions(fmu_host_standalone PUBLIC FMU_PATH_DEFAULT="${CMAKE_BINARY_DIR}")
set_property(TARGET fmu_host_standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(fmu_host_standalone fmu_instance)

# -------------------------------------------------
# Target that runs the fmu project, but skips the modelDescription.xml loading and other auxiliary things
# -------------------------------------------------
add_executable(nonfmu_host nonfmu_host.cpp FmuToolsCommon.hpp FmuToolsStandalone.hpp)
target_include_directories(nonfmu_host PUBLIC "fmi2_headers")
target_link_libraries(nonfmu_host PUBLIC fmu_instance)
# target_include_directories(nonfmu_host PUBLIC "rapidxml")
set_property(TARGET fmu_host_standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})



# # Create a custom command to perform the copy
# add_custom_command(
#     OUTPUT "${CMAKE_BINARY_DIR}/include"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory "${source_dir}" "${destination_dir}"
#     COMMENT "Copying folder from ${source_dir} to ${destination_dir}"
# )

# # Create a custom target that depends on the custom command
# add_custom_target(copy_target ALL
#     DEPENDS "${destination_dir}"
# )