# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Renderer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ShaderStructures.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/Pass.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/Pass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx_renderer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx_renderer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/BackendUtils.h"
)

set(ASSET_FILES
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SamplePixelShader.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SampleVertexShader.hlsl"
)

# Make an automatic library - will be static or dynamic based on user setting
add_library(Renderer STATIC ${SRC_FILES} ${ASSET_FILES})

# Set Pix path
SET(PIX_PATH "${CMAKE_SOURCE_DIR}/Externals/WinPixEventRuntime")
SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")

# We need this directory, and users of our library will need it too
target_include_directories(Renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"  "${PIX_PATH}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Renderer PUBLIC Core.Static "${PIX_PATH}/WinPixEventRuntime.lib" GLTFSDK d3d12.lib dxgi.lib d3dcompiler.lib)

# Avoid VS trying to compile the shaders when building the project
set_source_files_properties(${ASSET_FILES} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")
