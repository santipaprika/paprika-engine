# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Renderer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ShaderStructures.h"
)

set(ASSET_FILES
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SamplePixelShader.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SampleVertexShader.hlsl"
)

# Make an automatic library - will be static or dynamic based on user setting
add_library(Renderer STATIC ${SRC_FILES} ${ASSET_FILES})

# We need this directory, and users of our library will need it too
target_include_directories(Renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_libraries(Renderer PUBLIC Core)

# IDEs should put the headers in a nice place
#source_group(
#  TREE "${PROJECT_SOURCE_DIR}/include"
#  PREFIX "Header Files"
#  FILES ${HEADER_LIST})
