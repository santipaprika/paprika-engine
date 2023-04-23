#pragma once

#include <RHI/DescriptorHeap.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
	class StagingDescriptorHeap : public DescriptorHeap
	{
	public:
		StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
		~StagingDescriptorHeap() final;

		DescriptorHeapHandle GetNewHeapHandle();
		void FreeHeapHandle(DescriptorHeapHandle handle);

	private:
		std::vector<uint32_t> m_freeDescriptors;
		uint32_t m_currentDescriptorIndex;
		uint32_t m_activeHandleCount;
	};
}
