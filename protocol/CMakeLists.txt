# Try adding as a git submodule first, as this this does not require internet connection at build time
if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/flatbuffers/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/flatbuffers"
            "${CMAKE_BINARY_DIR}/vendor/flatbuffers"
            EXCLUDE_FROM_ALL
    )
else()
    message(STATUS "vendor/flatbuffers not found - fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            flatbuffers
            GIT_REPOSITORY https://github.com/google/flatbuffers.git
            GIT_TAG v25.12.19
    )
    FetchContent_MakeAvailable(flatbuffers)
endif()

set(SCHEMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/schemas/")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/")

file(GLOB SCHEMA_FILES CONFIGURE_DEPENDS
    "${SCHEMA_DIR}/*.fbs"
)

set(GENERATED_HEADERS)

foreach(SCHEMA_FILE ${SCHEMA_FILES})
    get_filename_component(SCHEMA_NAME "${SCHEMA_FILE}" NAME_WE)

    set(GENERATED_HEADER
        "${GENERATED_DIR}/${SCHEMA_NAME}_generated.h"
    )

    add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
        COMMAND $<TARGET_FILE:flatc> -c -o "${GENERATED_DIR}" "${SCHEMA_FILE}"
        DEPENDS ${SCHEMA_FILE}
        COMMENT "Compiling FlatBuffers schema: ${SCHEMA_NAME}.fbs"
        VERBATIM
    )

    list(APPEND GENERATED_HEADERS "${GENERATED_HEADER}")
endforeach()

add_custom_target(imp_protocol_schemas DEPENDS ${GENERATED_HEADERS})

add_library(imp_protocol STATIC
        src/serialisation.cpp
        src/tcp_socket.cpp
        src/tcp_listener.cpp
        src/frame.cpp
        src/control.cpp
        src/tool_server.cpp
        src/entity_command_serialisation.cpp
        src/world_serialisation.cpp
        src/scene_command_serialisation.cpp
        src/script_status_serialisation.cpp
        src/asset_command_serialisation.cpp
        src/cvar_command_serialisation.cpp
)

add_dependencies(imp_protocol imp_protocol_schemas)

target_include_directories(imp_protocol
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${GENERATED_DIR}
        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(imp_protocol PUBLIC flatbuffers imp_core)

if (WIN32)
    target_link_libraries(imp_protocol PRIVATE Ws2_32)
endif()

imp_add_third_party_target(flatc)
imp_add_third_party_target(flatbuffers)
