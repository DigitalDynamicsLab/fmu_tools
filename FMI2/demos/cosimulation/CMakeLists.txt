set(COMPONENT_NAME "myFmuCosimulation")
set(COMPONENT_MAIN_DIR "${CMAKE_BINARY_DIR}")
set(COMPONENT_SOURCES myFmuCosimulation.h myFmuCosimulation.cpp)
set(COMPONENT_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/my_resources)

set(FMU_TOOLS_DIR ${CMAKE_SOURCE_DIR}/FMI2)

#--------------------------------------------------------------

# Set include paths, compiler & linker flags, and libraries
set(COMPILER_FLAGS "")
set(LINKER_FLAGS "")
list(APPEND LIBS "")

#==============================================================

include(FetchContent)

#==============================================================

message(STATUS "...add ${COMPONENT_NAME}")
message(STATUS "   FetchContent(fmu_tools) from ${FMU_TOOLS_DIR}")

set(FMU_MODEL_IDENTIFIER "${COMPONENT_NAME}" CACHE INTERNAL "")
set(FMU_RESOURCES_DIRECTORY ${COMPONENT_RESOURCES_DIR} CACHE INTERNAL "")
set(USE_CUSTOM_TYPESPLATFORM OFF CACHE INTERNAL "")

set(FMU_MAIN_DIRECTORY ${COMPONENT_MAIN_DIR})
set(FMU_MSG_PREFIX "   - ")

FetchContent_Declare(
    ${FMU_MODEL_IDENTIFIER}
    SOURCE_DIR ${FMU_TOOLS_DIR}
)

FetchContent_MakeAvailable(${FMU_MODEL_IDENTIFIER})

string(TOUPPER ${FMU_MODEL_IDENTIFIER} FMU_MODEL_IDENTIFIER_UPPERCASE)
MARK_AS_ADVANCED(FETCHCONTENT_BASE_DIR)
MARK_AS_ADVANCED(FETCHCONTENT_FULLY_DISCONNECTED)
MARK_AS_ADVANCED(FETCHCONTENT_QUIET)
MARK_AS_ADVANCED(FETCHCONTENT_SOURCE_DIR_${FMU_MODEL_IDENTIFIER_UPPERCASE})
MARK_AS_ADVANCED(FETCHCONTENT_UPDATES_DISCONNECTED)
MARK_AS_ADVANCED(FETCHCONTENT_UPDATES_DISCONNECTED_${FMU_MODEL_IDENTIFIER_UPPERCASE})

#--------------------------------------------------------------
# add to the FMU creation target

target_sources(${FMU_MODEL_IDENTIFIER} PRIVATE ${COMPONENT_SOURCES})

set_target_properties(${FMU_MODEL_IDENTIFIER} PROPERTIES FOLDER demos COMPILE_FLAGS "${COMPILER_FLAGS}" LINK_FLAGS "${LINKER_FLAGS}")
target_link_libraries(${FMU_MODEL_IDENTIFIER} ${LIBS})  

source_group("" FILES ${COMPONENT_SOURCES})

