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
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Mesh.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Mesh.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Camera.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Camera.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/PassManager.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/PassManager.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/BackendUtils.h"
)

set(RHI_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/Buffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/Buffer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/CommandContext.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/CommandContext.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/ConstantBuffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/ConstantBuffer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/DescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeapHandle.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/GPUResource.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/GPUResource.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/ShaderDescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/ShaderDescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/StagingDescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/StagingDescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/VertexBuffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/VertexBuffer.h"
)
set(ASSET_FILES
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SamplePixelShader.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/SampleVertexShader.hlsl"
)

# Make an automatic library - will be static or dynamic based on user setting
add_library(Renderer STATIC ${SRC_FILES} ${RHI_FILES} ${ASSET_FILES})

# Set Pix path
SET(PIX_PATH "${CMAKE_SOURCE_DIR}/Externals/WinPixEventRuntime")
SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")

# We need this directory, and users of our library will need it too
target_include_directories(Renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"  "${PIX_PATH}/Include" "${GLTF_PATH}/Inc")
target_link_libraries(Renderer PUBLIC Core.Static "${PIX_PATH}/WinPixEventRuntime.lib" GLTFSDK dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib)

# Avoid VS trying to compile the shaders when building the project
set_source_files_properties(${ASSET_FILES} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")
