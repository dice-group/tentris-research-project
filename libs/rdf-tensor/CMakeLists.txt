cmake_minimum_required(VERSION 3.21)
set(lib_suffix "rdf-tensor")
set(lib "${PROJECT_NAME}-${lib_suffix}")

# Find cmake packages
find_package(Boost REQUIRED)
find_package(hypertrie REQUIRED)
find_package(Metall REQUIRED)
find_package(rdf4cpp REQUIRED)
find_package(dice-hash REQUIRED)

# Define the library
add_library(${lib} INTERFACE)
add_library(${PROJECT_NAME}::${lib_suffix} ALIAS ${lib})


target_include_directories(${lib} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(${lib} INTERFACE
        rdf4cpp::rdf4cpp
        hypertrie::hypertrie
        hypertrie::query
        Metall::Metall
        Boost::headers
        dice-hash::dice-hash
)

# TODO: why is this here???
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/dice/tentris/tentris_version.hpp)
target_include_directories(${lib} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        )

include(${CMAKE_SOURCE_DIR}/cmake/install_components.cmake)
install_component(INTERFACE ${lib_suffix} src)