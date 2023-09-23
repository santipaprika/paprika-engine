#pragma once

#include <stdafx_renderer.h>
#include <RHI/DescriptorHeap.h>

#include <RHI/StagingDescriptorHeap.h>

#include <RHI/GPUResource.h>

using Microsoft::WRL::ComPtr;

namespace PPK::RHI
{
	class GPUResourceManager
	{
	public:
		GPUResourceManager();
		void OnDestroy();
		[[nodiscard]] DescriptorHeapHandle GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		void FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle);
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetFramebufferDescriptorHandle(UINT frameIndex) const;
		[[nodiscard]] DescriptorHeapHandle* GetFramebuffersDescriptorHeapHandle() const;
		static std::shared_ptr<GPUResourceManager> Get() { return m_instance; };
	protected:
		static std::shared_ptr<GPUResourceManager> m_instance;

	private:
		StagingDescriptorHeap* m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	};
}