#pragma once

#include <RHI/DescriptorHeapElement.h>
#include <array>
#include <mutex>
#include <stdafx_renderer.h>
#include <d3dx12/d3dx12_barriers.h>

#define INVALID_INDEX -1

using namespace Microsoft::WRL;
namespace PPK::RHI
{
	class DescriptorHeapHandle;

	// TODO: pointers here are not ideal probably
	struct DescriptorHeapHandles
	{
		// TODO: Check if handle per frame is really needed after transition to bindless
		std::array<std::array<DescriptorHeapHandle, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>, gFrameCount> handles;
	};

	// TODO: Abstract heap-related mathods/attributes to new parent class 'HeapableObject'?
	class GPUResource
	{
	public:
		GPUResource();
		// Alternate constructor when we only use 1 descriptor element, handle assignment to heap elements array internally instead of relying on the caller.
		GPUResource(ComPtr<ID3D12Resource> resource, const DescriptorHeapHandles& descriptorHeapHandles,
									 D3D12_RESOURCE_STATES usageState, const std::string& name);
		GPUResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, const std::string& name);
		GPUResource(GPUResource&& other) noexcept;
		GPUResource& operator=(GPUResource&& other) noexcept;
		virtual ~GPUResource();

		void SetupResourceStats();
		void AddDescriptorHandle(const DescriptorHeapHandle& heapHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t frameIdx);

		[[nodiscard]] ComPtr<ID3D12Resource> GetResource() const { return m_resource; }
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GPUAddress; }
		[[nodiscard]] D3D12_RESOURCE_STATES GetUsageState() const { return m_usageState; }
		[[nodiscard]] const DescriptorHeapHandle& GetDescriptorHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t frameIdx = 0) const;
		[[nodiscard]] uint32_t GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) const;
		void SetUsageState(D3D12_RESOURCE_STATES usageState) { m_usageState = usageState; }

		[[nodiscard]] bool GetIsReady() const { return m_isReady; }
		void SetIsReady(bool isReady) { m_isReady = isReady; }

		void CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;


		// Debug
		[[nodiscard]] size_t GetSizeInBytes() const;

	protected:
		ComPtr<ID3D12Resource> m_resource;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress;
		D3D12_RESOURCE_STATES m_usageState;
		bool m_isReady;

		DescriptorHeapHandles m_descriptorHeapHandles;

		// Debug
		size_t m_sizeInBytes;
		std::string m_name;
	};


	namespace GPUResourceUtils
	{
		ComPtr<ID3D12Resource> CreateUninitializedGPUBuffer(size_t alignedSize, LPCSTR name, D3D12_RESOURCE_STATES outputState);
		
	}

	extern std::mutex g_ResourceCreationMutex;
}
