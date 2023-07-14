
cmake_minimum_required(VERSION 3.15)


project(fmu_generator_standalone)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(fmu_entity SHARED fmu_entity.cpp)
target_include_directories(fmu_entity PUBLIC "fmi2_headers")
target_include_directories(fmu_entity PUBLIC "rapidxml")



# set_property(TARGET fmu_entity PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

SET(FMU_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/binaries/win64/$<0:>")
set_target_properties(fmu_entity PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_entity PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_entity PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_entity PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_entity PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${FMU_RUNTIME_OUTPUT_DIRECTORY})
set_target_properties(fmu_entity PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${FMU_RUNTIME_OUTPUT_DIRECTORY})

# Create the ZIP command
# file(GLOB_RECURSE zipFiles_1 "${FMU_RUNTIME_OUTPUT_DIRECTORY}/*")
# file(GLOB_RECURSE zipFiles_2 "${CMAKE_SOURCE_DIR}/modelDescription.xml")
# SET(zipFiles ${zipFiles_1} ${zipFiles_1})
add_custom_command(
    TARGET fmu_entity
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/modelDescription.xml" "${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "fmu_entity.fmu" "modelDescription.xml" "${FMU_RUNTIME_OUTPUT_DIRECTORY}" "--format=zip"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating ZIP file: fmu_entity.fmu"
)



add_executable(fmu_host_standalone fmu_host_standalone.cpp FmuToolsStandalone.hpp)
target_include_directories(fmu_host_standalone PUBLIC "fmi2_headers")
target_include_directories(fmu_host_standalone PUBLIC "rapidxml")
set_property(TARGET fmu_host_standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
add_dependencies(fmu_host_standalone fmu_entity)


add_executable(nonfmu_host nonfmu_host.cpp FmuToolsStandalone.hpp)
target_include_directories(nonfmu_host PUBLIC "fmi2_headers")
target_link_libraries(nonfmu_host PUBLIC fmu_entity)
# target_include_directories(nonfmu_host PUBLIC "rapidxml")

# Set the working directory for MSVC
set_property(TARGET nonfmu_host PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${FMU_RUNTIME_OUTPUT_DIRECTORY})



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