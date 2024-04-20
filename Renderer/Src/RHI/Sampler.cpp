#include <RHI/Sampler.h>
#include <Renderer.h>
#include <ApplicationHelper.h>
#include <DirectXTex.h>

namespace PPK::RHI
{
	Sampler::Sampler(DescriptorHeapHandle samplerHeapHandle)
	{
	}

	Sampler::~Sampler()
	{
		Logger::Info("REMOVING Sampler");
		GPUResourceManager::Get()->FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_descriptorHeapHandle);
	}

	std::shared_ptr<Sampler> Sampler::CreateSampler(PCWSTR name)
	{
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		
		DescriptorHeapHandle samplerHeapHandle = GPUResourceManager::Get()->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		DX12Interface::Get()->GetDevice()->CreateSampler(&samplerDesc, samplerHeapHandle.GetCPUHandle());
				
		return std::make_shared<Sampler>(samplerHeapHandle);
	}
}
