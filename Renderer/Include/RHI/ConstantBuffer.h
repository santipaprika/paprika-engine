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
		ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize,
		               DescriptorHeapHandle constantBufferViewHandle);
		~ConstantBuffer() override;

		void SetConstantBufferData(const void* bufferData, uint32_t bufferSize);
		[[nodiscard]] DescriptorHeapHandle GetConstantBufferViewHandle() const { return m_constantBufferViewHandle; }

		static ConstantBuffer* CreateConstantBuffer(uint32_t bufferSize);

	private:
		void* m_mappedBuffer;
		uint32_t m_bufferSize;
		DescriptorHeapHandle m_constantBufferViewHandle;
	};
}
