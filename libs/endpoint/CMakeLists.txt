cmake_minimum_required(VERSION 3.21)
set(lib_suffix "endpoint")
set(lib "${PROJECT_NAME}-${lib_suffix}")

find_package(Taskflow REQUIRED)
find_package(restinio REQUIRED)
find_package(spdlog REQUIRED)
find_package(cppitertools REQUIRED)
find_package(RapidJSON REQUIRED)

add_library(${lib}
        src/dice/endpoint/HTTPServer.cpp
        src/dice/endpoint/SparqlEndpoint.cpp
        src/dice/endpoint/CountEndpoint.cpp
        src/dice/endpoint/SparqlStreamingEndpoint.cpp
        src/dice/endpoint/SparqlUpdateEndpoint.cpp
        src/dice/endpoint/SparqlQueryCache.cpp
        src/dice/endpoint/Endpoint.cpp
        )
add_library(${PROJECT_NAME}::${lib_suffix} ALIAS ${lib})

target_include_directories(${lib} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/private-include
)

target_link_libraries(${lib} PUBLIC
        ${PROJECT_NAME}::triple-store
        ${PROJECT_NAME}::node-store
        restinio::restinio
        Taskflow::Taskflow
        PRIVATE
        spdlog::spdlog
        cppitertools::cppitertools
        rapidjson
)

include(${CMAKE_SOURCE_DIR}/cmake/install_components.cmake)
install_component(PUBLIC ${lib_suffix} src)