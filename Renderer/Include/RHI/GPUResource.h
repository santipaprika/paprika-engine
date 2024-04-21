#pragma once

#include <stdafx_renderer.h>

#include <RHI/DescriptorHeapElement.h>

using namespace Microsoft::WRL;
namespace PPK::RHI
{
	class DescriptorHeapHandle;

	// TODO: Abstract heap-related mathods/attributes to new parent class 'HeapableObject'?
	class GPUResource
	{
	public:
		GPUResource(ComPtr<ID3D12Resource> resource, std::shared_ptr<DescriptorHeapElement> descriptorHeapElement, D3D12_RESOURCE_STATES usageState);
		virtual ~GPUResource();

		[[nodiscard]] ComPtr<ID3D12Resource> GetResource() const { return m_resource; }
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GPUAddress; }
		[[nodiscard]] D3D12_RESOURCE_STATES GetUsageState() const { return m_usageState; }
		[[nodiscard]] std::shared_ptr<DescriptorHeapElement> GetDescriptorHeapElement() const { return m_descriptorHeapElement; }
		void SetUsageState(D3D12_RESOURCE_STATES usageState) { m_usageState = usageState; }

		[[nodiscard]] bool GetIsReady() const { return m_isReady; }
		void SetIsReady(bool isReady) { m_isReady = isReady; }

		void CopyDescriptorsToShaderHeap(D3D12_CPU_DESCRIPTOR_HANDLE& currentCBVHandle);

	protected:
		ComPtr<ID3D12Resource> m_resource;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress;
		D3D12_RESOURCE_STATES m_usageState;
		bool m_isReady;
		// This should be an array if multiple views want to be supported?
		std::shared_ptr<DescriptorHeapElement> m_descriptorHeapElement;
	};
}
