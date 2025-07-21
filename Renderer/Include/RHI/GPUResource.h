#pragma once

#include <RHI/DescriptorHeapElement.h>
#include <array>
#include <mutex>
#include <d3dx12/d3dx12_barriers.h>

using namespace Microsoft::WRL;
namespace PPK::RHI
{
	class DescriptorHeapHandle;

	// TODO: pointers here are not ideal probably
	typedef std::array<std::shared_ptr<DescriptorHeapElement>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorHeapElements;

	// TODO: Abstract heap-related mathods/attributes to new parent class 'HeapableObject'?
	class GPUResource
	{
	public:
		GPUResource();
		GPUResource(ComPtr<ID3D12Resource> resource, const DescriptorHeapElements& descriptorHeapElements, D3D12_RESOURCE_STATES usageState, const std::string& name);
		// Alternate constructor when we only use 1 descriptor element, handle assignment to heap elements array internally instead of relying on the caller.
		GPUResource(ComPtr<ID3D12Resource> resource, std::shared_ptr<DescriptorHeapElement> descriptorHeapElement, D3D12_RESOURCE_STATES usageState, const std::string& name);
		GPUResource(GPUResource&& other) noexcept;
		GPUResource& operator=(GPUResource&& other) noexcept;
		virtual ~GPUResource();

		[[nodiscard]] ComPtr<ID3D12Resource> GetResource() const { return m_resource; }
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GPUAddress; }
		[[nodiscard]] D3D12_RESOURCE_STATES GetUsageState() const { return m_usageState; }
		[[nodiscard]] std::shared_ptr<DescriptorHeapElement> GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;
		[[nodiscard]] std::shared_ptr<DescriptorHeapElement> GetShaderDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;
		void SetUsageState(D3D12_RESOURCE_STATES usageState) { m_usageState = usageState; }

		[[nodiscard]] bool GetIsReady() const { return m_isReady; }
		void SetIsReady(bool isReady) { m_isReady = isReady; }

		void CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType) const;

		static ComPtr<ID3D12Resource> CreateInitializedGPUResource(const void* data, size_t dataSize, D3D12_RESOURCE_STATES outputState);

		void TransitionTo(ComPtr<ID3D12GraphicsCommandList4> commandList, D3D12_RESOURCE_STATES destState);

		// Debug
		[[nodiscard]] size_t GetSizeInBytes() const;

	protected:
		ComPtr<ID3D12Resource> m_resource;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress;
		D3D12_RESOURCE_STATES m_usageState;
		bool m_isReady;

		DescriptorHeapElements m_descriptorHeapElements;

		// Debug
		size_t m_sizeInBytes;
		std::string m_name;
	};


	namespace GPUResourceUtils
	{
		void UpdateSubresourcesImmediately(ComPtr<ID3D12Resource> uploadResource, ComPtr<ID3D12Resource> resource,
		                                   D3D12_SUBRESOURCE_DATA subresourceData, CD3DX12_RESOURCE_BARRIER transitionBarrier);
	}

	extern std::mutex g_ResourceCreationMutex;
}
