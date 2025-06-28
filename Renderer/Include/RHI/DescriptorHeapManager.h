#pragma once

#include <RHI/StagingDescriptorHeap.h>
#include <RHI/ShaderDescriptorHeap.h>


using Microsoft::WRL::ComPtr;

namespace PPK::RHI
{
	constexpr UINT gFrameCount = 2;

	class DescriptorHeapManager
	{
	public:
		DescriptorHeapManager();
		void OnDestroy();
		[[nodiscard]] DescriptorHeapHandle GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		[[nodiscard]] DescriptorHeapHandle GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t frameIdx, HeapLocation heapLocation);
		void FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, DescriptorHeapHandle descriptorHeapHandle);
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetFramebufferDescriptorHandle(UINT frameIndex) const;
		[[nodiscard]] DescriptorHeapHandle* GetFramebuffersDescriptorHeapHandle() const;
		[[nodiscard]] ShaderDescriptorHeap* GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT frameIndex) const;
		void ResetShaderHeap(UINT frameIndex);
		static std::shared_ptr<DescriptorHeapManager> Get() { return m_instance; };
	protected:
		static std::shared_ptr<DescriptorHeapManager> m_instance;

	private:
		StagingDescriptorHeap* m_stagingDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		ShaderDescriptorHeap* m_shaderDescriptorHeaps[gFrameCount][D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES - 2]; // Exclude RTV and DSV
	};
}