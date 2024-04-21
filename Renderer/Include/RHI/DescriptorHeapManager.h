#pragma once

#include <stdafx_renderer.h>

#include <RHI/StagingDescriptorHeap.h>
#include <RHI/ShaderDescriptorHeap.h>


using Microsoft::WRL::ComPtr;

namespace PPK::RHI
{
	class DescriptorHeapManager
	{
	public:
		DescriptorHeapManager();
		void OnDestroy();
		[[nodiscard]] DescriptorHeapHandle GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		[[nodiscard]] DescriptorHeapHandle GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t count, uint32_t frameIdx);
		void FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle);
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetFramebufferDescriptorHandle(UINT frameIndex) const;
		[[nodiscard]] DescriptorHeapHandle* GetFramebuffersDescriptorHeapHandle() const;
		[[nodiscard]] ShaderDescriptorHeap* GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT frameIndex) const;
		void ResetShaderHeap(UINT frameIndex);
		static std::shared_ptr<DescriptorHeapManager> Get() { return m_instance; };
	protected:
		static std::shared_ptr<DescriptorHeapManager> m_instance;
		static constexpr UINT FrameCount = 2;

	private:
		StagingDescriptorHeap* m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		ShaderDescriptorHeap* m_shaderDescriptorHeaps[FrameCount][D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES - 2]; // Exclude RTV and DSV
	};
}