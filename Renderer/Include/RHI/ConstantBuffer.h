#pragma once

#include <RHI/GPUResource.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK
{
	class Renderer;
}

namespace PPK::RHI
{
	class ConstantBuffer final : public GPUResource
	{
	public:
		ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize,
		               DescriptorHeapHandle constantBufferViewHandle);
		~ConstantBuffer() override;

		void SetConstantBufferData(const void* bufferData, uint32_t bufferSize);

		static ConstantBuffer* CreateConstantBuffer(uint32_t bufferSize, LPCWSTR name = L"ConstantBufferResource",
		                                            bool allowCpuWrites = false, const void* bufferData = nullptr);

	private:
		void* m_mappedBuffer;
		uint32_t m_bufferSize;
	};
}
