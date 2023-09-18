cmake_minimum_required(VERSION 3.15)


project(fmu_generator_chrono)

set(FMU_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/../fmu_generator/" CACHE PATH "Path to fmu_generator.")

#########################
# Importing Chrono
#########################


LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/../chrono/lib")
find_package(Chrono
             OPTIONAL_COMPONENTS Irrlicht
             CONFIG)

if (NOT Chrono_FOUND)
  message(FATAL_ERROR "Could not find Chrono or one of its required modules")
  return()
endif()

if(APPLE)
    set(CMAKE_MACOSX_BUNDLE ON)
endif()

include_directories(${CHRONO_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if(MSVC AND ${MSVC_VERSION} GREATER_EQUAL 1915)
    add_definitions( "-D_ENABLE_EXTENDED_ALIGNED_STORAGE" )
  endif()
endif()

if(MSVC)
    add_definitions("-D_CRT_SECURE_NO_DEPRECATE")  # avoids deprecation warnings
    add_definitions("-D_SCL_SECURE_NO_DEPRECATE")  # avoids deprecation warnings
    add_definitions( "-DNOMINMAX" )                # do not use MSVC's min/max macros
    add_compile_options(/wd4275)                   # disable warnings triggered by Irrlicht
    add_compile_options(/wd4251)                   # class needs to have dll-interface
endif()


#########################
# Fetching fmu_generator
#########################
MESSAGE(STATUS "-------------- FetchContent(fmu_generator) --------------")

include(FetchContent)
FetchContent_Declare(
    PACK_FMU
    SOURCE_DIR ${FMU_GENERATOR_DIR}
)

set(FMU_MODEL_IDENTIFIER "myChronoFmuIdentifier" CACHE STRING "FMU Model Identifier: should fulfill operating system and C-function naming standards.")
set(BUILD_TESTING OFF)
set(USE_EXTERNAL_FMU_COMPONENT_SOURCES ON CACHE INTERNAL "")
set(ENABLE_GUID_CHECK ON)

FetchContent_MakeAvailable(PACK_FMU)

# MESSAGE(STATUS "FMI_PLATFORM: ${FMI_PLATFORM}")
# MESSAGE(STATUS "FMU_RUNTIME_OUTPUT_DIRECTORY: ${FMU_RUNTIME_OUTPUT_DIRECTORY}")

MARK_AS_ADVANCED(FETCHCONTENT_BASE_DIR)
MARK_AS_ADVANCED(FETCHCONTENT_FULLY_DISCONNECTED)
MARK_AS_ADVANCED(FETCHCONTENT_QUIET)
MARK_AS_ADVANCED(FETCHCONTENT_SOURCE_DIR_PACK_FMU)
MARK_AS_ADVANCED(FETCHCONTENT_UPDATES_DISCONNECTED)
MARK_AS_ADVANCED(FETCHCONTENT_UPDATES_DISCONNECTED_PACK_FMU)
MESSAGE(STATUS "---------------------------------------------------------")


# Add sources to imported target
target_sources(FmuComponent PRIVATE FmuComponentChrono.h FmuComponentChrono.cpp)
target_compile_definitions(FmuComponent PUBLIC "CHRONO_DATA_DIR=\"${CHRONO_DATA_DIR}\"") 
target_compile_options(FmuComponent PUBLIC ${CHRONO_CXX_FLAGS})
target_link_options(FmuComponent PUBLIC ${CHRONO_LINKER_FLAGS})
target_link_libraries(FmuComponent ${CHRONO_LIBRARIES})


# set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# add_DLL_copy_command("${CHRONO_DLLS}")

string(REPLACE ";" "\" \"" CHRONO_DLLS_SPACEDELIM "${CHRONO_DLLS}")
SET(CHRONO_DLLS_SPACEDELIM "\"${CHRONO_DLLS_SPACEDELIM}\"")
add_custom_target(COPY_DEPENDENCIES
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CHRONO_DLLS_SPACEDELIM}" "${FMU_RUNTIME_OUTPUT_DIRECTORY}"
  COMMENT "Creating FMU file: ${FMU_MODEL_IDENTIFIER}.fmu"
)

add_dependencies(fmu_modeldescription_generator COPY_DEPENDENCIES)
