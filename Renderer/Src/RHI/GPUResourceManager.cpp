#include <RHI/GPUResourceManager.h>
#include <Renderer.h>

using namespace PPK::RHI;

constexpr int numDescriptorsPerType[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		10,	// CBV_SRV_UAV
		2,	// SAMPLER
		2,	// RTV
		2	// DSV
};

GPUResourceManager::GPUResourceManager()
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
	{
		m_stagingDescriptorHeaps[descriptorHeapType] = new StagingDescriptorHeap(
			static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType), 
			numDescriptorsPerType[descriptorHeapType]);

		// RTV and DSV can't include shader visible heap flag
		if (descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			for (int i = 0; i < FrameCount; i++)
			{
				m_shaderDescriptorHeaps[i][descriptorHeapType] = new ShaderDescriptorHeap(
					static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType),
					numDescriptorsPerType[descriptorHeapType]);
			}
		}
	}

	m_instance = std::make_shared<GPUResourceManager>(*this);
}

void GPUResourceManager::OnDestroy()
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
	{
		if (descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			for (int i = 0; i < FrameCount; i++)
			{
				delete m_shaderDescriptorHeaps[i][descriptorHeapType];
			}
		}
		delete m_stagingDescriptorHeaps[descriptorHeapType];
	}
}

std::shared_ptr<GPUResourceManager> GPUResourceManager::m_instance;

PPK::RHI::DescriptorHeapHandle GPUResourceManager::GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	return m_stagingDescriptorHeaps[heapType]->GetNewHeapHandle();
}

DescriptorHeapHandle GPUResourceManager::GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	uint32_t count, uint32_t frameIdx)
{
	// Lazy hacky way to detect when a handle for a new frame is requested, in order to reset the heap
	// Careful, if multithreading is implemented in the future this needs to be changed
	static uint32_t currentIdx = frameIdx;
	if (currentIdx != frameIdx)
	{
		ResetShaderHeap(frameIdx);
		currentIdx = frameIdx;
	}
	return m_shaderDescriptorHeaps[frameIdx][heapType]->GetHeapHandleBlock(count);
}

void GPUResourceManager::FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle)
{
	m_stagingDescriptorHeaps[heapType]->FreeHeapHandle(descriptorHeapHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUResourceManager::GetFramebufferDescriptorHandle(UINT frameIndex) const
{
	return m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetHeapCPUAtIndex(frameIndex);
}

DescriptorHeapHandle* GPUResourceManager::GetFramebuffersDescriptorHeapHandle() const
{
	return dynamic_cast<DescriptorHeapHandle*>(m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
}

void GPUResourceManager::ResetShaderHeap(UINT frameIndex)
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES - 2; descriptorHeapType++)
	{
		m_shaderDescriptorHeaps[frameIndex][descriptorHeapType]->Reset();
	}
}
