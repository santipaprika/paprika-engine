cmake_minimum_required(VERSION 3.15)

project(
    PaprikaEngine
    VERSION 0.0.1
    DESCRIPTION "Toy Renderer by @santipaprika"
    LANGUAGES CXX
)

add_definitions(-DUNICODE)
set(CMAKE_CXX_STANDARD 17)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add assets directory as a definition to use in the C++ code
add_definitions( -DASSETS_PATH="${PROJECT_SOURCE_DIR}/Assets" )

# Externals
add_subdirectory(Externals)

# The compiled libraries code is here
add_subdirectory(Renderer)
add_subdirectory(Scene)
add_subdirectory(Core)

# The executable code is here
add_subdirectory(Launcher)