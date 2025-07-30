cmake_minimum_required(VERSION 3.21)
set(lib_suffix "sparql2tensor")
set(lib "${PROJECT_NAME}-${lib_suffix}")

# Find cmake packages
find_package(sparql-parser-base REQUIRED)
find_package(robin_hood REQUIRED)
find_package(dice-hash REQUIRED)

# Define the library
add_library(${lib}
        src/dice/sparql2tensor/parser/exception/SPARQLErrorListener.cpp
        src/dice/sparql2tensor/parser/visitors/PrologueVisitor.cpp
        src/dice/sparql2tensor/parser/visitors/SelectAskQueryVisitor.cpp
        src/dice/sparql2tensor/SPARQLQuery.cpp
        src/dice/sparql2tensor/UPDATEQuery.cpp
        )
add_library(${PROJECT_NAME}::${lib_suffix} ALIAS ${lib})

target_include_directories(${lib} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private-include
)

target_link_libraries(${lib} PUBLIC
        ${PROJECT_NAME}::rdf-tensor
        robin_hood::robin_hood
        PRIVATE
        sparql-parser-base::sparql-parser-base
)

include(${CMAKE_SOURCE_DIR}/cmake/install_components.cmake)
install_component(PUBLIC ${lib_suffix} src)