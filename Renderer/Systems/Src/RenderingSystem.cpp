#include <ApplicationHelper.h>
#include <ComponentManager.h>
#include <Renderer.h>
#include <RenderingSystem.h>
#include <span>
#include <Timer.h>
#include <TransformComponent.h>
#include <TransformUtils.h>

RenderingSystem::RenderingSystem(ComponentArray<TransformComponent>* transformComponents,
                                 ComponentArray<CameraComponent>* cameraComponents)
    :
    m_transformComponents(transformComponents), m_cameraComponents(cameraComponents)
{
}

MeshComponent RenderingSystem::CreateMeshComponent(MeshComponent::MeshBuildData* inMeshData, const TransformComponent& transform,
                                                   const Material& material,
                                                   uint32_t meshIdx, const std::string& name)
{
    RHI::ConstantBuffer constantBuffer = RHI::ConstantBufferUtils::CreateConstantBuffer(sizeof(TransformComponent::RenderData),
        std::string("ObjectCB_" + name).c_str(), true);
    // Fill object buffer with initial data (transform)
    RHI::ConstantBufferUtils::UpdateConstantBufferData(constantBuffer, (void*)&transform.m_renderData, sizeof(TransformComponent::RenderData));
    
    const Matrix& objectTransform = transform.m_renderData.m_objectToWorldMatrix;
    // Prepare 3x4 transform for BLAS
    DirectX::XMFLOAT3X4 BLASTransform(
        objectTransform._11, objectTransform._21, objectTransform._31, objectTransform._41,
        objectTransform._12, objectTransform._22, objectTransform._32, objectTransform._42,
        objectTransform._13, objectTransform._23, objectTransform._33, objectTransform._43
    );
    RHI::ConstantBuffer BLASTransformBuffer = RHI::ConstantBufferUtils::CreateConstantBuffer(sizeof(DirectX::XMFLOAT3X4),
        std::string("BLASTransform_" + name).c_str(), true,
        nullptr);//, (void*)&BLASTransform);
    RHI::ConstantBufferUtils::UpdateConstantBufferData(BLASTransformBuffer, (void*)reinterpret_cast<const DirectX::XMFLOAT3X4*>(&BLASTransform), sizeof(DirectX::XMFLOAT3X4));
    MeshComponent::MeshBuildData& meshData = *inMeshData;
    std::vector<MeshComponent::Vertex> vertexAttributes;
    vertexAttributes.reserve(meshData.m_nVertices);

    const Vector3* groupedPos = reinterpret_cast<const Vector3*>(meshData.m_vertices.data());
    const Vector2* groupedUvs = reinterpret_cast<const Vector2*>(meshData.m_uvs.data());
    const Vector3* groupedNormals = reinterpret_cast<const Vector3*>(meshData.m_normals.data());
    const Vector4* groupedColors = reinterpret_cast<const Vector4*>(meshData.m_colors.data());

    for (int i = 0; i < meshData.m_nVertices; i++)
    {
        vertexAttributes.push_back(
            {
                groupedPos[i], groupedUvs[i], groupedNormals[i],
                groupedColors ? groupedColors[i] : Vector4(0, 0, 0, 0)
            }
        );
    }

    RHI::VertexBuffer* vertexBuffer = RHI::VertexBuffer::CreateVertexBuffer(vertexAttributes.data(), sizeof(MeshComponent::Vertex),
                                                           sizeof(MeshComponent::Vertex) * meshData.m_nVertices, std::string("VtxBuffer_" + name).c_str());
    RHI::IndexBuffer* indexBuffer = RHI::IndexBuffer::CreateIndexBuffer(meshData.m_indices.data(),
                                                        sizeof(uint32_t) * meshData.m_nIndices, std::string("IdxBuffer_" + name).c_str());

    
    return std::move(MeshComponent(material, std::move(constantBuffer), std::move(BLASTransformBuffer), vertexBuffer, meshData.m_nVertices, indexBuffer, meshData.m_nIndices, name));
}

Entity RenderingSystem::GetMainCameraId() const
{
    SCOPED_TIMER("RenderingSystem::GetMainCameraId")
    return m_cameraComponents->GetEntityFromComponentIndex(0);
}

void RenderingSystem::UpdateCameraRenderData(Entity cameraId, uint32_t frameIdx) const
{
    SCOPED_TIMER("RenderingSystem::UpdateCameraRenderData")

    CameraComponent& cameraComponent = (*m_cameraComponents)[cameraId];
    if (!cameraComponent.m_dirtyRenderState[frameIdx])
    {
        return;
    }

    const TransformComponent& transformComponent = (*m_transformComponents)[cameraId];

    // Update camera matrices
    CameraComponent::CameraMatrices cameraMatrices;
    cameraMatrices.m_viewToWorld = transformComponent.m_renderData.m_objectToWorldMatrix;
    cameraMatrices.m_worldToView = TransformUtils::GetInverseTransform(cameraMatrices.m_viewToWorld);
    cameraMatrices.m_viewToClip = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        cameraComponent.m_cameraInternals.m_fov, cameraComponent.m_cameraInternals.m_aspectRatio,
        cameraComponent.m_cameraInternals.m_near,
        cameraComponent.m_cameraInternals.m_far);

    RHI::ConstantBufferUtils::UpdateConstantBufferData(cameraComponent.GetConstantBuffer(frameIdx),
                                                       (void*)&cameraMatrices,
                                                       sizeof(CameraComponent::CameraMatrices));
}

uint32_t RenderingSystem::GetCameraIndexInResourceDescriptorHeap(Entity cameraId, uint32_t frameIdx) const
{
    // TODO: This should go to Init Scene Pass Data probably
    SCOPED_TIMER("RenderingSystem::GetCameraResourceDescriptorHandle")

    CameraComponent& cameraComponent = (*m_cameraComponents)[cameraId];

    return cameraComponent.GetConstantBuffer(frameIdx).GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}


#define DEBUG_BLAS_BUILD 0
ComPtr<ID3D12Resource> RenderingSystem::BuildBottomLevelAccelerationStructure(std::span<MeshComponent> meshes)
{
#if DEBUG_BLAS_BUILD
    // Start capture
    PIXCaptureParameters captureParams = {};
    captureParams.TimingCaptureParameters.CaptureGpuTiming = true;// .Version = PIX_CAPTURE_PARAMETERS_VERSION;
    captureParams.GpuCaptureParameters.FileName = L"BLASCapture.wpix";

    // Start capture
    PIXBeginCapture(PIX_CAPTURE_GPU, &captureParams);
#endif
    const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();

    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
    geometryDescs.reserve(64);
    for (MeshComponent& mesh : meshes)
    {
        gRenderer->TransitionResources(commandList, {{&mesh.GetBLASTransformBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE}});
        
        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Triangles.VertexBuffer.StartAddress = mesh.GetVertexBuffer()->GetGpuAddress();
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshComponent::Vertex);
        geometryDesc.Triangles.VertexCount = mesh.GetVertexCount();
        geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        geometryDesc.Triangles.IndexBuffer = mesh.GetIndexBuffer()->GetGpuAddress();
        geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
        geometryDesc.Triangles.IndexCount = mesh.GetIndexCount();
        geometryDesc.Triangles.Transform3x4 = mesh.GetBLASTransformBuffer().GetGpuAddress();
        geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        geometryDescs.push_back(geometryDesc);
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
    asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    asInputs.NumDescs = geometryDescs.size();
    asInputs.pGeometryDescs = geometryDescs.data();
    asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    // Get the size requirements for the acceleration structure
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    gDevice->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &prebuildInfo);

    // Create the scratch buffer for the build process
    ComPtr<ID3D12Resource> scratchBuffer;
    CD3DX12_RESOURCE_DESC scratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        prebuildInfo.ScratchDataSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    gDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &scratchBufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&scratchBuffer));
    NAME_D3D12_OBJECT_CUSTOM(scratchBuffer, L"BLASScratchBuffer");

    // Create the acceleration structure buffer
    ComPtr<ID3D12Resource> BLAS;
    CD3DX12_RESOURCE_DESC asBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        prebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    gDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &asBufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&BLAS));
    NAME_D3D12_OBJECT_CUSTOM(scratchBuffer, L"BLASBuffer");

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = asInputs;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = BLAS->GetGPUVirtualAddress();

    // TODO: Use same command list for all initialization commands! Use gpu barriers instead if specific resources are needed
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    ThrowIfFailed(commandList->Close());
    gRenderer->ExecuteCommandListOnce();

#if DEBUG_BLAS_BUILD
    // End capture and save
    PIXEndCapture(false);
#endif

    // Deallocate scratch buffer (ExecuteCommandListOnce waits for gpu command to be finished, so there's no risk)
    scratchBuffer.Reset();

    return BLAS;
}

RHI::GPUResource* RenderingSystem::BuildTopLevelAccelerationStructure(ComPtr<ID3D12Resource> BLAS)
{
    // Describe the instance(s) for the TLAS
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.InstanceID = 0;
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
    instanceDesc.AccelerationStructure = BLAS->GetGPUVirtualAddress();
    instanceDesc.InstanceMask = 0x01; // Bit 1 for opaque geometry BLAS

    // Identity for now
    float transformMatrix[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f }
    };
    memcpy(instanceDesc.Transform, &transformMatrix, sizeof(instanceDesc.Transform));

    // Create a buffer for the instance data
    ComPtr<ID3D12Resource> instanceDescBuffer;
    // hardcoded for now - but if we have more instances it's better to arange them contiguously in gpu memory
    // here because tlasInputs.InstanceDescs only consumes one gpu address
    constexpr uint32_t numInstances = 1;
    UINT instanceDescBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numInstances;
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC instanceDescBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceDescBufferSize);
    gDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &instanceDescBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&instanceDescBuffer));
    NAME_D3D12_OBJECT_CUSTOM(instanceDescBuffer, L"TLASInstanceDescsBuffer");

    // Map the instance data buffer and copy the instance description
    void* mappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0); // We won't read from this resource on CPU
    instanceDescBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, &instanceDesc, instanceDescBufferSize);
    instanceDescBuffer->Unmap(0, nullptr);

    // Describe the TLAS inputs
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
    tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasInputs.NumDescs = numInstances;
    tlasInputs.InstanceDescs = instanceDescBuffer->GetGPUVirtualAddress();
    tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    // Get the size requirements for the TLAS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo = {};
    gDevice->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &tlasPrebuildInfo);

    // Create a scratch buffer for the TLAS build
    ComPtr<ID3D12Resource> scratchBuffer;
    CD3DX12_RESOURCE_DESC scratchBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        tlasPrebuildInfo.ScratchDataSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    gDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &scratchBufferDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&scratchBuffer));
    NAME_D3D12_OBJECT_CUSTOM(scratchBuffer, L"TLASScratchBuffer");

    // Create the TLAS buffer
    ComPtr<ID3D12Resource> TLAS;
    CD3DX12_RESOURCE_DESC tlasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        tlasPrebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    gDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &tlasBufferDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&TLAS));
    NAME_D3D12_OBJECT_CUSTOM(TLAS, L"TLAS");

    // Describe the TLAS build
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasBuildDesc = {};
    tlasBuildDesc.Inputs = tlasInputs;
    tlasBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    tlasBuildDesc.DestAccelerationStructureData = TLAS->GetGPUVirtualAddress();

    // TODO: Use same command list for all initialization commands! Use gpu barriers instead if specific resources are needed
    const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCurrentCommandListReset();
    commandList->BuildRaytracingAccelerationStructure(&tlasBuildDesc, 0, nullptr);
    ThrowIfFailed(commandList->Close());
    gRenderer->ExecuteCommandListOnce();

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.RaytracingAccelerationStructure.Location = TLAS->GetGPUVirtualAddress();
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    
    RHI::DescriptorHeapHandles descriptorHeapHandles;
    for (int i = 0; i < gFrameCount; i++)
    {
        RHI::ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);
        descriptorHeapHandles.handles[i][D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = resourceDescriptorHeap->GetHeapLocationNewHandle(RHI::HeapLocation::TLAS);
        gDevice->CreateShaderResourceView(nullptr, &SRVDesc, descriptorHeapHandles.handles[i][D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetCPUHandle());
    }
    
    // Deallocate scratch buffer (ExecuteCommandListOnce waits for gpu command to be finished, so there's no risk)
    scratchBuffer.Reset();
    instanceDescBuffer.Reset();

    return new RHI::GPUResource(TLAS, descriptorHeapHandles, D3D12_RESOURCE_STATE_GENERIC_READ, "TLAS");
}

RHI::ConstantBuffer RenderingSystem::CreateLightsBuffer(ComponentArray<PointLightComponent>* pointLights)
{
    std::vector<PointLightComponent::RenderData> lights;
    lights.reserve(8);
    for (PointLightComponent& pointLight : pointLights->GetSpan())
    {
        lights.push_back(pointLight.m_renderData);
    }
    RHI::ConstantBuffer lightsBuffer = RHI::ConstantBufferUtils::CreateStructuredBuffer(lights.size(),
        sizeof(PointLightComponent::RenderData), "LightsBuffer", lights.data());

    return std::move(lightsBuffer);
}
