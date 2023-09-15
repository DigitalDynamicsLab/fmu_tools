
cmake_minimum_required(VERSION 3.10)


project(fmu_generator)


LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/../chrono/lib")
find_package(Chrono
             OPTIONAL_COMPONENTS Irrlicht
             CONFIG)

if (Chrono_FOUND)
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


add_executable(fmu_host SHARED fmu_host.cpp FmuTools.hpp)
target_compile_definitions(fmu_host PUBLIC "CHRONO_DATA_DIR=\"${CHRONO_DATA_DIR}\"") 
target_compile_options(fmu_host PUBLIC ${CHRONO_CXX_FLAGS})
target_link_options(fmu_host PUBLIC ${CHRONO_LINKER_FLAGS})
target_link_libraries(fmu_host ${CHRONO_LIBRARIES})

add_library(FmuComponentBase SHARED FmuToolsExport.cpp)
target_include_directories(FmuComponentBase PUBLIC "fmi2_headers")
target_compile_definitions(FmuComponentBase PUBLIC "CHRONO_DATA_DIR=\"${CHRONO_DATA_DIR}\"") 
target_compile_options(FmuComponentBase PUBLIC ${CHRONO_CXX_FLAGS})
target_link_options(FmuComponentBase PUBLIC ${CHRONO_LINKER_FLAGS})
target_link_libraries(FmuComponentBase ${CHRONO_LIBRARIES})






set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_DLL_copy_command("${CHRONO_DLLS}")
