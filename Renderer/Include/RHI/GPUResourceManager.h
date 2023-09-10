#pragma once

#include <stdafx_renderer.h>
#include <RHI/DescriptorHeap.h>

#include <RHI/StagingDescriptorHeap.h>

namespace PPK::RHI
{
	class GPUResourceManager
	{
	public:
		GPUResourceManager();
		[[nodiscard]] DescriptorHeapHandle GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetFramebufferDescriptorHandle(UINT frameIndex) const;
		static std::shared_ptr<GPUResourceManager> Get() { return m_instance; };
	protected:
		static std::shared_ptr<GPUResourceManager> m_instance;

	private:
		std::shared_ptr<StagingDescriptorHeap> m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	};
}