#pragma once

#include <RHI/GPUResource.h>

namespace PPK
{
	class Renderer;
}

namespace PPK::RHI
{
	// TODO: Rename to buffer and generalize for SRV, UAV, CBV
	class ConstantBuffer final : public GPUResource
	{
	public:
		ConstantBuffer();
		ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState,
					   const DescriptorHeapHandles& constantBufferViewHandles, LPCSTR name);
		ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, LPCSTR name);
		ConstantBuffer(ConstantBuffer&& other) noexcept;
		ConstantBuffer(ConstantBuffer& other) = delete;
		ConstantBuffer& operator=(ConstantBuffer&& other) noexcept;
		~ConstantBuffer() override;

	private:
	};

	namespace ConstantBufferUtils
	{
		/**
		 * Create buffer and optionally upload data using a persistent upload heap buffer. This doesn't create views.
		 * @param alignedSize Buffer size - must be aligned (see D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
		 * @param name Resource name
		 * @param bufferData [optional] Input data to fill the buffer with
		 * @param bAllowUav [optional] Allow the resource to be used with UAVs
		 * @return Buffer GPU resource (ConstantBuffer for now but should be generic Buffer)
		 */
		ConstantBuffer CreateBuffer(uint32_t alignedSize, LPCSTR name = "UnspecifiedBufferResource",
		                            const void* bufferData = nullptr, bool bAllowUav = false);

		/**
		 * Create a constant buffer and optionally upload data using a persistent upload heap buffer.
		 * This creates the resource with a CBV view.
		 * @param bufferSize Buffer size
		 * @param name Resource name
		 * @param bufferData [optional] Input data to fill the buffer with
		 * @param alignment [optional] Custom alignment to apply to the size. Default is D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
		 * @return Constant Buffer GPU resource
		 */
		ConstantBuffer CreateConstantBuffer(uint32_t bufferSize, LPCSTR name = "ConstantBufferResource",
											const void* bufferData = nullptr,
											uint32_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		/**
		 * Create a structured buffer and optionally upload data using a persistent upload heap buffer.
		 * This creates the resource with a SRV view.
		 * @param numElements Number of elements in the structured buffer
		 * @param elementSize Element size in bytes
		 * @param name Resource name
		 * @param bufferData [optional] Input data to fill the buffer with
		 * @param alignment [optional] Custom alignment to apply to the size. Default is D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
		 * @return Constant Buffer GPU resource (representing the Structured Buffer)
		 */
		ConstantBuffer CreateStructuredBuffer(uint32_t numElements, uint32_t elementSize, LPCSTR name = "StructuredBufferResource",
											const void* bufferData = nullptr,
											uint32_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		/**
		 * Create a byte address buffer and optionally upload data using a persistent upload heap buffer.
		 * This creates the resource with SRV and UAV views.
		 * @param numElements Number of elements in the structured buffer
		 * @param elementSize Element size in bytes
		 * @param name Resource name
		 * @param bufferData [optional] Input data to fill the buffer with
		 * @param alignment [optional] Custom alignment to apply to the size. Default is D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
		 * @return Constant Buffer GPU resource (representing the Structured Buffer)
		 */
		ConstantBuffer CreateByteAddressBuffer(uint32_t numElements, uint32_t elementSize, LPCSTR name = "ByteAddressBufferResource",
											const void* bufferData = nullptr,
											uint32_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}
}
