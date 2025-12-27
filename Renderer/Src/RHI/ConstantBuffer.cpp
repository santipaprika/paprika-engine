#include <ApplicationHelper.h>
#include <cassert>
#include <Renderer.h>
#include <RHI/ConstantBuffer.h>

namespace PPK::RHI
{
	ConstantBuffer::ConstantBuffer()
	{
	}

	ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState,
	                               const DescriptorHeapHandles& constantBufferViewHandles, LPCSTR name)
		: GPUResource(resource, constantBufferViewHandles, usageState, name)
	{
	}

	ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, LPCSTR name)
		: GPUResource(resource, usageState, name)
	{
	}

	ConstantBuffer::ConstantBuffer(ConstantBuffer&& other) noexcept: GPUResource(std::move(other))
	{
	}

	ConstantBuffer& ConstantBuffer::operator=(ConstantBuffer&& other) noexcept
	{
		if (this != &other)
		{
		}

		GPUResource::operator=(GPUResource(std::forward<GPUResource>(other)));
	
		return *this;
	}

	ConstantBuffer::~ConstantBuffer()
	{
		Logger::Verbose(("REMOVING CB " + std::string(m_name)).c_str());
	}

	namespace ConstantBufferUtils
	{
		static ConstantBuffer CreateBuffer(uint32_t alignedSize, LPCSTR name, const void* bufferData, bool bAllowUav)
		{
			ComPtr<ID3D12Resource> constantBufferResource = GPUResourceUtils::CreateUninitializedGPUBuffer(alignedSize, name, bAllowUav);
			ConstantBuffer constantBuffer = std::move(ConstantBuffer(constantBufferResource,
			                                                         D3D12_RESOURCE_STATE_GENERIC_READ, name));
			if (bufferData)
			{
				// Copy data to the intermediate upload heap and then schedule a copy
				// from the upload heap to the default heap constant buffer.
				D3D12_SUBRESOURCE_DATA subresourceData;
				subresourceData.pData = bufferData;
				subresourceData.RowPitch = alignedSize;
				subresourceData.SlicePitch = alignedSize;
				gRenderer->SetBufferData(subresourceData, &constantBuffer);
			}

			constantBuffer.SetIsReady(true);

			return std::move(constantBuffer);
		}

		ConstantBuffer CreateConstantBuffer(uint32_t bufferSize, LPCSTR name, const void* bufferData, uint32_t alignment)
		{
			const uint32_t alignedSize = (bufferSize / alignment + 1) * alignment;
			ConstantBuffer constantBuffer = CreateBuffer(alignedSize, name, bufferData);

			// Create CBV view
			Logger::Verbose(("CREATING heap handle for buffer " + std::string(name)).c_str());
	
			D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {};
			constantBufferViewDesc.SizeInBytes = alignedSize;
			constantBufferViewDesc.BufferLocation = constantBuffer.GetResource()->GetGPUVirtualAddress();

			for (int i = 0; i < gFrameCount; i++)
			{
				ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);
				DescriptorHeapHandle handle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::OBJECTS);
				gDevice->CreateConstantBufferView(&constantBufferViewDesc, handle.GetCPUHandle());
				constantBuffer.AddDescriptorHandle(handle, RHI::EResourceViewType::CBV, i);
			}

			return std::move(constantBuffer);
		}

		ConstantBuffer CreateStructuredBuffer(uint32_t numElements, uint32_t elementSize, LPCSTR name, const void* bufferData,
		                                      uint32_t alignment)
		{
			uint32_t bufferSize = elementSize * numElements;
			const uint32_t alignedSize = (bufferSize / alignment + 1) * alignment;
			ConstantBuffer structuredBuffer = CreateBuffer(alignedSize, name, bufferData);

			Logger::Verbose(("CREATING heap handle for buffer " + std::string(name)).c_str());

			// TODO: view types shouldn't be mutually exclusive - fix by modifying handles array indexing
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			D3D12_BUFFER_SRV bufferSrv;
			bufferSrv.FirstElement = 0;
			bufferSrv.NumElements = numElements;
			bufferSrv.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			bufferSrv.StructureByteStride = elementSize;
			srvDesc.Buffer = bufferSrv;
			for (int i = 0; i < gFrameCount; i++)
			{
				ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);
				DescriptorHeapHandle handle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::OBJECTS);
				gDevice->CreateShaderResourceView(structuredBuffer.GetResource().Get(), &srvDesc, handle.GetCPUHandle());
				structuredBuffer.AddDescriptorHandle(handle, RHI::EResourceViewType::SRV, i);
			}

			return std::move(structuredBuffer);
		}

		ConstantBuffer CreateByteAddressBuffer(uint32_t numElements, uint32_t elementSize, LPCSTR name, const void* bufferData,
											  uint32_t alignment)
		{
			uint32_t bufferSize = elementSize * numElements;
			const uint32_t alignedSize = (bufferSize / alignment + 1) * alignment;
			ConstantBuffer byteAddressBuffer = CreateBuffer(alignedSize, name, bufferData, /* bAllowUav */ true);

			Logger::Verbose(("CREATING heap handle for buffer " + std::string(name)).c_str());

			// TODO: view types shouldn't be mutually exclusive - fix by modifying handles array indexing
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			D3D12_BUFFER_SRV bufferSrv;
			bufferSrv.FirstElement = 0;
			bufferSrv.NumElements = numElements;
			bufferSrv.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			bufferSrv.StructureByteStride = 0;
			srvDesc.Buffer = bufferSrv;

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			D3D12_BUFFER_UAV bufferUav;
			bufferUav.FirstElement = 0;
			bufferUav.NumElements = numElements;
			bufferUav.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			bufferUav.StructureByteStride = 0;
			bufferUav.CounterOffsetInBytes = 0;
			uavDesc.Buffer = bufferUav;
			for (int i = 0; i < gFrameCount; i++)
			{
				ShaderDescriptorHeap* resourceDescriptorHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i);

				DescriptorHeapHandle srvHandle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::OBJECTS);
				gDevice->CreateShaderResourceView(byteAddressBuffer.GetResource().Get(), &srvDesc, srvHandle.GetCPUHandle());
				byteAddressBuffer.AddDescriptorHandle(srvHandle, RHI::EResourceViewType::SRV, i);

				DescriptorHeapHandle uavHandle = resourceDescriptorHeap->GetHeapLocationNewHandle(HeapLocation::OBJECTS);
				gDevice->CreateUnorderedAccessView(byteAddressBuffer.GetResource().Get(), nullptr, &uavDesc, uavHandle.GetCPUHandle());
				byteAddressBuffer.AddDescriptorHandle(uavHandle, RHI::EResourceViewType::UAV, i);
			}

			return std::move(byteAddressBuffer);
		}
	}
}
