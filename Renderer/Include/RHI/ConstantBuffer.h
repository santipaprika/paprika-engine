#pragma once

#include <RHI/GPUResource.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
	class ConstantBuffer final : public GPUResource
	{
	public:
		ConstantBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize,
		               DescriptorHeapHandle constantBufferViewHandle);
		~ConstantBuffer() override;

		void SetConstantBufferData(const void* bufferData, uint32_t bufferSize);
		[[nodiscard]] DescriptorHeapHandle GetConstantBufferViewHandle() const { return mConstantBufferViewHandle; }

	private:
		void* mMappedBuffer;
		uint32_t mBufferSize;
		DescriptorHeapHandle mConstantBufferViewHandle;
	};
}
