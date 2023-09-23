#pragma once

#include <stdafx_renderer.h>

#include <RHI/DescriptorHeapHandle.h>

using namespace Microsoft::WRL;
namespace PPK::RHI
{
	class DescriptorHeapHandle;

	class GPUResource
	{
	public:
		GPUResource(ComPtr<ID3D12Resource> resource, DescriptorHeapHandle descriptorHeapHandle, D3D12_RESOURCE_STATES usageState);
		virtual ~GPUResource();

		[[nodiscard]] ComPtr<ID3D12Resource> GetResource() const { return m_resource; }
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GPUAddress; }
		[[nodiscard]] D3D12_RESOURCE_STATES GetUsageState() const { return m_usageState; }
		[[nodiscard]] DescriptorHeapHandle GetDescriptorHeapHandle() const { return m_descriptorHeapHandle; }
		void SetUsageState(D3D12_RESOURCE_STATES usageState) { m_usageState = usageState; }

		[[nodiscard]] bool GetIsReady() const { return m_isReady; }
		void SetIsReady(bool isReady) { m_isReady = isReady; }

	protected:
		ComPtr<ID3D12Resource> m_resource;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress;
		D3D12_RESOURCE_STATES m_usageState;
		DescriptorHeapHandle m_descriptorHeapHandle;
		bool m_isReady;
	};
}
