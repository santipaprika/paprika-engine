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
		m_currentDescriptorHeaps[descriptorHeapType] = new StagingDescriptorHeap(
			static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType), 
			numDescriptorsPerType[descriptorHeapType]);
	}

	m_instance = std::make_shared<GPUResourceManager>(*this);
}

void GPUResourceManager::OnDestroy()
{
	for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
	{
		delete m_currentDescriptorHeaps[descriptorHeapType];
	}
}

std::shared_ptr<GPUResourceManager> GPUResourceManager::m_instance;

PPK::RHI::DescriptorHeapHandle GPUResourceManager::GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	return m_currentDescriptorHeaps[heapType]->GetNewHeapHandle();
}

void GPUResourceManager::FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle)
{
	m_currentDescriptorHeaps[heapType]->FreeHeapHandle(descriptorHeapHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUResourceManager::GetFramebufferDescriptorHandle(UINT frameIndex) const
{
	return m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetHeapCPUAtIndex(frameIndex);
}

DescriptorHeapHandle* GPUResourceManager::GetFramebuffersDescriptorHeapHandle() const
{
	return dynamic_cast<DescriptorHeapHandle*>(m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
}
