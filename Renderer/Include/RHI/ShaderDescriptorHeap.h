#pragma once

#include <RHI/DescriptorHeap.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
	class RenderPassDescriptorHeap : public DescriptorHeap
	{
	public:
		RenderPassDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
		//~RenderPassDescriptorHeap() final;

		void Reset();
		DescriptorHeapHandle GetHeapHandleBlock(uint32_t count);

	private:
		uint32_t m_currentDescriptorIndex;
	};
}
