#include <RHI/DescriptorHeapManager.h>
#include <Renderer.h>

using namespace PPK::RHI;

constexpr int numDescriptorsPerType[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		12,	// CBV_SRV_UAV
		2,	// SAMPLER
		6,	// RTV
		2	// DSV
};

//DescriptorHeapManager* gDescriptorHeapManager;

DescriptorHeapManager::DescriptorHeapManager()
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
	{
		m_stagingDescriptorHeaps[descriptorHeapType] = new StagingDescriptorHeap(
			static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType), 
			numDescriptorsPerType[descriptorHeapType]);

		// RTV and DSV can't include shader visible heap flag
		if (descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			for (int i = 0; i < gFrameCount; i++)
			{
				m_shaderDescriptorHeaps[i][descriptorHeapType] = new ShaderDescriptorHeap(
					static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType),
					numDescriptorsPerType[descriptorHeapType]);
			}
		}
	}

	m_instance = std::make_shared<DescriptorHeapManager>(*this);
}

void DescriptorHeapManager::OnDestroy()
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
	{
		if (descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		{
			for (int i = 0; i < gFrameCount; i++)
			{
				delete m_shaderDescriptorHeaps[i][descriptorHeapType];
			}
		}
		delete m_stagingDescriptorHeaps[descriptorHeapType];
	}
}

std::shared_ptr<DescriptorHeapManager> DescriptorHeapManager::m_instance;

DescriptorHeapHandle DescriptorHeapManager::GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	return m_stagingDescriptorHeaps[heapType]->GetNewHeapHandle();
}

DescriptorHeapHandle DescriptorHeapManager::GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType,
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

void DescriptorHeapManager::FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle)
{
	m_stagingDescriptorHeaps[heapType]->FreeHeapHandle(descriptorHeapHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetFramebufferDescriptorHandle(UINT frameIndex) const
{
	return m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetHeapCPUAtIndex(frameIndex);
}

DescriptorHeapHandle* DescriptorHeapManager::GetFramebuffersDescriptorHeapHandle() const
{
	return dynamic_cast<DescriptorHeapHandle*>(m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
}

ShaderDescriptorHeap* DescriptorHeapManager::GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT frameIndex) const
{
	return m_shaderDescriptorHeaps[frameIndex][heapType];
}

void DescriptorHeapManager::ResetShaderHeap(UINT frameIndex)
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES - 2; descriptorHeapType++)
	{
		m_shaderDescriptorHeaps[frameIndex][descriptorHeapType]->Reset();
	}
}
