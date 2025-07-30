cmake_minimum_required(VERSION 3.21)
set(lib_suffix "node-store")
set(lib "${PROJECT_NAME}-${lib_suffix}")

# Find cmake packages
find_package(hypertrie REQUIRED)
find_package(robin_hood REQUIRED)
find_package(dice-hash REQUIRED)
find_package(Boost REQUIRED)
find_package(rdf4cpp REQUIRED)

# Define the library
add_library(${lib}
        src/dice/node-store/PersistentNodeStorageBackendImpl.cpp
        src/dice/node-store/PersistentNodeStorageBackend.cpp
        src/dice/node-store/MetallBNodeBackend.cpp
        src/dice/node-store/MetallIRIBackend.cpp
        src/dice/node-store/MetallLiteralBackend.cpp
        src/dice/node-store/MetallVariableBackend.cpp
        )
add_library(${PROJECT_NAME}::${lib_suffix} ALIAS ${lib})

target_include_directories(${lib} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(${lib} PUBLIC
        ${PROJECT_NAME}::rdf-tensor
        rdf4cpp::rdf4cpp
)

include(${CMAKE_SOURCE_DIR}/cmake/install_components.cmake)
install_component(PUBLIC ${lib_suffix} src)