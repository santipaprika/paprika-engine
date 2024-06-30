#pragma once

#include <RHI/DescriptorHeap.h>

namespace PPK::RHI
{
	class DescriptorHeapHandle;

	// Shader visible descriptor heap
	class ShaderDescriptorHeap : public DescriptorHeap
	{
	public:
		ShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
		//~ShaderDescriptorHeap() final;

		void Reset();
		DescriptorHeapHandle GetHeapHandleBlock(uint32_t count);

	private:
		uint32_t m_currentDescriptorIndex;
	};
}
