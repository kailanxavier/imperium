function(imp_add_third_party_target TARGET)

    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR
                "Third party target '${TARGET}' does not exist"
        )
    endif()

    set(THIRD_PARTY_BIN_DIR
            "${IMP_OUTPUT_DIR}/third_party/bin"
    )

    set(THIRD_PARTY_LIB_DIR
            "${IMP_OUTPUT_DIR}/third_party/lib"
    )


    set_target_properties(${TARGET}
            PROPERTIES

            RUNTIME_OUTPUT_DIRECTORY
            "${THIRD_PARTY_BIN_DIR}"

            LIBRARY_OUTPUT_DIRECTORY
            "${THIRD_PARTY_LIB_DIR}"

            ARCHIVE_OUTPUT_DIRECTORY
            "${THIRD_PARTY_LIB_DIR}"
    )


    foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)

        string(TOUPPER ${CONFIG} CONFIG_UPPER)

        set_target_properties(${TARGET}
                PROPERTIES

                RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}
                "${THIRD_PARTY_BIN_DIR}"

                LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER}
                "${THIRD_PARTY_LIB_DIR}"

                ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}
                "${THIRD_PARTY_LIB_DIR}"
        )

    endforeach()

endfunction()
