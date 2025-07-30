cmake_minimum_required(VERSION 3.24)
project(tentris VERSION 1.5.0
        DESCRIPTION "Tentris - A tensor-based Triplestore.")

include(cmake/boilerplate_init.cmake)
boilerplate_init()

if (PROJECT_IS_TOP_LEVEL)
    set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=boost/*:header_only=True")

    if (BUILD_TESTING)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_test_deps=True")
    endif ()
endif ()

set(style_files
        .clang-format
        .clang-tidy
)
foreach(style_file ${style_files})
    file(DOWNLOAD "https://raw.githubusercontent.com/dice-group/tentris-cpp-coding-guidelines/main/${style_file}"
            "${CMAKE_SOURCE_DIR}/${style_file}"
            TLS_VERIFY ON)
endforeach()

if (PROJECT_IS_TOP_LEVEL AND USE_CLANG_TIDY)
    include(cmake/ClangTidy.cmake)
endif ()

add_compile_definitions(Dnsel_CONFIG_SELECT_EXPECTED=nsel_EXPECTED_NONSTD)

add_subdirectory(libs)
add_subdirectory(execs)
