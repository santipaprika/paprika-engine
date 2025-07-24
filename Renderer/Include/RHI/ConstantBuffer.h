#pragma once

#include <RHI/GPUResource.h>

namespace PPK
{
	class Renderer;
}

namespace PPK::RHI
{
	class ConstantBuffer final : public GPUResource
	{
	public:
		ConstantBuffer();
		ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, uint32_t bufferSize,
					   std::shared_ptr<DescriptorHeapElement> constantBufferViewElement, LPCSTR name);
		ConstantBuffer(ConstantBuffer&& other) noexcept;
		ConstantBuffer(ConstantBuffer& other) = delete;
		ConstantBuffer& operator=(ConstantBuffer&& other) noexcept;
		~ConstantBuffer() override;

		void SetConstantBufferData(const void* bufferData, uint32_t bufferSize);

	private:
		void* m_mappedBuffer;
		uint32_t m_bufferSize;
	};

	namespace ConstantBufferUtils
	{
		ConstantBuffer CreateConstantBuffer(uint32_t bufferSize, LPCSTR name = "ConstantBufferResource",
											bool allowCpuWrites = false, const void* bufferData = nullptr,
											uint32_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		void UpdateConstantBufferData(RHI::ConstantBuffer& constantBuffer, const void* data, uint32_t bufferSize);
	}
}
