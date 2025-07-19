#pragma once

#include <mutex>
#include <RHI/DescriptorHeap.h>

namespace PPK::RHI
{
	class DescriptorHeapHandle;

	class StagingDescriptorHeap : public DescriptorHeap
	{
	public:
		StagingDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
		~StagingDescriptorHeap() final;

		DescriptorHeapHandle GetNewHeapHandle();
		void FreeHeapHandle(DescriptorHeapHandle handle);

	private:
		std::mutex m_handleUpdateMutex;
		std::vector<uint32_t> m_freeDescriptors;
		uint32_t m_currentDescriptorIndex;
		uint32_t m_activeHandleCount;
	};
}
