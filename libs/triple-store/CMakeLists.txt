cmake_minimum_required(VERSION 3.21)
set(lib_suffix "triple-store")
set(lib "${PROJECT_NAME}-${lib_suffix}")

# Find cmake packages
find_package(dice-hash REQUIRED)

# Define the library
add_library(${lib}
        src/dice/triple-store/TripleStore.cpp
        )
add_library(${PROJECT_NAME}::${lib_suffix} ALIAS ${lib})

target_include_directories(${lib} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(${lib} PUBLIC
        ${PROJECT_NAME}::sparql2tensor
        ${PROJECT_NAME}::rdf-tensor
)

include(${CMAKE_SOURCE_DIR}/cmake/install_components.cmake)
install_component(PUBLIC ${lib_suffix} src)
