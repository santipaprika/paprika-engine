cmake_minimum_required(VERSION 3.15)

project(
    PaprikaEngine
    VERSION 0.0.1
    DESCRIPTION "Toy Renderer by @santipaprika"
    LANGUAGES CXX
)

add_definitions(-DUNICODE)
set(CMAKE_CXX_STANDARD 17)

# Externals

# The compiled libraries code is here
add_subdirectory(Renderer)
add_subdirectory(Core)

# The executable code is here
add_subdirectory(Launcher)