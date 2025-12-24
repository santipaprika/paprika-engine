#include <Renderer.h>
#include <ApplicationHelper.h>
#include <RHI/PersistentUploadBuffer.h>

// Currently 32MB of persistent upload buffer always mapped
constexpr size_t g_persistentBufferSize = 1ull << 25;

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

    CD3DX12_RANGE readRange(0, 0);
    m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedBuffer));
    
    m_isReady = true;
}

void PersistentUploadBuffer::ResetIndex()
{
    m_firstFreeIndex = 0;
}

uint32_t PersistentUploadBuffer::SetData(const D3D12_SUBRESOURCE_DATA& subresourceData, GPUResource* destResource)
{
    // Buffer size is equal to slicePitch if we update just 1 subresource
    // TODO: For more subresources this is not valid! Double check
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
        memcpy(static_cast<byte*>(m_mappedBuffer) + m_firstFreeIndex, subresourceData.pData, bufferSize);

        // Update current index
        uint32_t bufferOffset = m_firstFreeIndex;
        m_firstFreeIndex += bufferSize;

        const ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCommandContext()->GetCurrentCommandList();
        D3D12_RESOURCE_STATES originalState = destResource->GetUsageState();
        gRenderer->TransitionResources(commandList, {
            { destResource, D3D12_RESOURCE_STATE_COPY_DEST }
        });

        // This performs the memcpy through intermediate buffer
        // TODO: Should be >1 subresources for mips/slices
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
