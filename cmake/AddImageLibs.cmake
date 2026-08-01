include (FetchContent)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|x86_64|AMD64|i.86)$" OR CMAKE_GENERATOR_PLATFORM MATCHES "x64|Win32")
    find_program(IMP_NASM_EXECUTABLE nasm)
    if (NOT IMP_NASM_EXECUTABLE)
        # we want to fail loudly here if nasm isn't found because libjpeg will build with
        # no SIMD acceleration, defeating the point of even having it in the first place
        message(WARNING
            "NASM not found on PATH. libjpeg-turbo will build WITHOUT SIMD acceleration"
        )
    else()
        message(STATUS "Found NASM: ${IMP_NASM_EXECUTABLE}")
    endif()
endif()

find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)

if (NOT TARGET libjpeg-turbo::turbojpeg)
    message(FATAL_ERROR
        "Expected target 'libjpeg-turbo::turbojpeg' not found after find_package(libjpeg-turbo). "
        "Check that CMAKE_TOOLCHAIN_FILE points at vcpkg.cmake and that vcpkg.json is present at "
        "${CMAKE_SOURCE_DIR}."
    )
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/libdeflate/CMakeLists.txt")
    set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/vendor/libdeflate"
        "${CMAKE_BINARY_DIR}/vendor/libdeflate"
        EXCLUDE_FROM_ALL
    )
    set(_imp_libdeflate_src_dir "${CMAKE_SOURCE_DIR}/vendor/libdeflate")
else()
    message(STATUS "vendor/libdeflate not found - fetching via FetchContent")
    FetchContent_Declare(
        libdeflate
        GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
        GIT_TAG v1.20
    )
    set(LIBDEFLATE_BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_GZIP OFF CACHE BOOL "" FORCE)
    set(LIBDEFLATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(libdeflate)
    set(_imp_libdeflate_src_dir "${libdeflate_SOURCE_DIR}")
endif()
 
if (NOT TARGET libdeflate_static)
    get_directory_property(_imp_libdeflate_targets DIRECTORY "${_imp_libdeflate_src_dir}" BUILDSYSTEM_TARGETS)
    message(FATAL_ERROR
        "Expected target 'libdeflate_static' not found after fetching libdeflate. "
        "Actual targets found in ${_imp_libdeflate_src_dir}: ${_imp_libdeflate_targets}. "
        "Update the target name in AddImageLibs.cmake and the link line in "
        "imp/gfx/CMakeLists.txt to match."
    )
endif()

option(IMP_USE_VENDORED_ZLIB "Vendor zlib-ng via FetchContent instead of using system zlib" ON)
 
if (IMP_USE_VENDORED_ZLIB)
    if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/zlib-ng/CMakeLists.txt")
        set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
        set(ZLIB_ALIASES ON CACHE BOOL "" FORCE)
        set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(WITH_GTEST OFF CACHE BOOL "" FORCE)
        set(WITH_GZFILEOP OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        add_subdirectory(
            "${CMAKE_SOURCE_DIR}/vendor/zlib-ng"
            "${CMAKE_BINARY_DIR}/vendor/zlib-ng"
            EXCLUDE_FROM_ALL
        )
    else()
        message(STATUS "vendor/zlib-ng not found - fetching via FetchContent")
        FetchContent_Declare(
            zlib-ng
            GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng.git
            GIT_TAG 2.2.2
        )
        set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
        set(ZLIB_ALIASES ON CACHE BOOL "" FORCE)
        set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
        set(WITH_GTEST OFF CACHE BOOL "" FORCE)
        set(WITH_GZFILEOP OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(zlib-ng)
    endif()
 
    if (NOT TARGET zlib)
        message(FATAL_ERROR
            "Expected target 'zlibstatic' not found after fetching zlib-ng. "
            "Check ${CMAKE_BINARY_DIR}/vendor/zlib-ng for the actual target name, "
            "or set IMP_USE_VENDORED_ZLIB=OFF to use system zlib instead."
        )
    endif()
 
    if (NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlib)
    endif()
 
    get_target_property(_imp_zlib_ng_src_dir zlibstatic SOURCE_DIR)
    set(ZLIB_INCLUDE_DIR "${_imp_zlib_ng_src_dir}" CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY "zlibstatic" CACHE STRING "" FORCE)
    set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
 
    message(STATUS
        "zlib-ng bridged as ZLIB::ZLIB for libspng. If libspng's configure step "
        "still fails to find ZLIB, this bridge needs adjusting for your CMake "
        "version. The safe fallback is -DIMP_USE_VENDORED_ZLIB=OFF to use "
        "system zlib via find_package(ZLIB REQUIRED) instead."
    )
else()
    find_package(ZLIB REQUIRED)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/libspng/CMakeLists.txt")
    set(SPNG_SHARED OFF CACHE BOOL "" FORCE)
    set(SPNG_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${CMAKE_SOURCE_DIR}/vendor/libspng"
        "${CMAKE_BINARY_DIR}/vendor/libspng"
        EXCLUDE_FROM_ALL
    )
else()
    message(STATUS "vendor/libspng not found - fetching via FetchContent")
    FetchContent_Declare(
        libspng
        GIT_REPOSITORY https://github.com/randy408/libspng.git
        GIT_TAG v0.7.4
    )
    set(SPNG_SHARED OFF CACHE BOOL "" FORCE)
    set(SPNG_STATIC ON CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(libspng)
endif()

if (NOT TARGET spng_static)
    message(FATAL_ERROR
        "Expected target 'spng_static' not found after fetching libspng. "
        "This is the step most likely to break - check "
        "${CMAKE_BINARY_DIR}/vendor/libspng's CMake output above for the actual "
        "find_package(ZLIB) result and target name."
    )
endif()



macro(imp_try_add_third_party_target TARGET_NAME)
    if (TARGET ${TARGET_NAME})
        imp_add_third_party_target(${TARGET_NAME})
    else()
        message(STATUS "Skipping imp_add_third_party_target: target '${TARGET_NAME}' not found")
    endif()
endmacro()

imp_try_add_third_party_target(spng_static)
imp_try_add_third_party_target(deflatestatic)
if (IMP_USE_VENDORED_ZLIB)
    imp_try_add_third_party_target(zlib-ng-static)
    imp_try_add_third_party_target(zlib)
endif()
