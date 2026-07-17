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

find_program(FLATC_EXECUTABLE flatc)

set(SCHEMA_FILE "${CMAKE_CURRENT_SOURCE_DIR}/schemas/memory.fbs")
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/memory_generated.h")

add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND flatc -c -o "${CMAKE_CURRENT_BINARY_DIR}/generated/" ${SCHEMA_FILE}
        DEPENDS ${SCHEMA_FILE}
        COMMENT "Compiling FlatBuffers schema: memory.fbs"
)

add_custom_target(imp_protocol_schemas DEPENDS ${GENERATED_HEADER})

add_library(imp_protocol STATIC
        src/connection.cpp
        src/serialisation.cpp
        src/tcp_socket.cpp
        src/tcp_listener.cpp
        src/frame.cpp
        src/control.cpp
        src/tool_server.cpp
)

add_dependencies(imp_protocol imp_protocol_schemas)

target_include_directories(imp_protocol
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_BINARY_DIR}/generated
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
