#include <Renderer.h>
#include <ApplicationHelper.h>
#include <RHI/PersistentUploadBuffer.h>

// Currently 128MB of persistent upload buffer always mapped
constexpr size_t g_persistentBufferSize = 1ull << 27;

PersistentUploadBuffer::PersistentUploadBuffer(uint32_t frameIdx)
{
    // Create use a persistent upload buffer that will be used for updates
    static_assert(g_persistentBufferSize % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(g_persistentBufferSize);
    CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(gDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE, // should be read?
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    m_firstFreeIndex = 0;

    // GPUResource members
    m_name = "UploadBuffer_Persistent_" + std::to_string(frameIdx);
	NAME_D3D12_OBJECT_CUSTOM(m_resource, m_name.c_str());

    m_sizeInBytes = g_persistentBufferSize;
    m_usageState = D3D12_RESOURCE_STATE_COPY_SOURCE;
    SetupResourceStats();

    m_isReady = true;
}

void PersistentUploadBuffer::ResetIndex()
{
    m_firstFreeIndex = 0;
}

uint32_t PersistentUploadBuffer::SetData(ResourceUpdateArgs& updateArgs)
{
    const uint32_t bufferSize = updateArgs.m_memorySize;
    GPUResource* destResource = updateArgs.m_destResource;

    {
        std::lock_guard lock(m_updateResourceMutex);
        // We don't support copying more than 2MB at a time
        if (m_firstFreeIndex + bufferSize >= g_persistentBufferSize)
        {
            gRenderer->GetCommandContext()->GetCurrentCommandList()->Close();
            gRenderer->ExecuteCommandListOnce(true);
        }
        PPK::Logger::Assert(m_firstFreeIndex + bufferSize < g_persistentBufferSize);

        // Update current index
        uint32_t bufferOffset = m_firstFreeIndex;
        m_firstFreeIndex += bufferSize;

        const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCommandContext()->GetCurrentCommandList();
        D3D12_RESOURCE_STATES originalState = destResource->GetUsageState();
        gRenderer->TransitionResources(commandList, {
            { destResource, D3D12_RESOURCE_STATE_COPY_DEST }
        });

        for (int i = 0; i < updateArgs.m_numSubresources; i++)
        {
            updateArgs.m_layouts[i].Offset += bufferOffset;
        }
        UINT64 size = UpdateSubresources(commandList.Get(), destResource->GetResource().Get(), m_resource.Get(), 0, updateArgs.m_numSubresources,
            updateArgs.m_memorySize, updateArgs.m_layouts, updateArgs.m_numRows, updateArgs.m_rowSizesInBytes, updateArgs.m_srcData.data());
        PPK::Logger::Assert(size > 0, ("Unable to allocate subresources for " + destResource->GetName()).c_str());

        // Set data is in charge of assigning the resource to its original state.
        // Having different usage states after updating is rare, so worth doing here for now
        gRenderer->TransitionResources(commandList, {
            { destResource, originalState }
        });
        
        return bufferOffset;
    }
}

uint32_t PersistentUploadBuffer::SetData(const D3D12_SUBRESOURCE_DATA& subresourceData, GPUResource* destResource)
{
    // this path doesn't support multiple subresources, use function above instead
    PPK::Logger::Assert(destResource->GetResource()->GetDesc().DepthOrArraySize == 1 && destResource->GetResource()->GetDesc().MipLevels == 1);

    // Buffer size is equal to slicePitch if we update just 1 subresource
    const uint32_t bufferSize = subresourceData.SlicePitch;

    {
        std::lock_guard lock(m_updateResourceMutex);
        // We don't support copying more than 2MB at a time
        if (m_firstFreeIndex + bufferSize >= g_persistentBufferSize)
        {
            gRenderer->GetCommandContext()->GetCurrentCommandList()->Close();
            gRenderer->ExecuteCommandListOnce(true);
        }
        PPK::Logger::Assert(m_firstFreeIndex + bufferSize < g_persistentBufferSize);

        // Update current index
        uint32_t bufferOffset = m_firstFreeIndex;
        m_firstFreeIndex += bufferSize;

        const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCommandContext()->GetCurrentCommandList();
        D3D12_RESOURCE_STATES originalState = destResource->GetUsageState();
        gRenderer->TransitionResources(commandList, {
            { destResource, D3D12_RESOURCE_STATE_COPY_DEST }
        });

        // This performs the memcpy through intermediate buffer
        UpdateSubresources<1>(commandList.Get(), destResource->GetResource().Get(), m_resource.Get(), bufferOffset, 0, 1,
                              &subresourceData);

        // Set data is in charge of assigning the resource to its original state.
        // Having different usage states after updating is rare, so worth doing here for now
        gRenderer->TransitionResources(commandList, {
            { destResource, originalState }
        });
        
        return bufferOffset;
    }
}
