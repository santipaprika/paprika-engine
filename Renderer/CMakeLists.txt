# Note that headers are optional, and do not affect add_library, but they will not
# show up in IDEs unless they are listed in add_library.

# Optionally glob, but only for CMake 3.12 or later:
# file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")
set(SRC_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Renderer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/ShaderStructures.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/DxgiHelper.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/Pass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/Pass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/DepthPass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/DepthPass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/CustomClearBuffersPass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/CustomClearBuffersPass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/ShadowVariancePass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/ShadowVariancePass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/BasePass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/BasePass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/Passes/DenoisePPFXPass.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/Passes/DenoisePPFXPass.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/stdafx_renderer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/stdafx_renderer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/PassManager.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/PassManager.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/Src/MeshComponent.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/MeshComponent.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/Src/CameraComponent.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/CameraComponent.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/Src/PointLightComponent.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/PointLightComponent.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Components/Src/Material.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Components/Include/Material.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Systems/Src/RenderingSystem.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Systems/Include/RenderingSystem.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Systems/Src/ControllerSystem.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Systems/Include/ControllerSystem.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Include/BackendUtils.h"
)

set(RHI_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/Buffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/Buffer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/CommandContext.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/CommandContext.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/ConstantBuffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/ConstantBuffer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/PersistentUploadBuffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/PersistentUploadBuffer.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/DescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/DescriptorHeapElement.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeapElement.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/DescriptorHeapHandle.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeapHandle.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/DescriptorHeapManager.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/DescriptorHeapManager.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/GPUResource.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/GPUResource.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/Sampler.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/Sampler.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/ShaderDescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/ShaderDescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/StagingDescriptorHeap.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/StagingDescriptorHeap.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/Texture.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/Texture.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Src/RHI/VertexBuffer.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Include/RHI/VertexBuffer.h"
)
set(ASSET_FILES
        "${PROJECT_SOURCE_DIR}/Assets/Shaders/DepthVS.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/BasePassVS.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/BasePassPS.hlsl"
        "${PROJECT_SOURCE_DIR}/Assets/Shaders/DenoiseVS.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/DenoisePS.hlsl"
		"${PROJECT_SOURCE_DIR}/Assets/Shaders/ShadowVarianceVS.hlsl"
        "${PROJECT_SOURCE_DIR}/Assets/Shaders/ShadowVariancePS.hlsl"
        "${PROJECT_SOURCE_DIR}/Assets/Shaders/ShadowVarianceCS.hlsl"
        "${PROJECT_SOURCE_DIR}/Assets/Shaders/CustomClearBuffersCS.hlsl"
)

# Make an automatic library - will be static or dynamic based on user setting
add_library(Renderer STATIC ${SRC_FILES} ${RHI_FILES} ${ASSET_FILES})

# Set Pix path
SET(PIX_PATH "${CMAKE_SOURCE_DIR}/Externals/WinPixEventRuntime")
SET(GLTF_PATH "${CMAKE_SOURCE_DIR}/Externals/glTF-SDK/GLTFSDK")


# include and link DXC
set(DXC_ROOT "${CMAKE_SOURCE_DIR}/Externals/DirectXShaderCompiler")
#link_directories(PUBLIC "${DXC_ROOT}/lib/x64")
set(DXC_LIBRARIES "${DXC_ROOT}/lib/x64/dxil.lib" "${DXC_ROOT}/lib/x64/dxcompiler.lib")

# We need this directory, and users of our library will need it too
target_include_directories(Renderer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include"  "${PIX_PATH}/Include" "${GLTF_PATH}/Inc"
                            "${CMAKE_CURRENT_SOURCE_DIR}/Components/Include" "${CMAKE_CURRENT_SOURCE_DIR}/Systems/Include"
                            "${DXC_ROOT}/inc")
# TODO: organize a bit this, or at least make it more consistent
target_link_libraries(Renderer PUBLIC Core.Static "${PIX_PATH}/WinPixEventRuntime.lib" GLTFSDK dxguid.lib d3d12.lib dxgi.lib d3dcompiler.lib "${DXC_ROOT}/lib/x64/dxcompiler.lib")

# Avoid VS trying to compile the shaders when building the project
set_source_files_properties(${ASSET_FILES} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")
