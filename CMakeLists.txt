
cmake_minimum_required(VERSION 3.15)


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

set(FMU_MODEL_IDENTIFIER "FmuComponent" CACHE STRING "FMU Model Identifier: should fulfill operating system and C-function naming standards.")
SET(FMU_FILENAME "${FMU_MAIN_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu")
MESSAGE(STATUS "FMU to be created: \"${FMU_FILENAME}\"")

SET(FMU_UNPACK_DIRECTORY "${FMU_MAIN_DIRECTORY}/.${FMU_MODEL_IDENTIFIER}/")


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
    MESSAGE(STATUS "Architecture: ${OS_SUFFIX}")
    MARK_AS_ADVANCED(FORCE TARGET_SYSTEM)
endif()

SET(FMU_RUNTIME_OUTPUT_DIRECTORY "${FMU_MAIN_DIRECTORY}/binaries/${OS_SUFFIX}/$<0:>")



# -------------------------------------------------
# Target: compiles FMU source code
# -------------------------------------------------
add_library(FmuComponent SHARED FmuToolsCommon.h FmuToolsCommon.cpp FmuToolsExport.h FmuToolsExport.cpp TypesVariants.h FmuComponent.h FmuComponent.cpp)
target_include_directories(FmuComponent PUBLIC "variant")
target_include_directories(FmuComponent PUBLIC "fmi2_headers")
target_include_directories(FmuComponent PUBLIC "rapidxml")
target_compile_definitions(FmuComponent PUBLIC FMU_MODEL_IDENTIFIER="${FMU_MODEL_IDENTIFIER}")
target_compile_definitions(FmuComponent PUBLIC FMU_OS_SUFFIX="${OS_SUFFIX}")
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
target_compile_definitions(fmu_modeldescription_generator PUBLIC FMU_RUNTIME_OUTPUT_DIRECTORY="${FMU_RUNTIME_OUTPUT_DIRECTORY}")
target_compile_definitions(fmu_modeldescription_generator PUBLIC FMU_MAIN_DIRECTORY="${FMU_MAIN_DIRECTORY}")
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


# # -------------------------------------------------
# # Target that extracts back the created FMU
# # -------------------------------------------------
# add_custom_target(UNPACK_FMU
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${FMU_UNPACK_DIRECTORY}
#     COMMAND ${CMAKE_COMMAND} -E tar "xzf" "${FMU_MAIN_DIRECTORY}/${FMU_MODEL_IDENTIFIER}.fmu"
#     WORKING_DIRECTORY ${FMU_UNPACK_DIRECTORY}
#     COMMENT "Unpacking FMU file: \"${FMU_MODEL_IDENTIFIER}.fmu\" into \"${FMU_UNPACK_DIRECTORY}\"."
# )
# add_dependencies(UNPACK_FMU PACK_FMU)

# # takes care of creating the unpacking folder
# add_custom_command(
#     TARGET UNPACK_FMU PRE_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${FMU_UNPACK_DIRECTORY}
#     COMMENT "Creating unpacking folder ${FMU_UNPACK_DIRECTORY}."
# )


# -------------------------------------------------
# Target that runs the compiled and extracted fmu
# -------------------------------------------------
add_executable(fmu_host_standalone fmu_host_standalone.cpp FmuToolsImport.hpp)
target_include_directories(fmu_host_standalone PUBLIC "fmi2_headers")
target_include_directories(fmu_host_standalone PUBLIC "rapidxml")
target_include_directories(fmu_host_standalone PUBLIC "miniz-cpp")
target_compile_definitions(fmu_host_standalone PUBLIC FMU_UNPACK_DIRECTORY="${FMU_UNPACK_DIRECTORY}")
target_compile_definitions(fmu_host_standalone PUBLIC FMU_MAIN_DIRECTORY="${FMU_MAIN_DIRECTORY}")
target_compile_definitions(fmu_host_standalone PUBLIC FMU_FILENAME="${FMU_FILENAME}")
target_compile_definitions(fmu_host_standalone PUBLIC FMU_OS_SUFFIX="${OS_SUFFIX}")
target_compile_definitions(fmu_host_standalone PUBLIC SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "6.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
    target_link_options(fmu_host_standalone PRIVATE "LINKER:-as-needed")
    target_link_libraries(fmu_host_standalone PRIVATE stdc++fs)
    target_link_libraries(fmu_host_standalone PRIVATE ${CMAKE_DL_LIBS})
endif()
set_property(TARGET fmu_host_standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${FMU_UNPACK_DIRECTORY})
# add_dependencies(fmu_host_standalone UNPACK_FMU)


# -------------------------------------------------
# Target that runs the fmu project, but skips the modelDescription.xml loading and other auxiliary things
# -------------------------------------------------
add_executable(nonfmu_host nonfmu_host.cpp FmuToolsCommon.h FmuToolsImport.hpp)
target_include_directories(nonfmu_host PUBLIC "fmi2_headers")
target_link_libraries(nonfmu_host PUBLIC FmuComponent)
# target_include_directories(nonfmu_host PUBLIC "rapidxml")
set_property(TARGET fmu_host_standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
