add_library(imp_protocol STATIC
        src/connection.cpp
        src/serialisation.cpp
)

target_include_directories(imp_protocol
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
