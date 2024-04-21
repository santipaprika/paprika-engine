#include <RHI/Sampler.h>
#include <Renderer.h>
#include <ApplicationHelper.h>
#include <DirectXTex.h>

namespace PPK::RHI
{
	Sampler::Sampler(std::shared_ptr<DescriptorHeapElement> samplerHeapElement)
		: m_descriptorHeapElement(samplerHeapElement)
	{
	}

	Sampler::~Sampler()
	{
		Logger::Info("REMOVING Sampler");
	}

	std::shared_ptr<Sampler> Sampler::CreateSampler(PCWSTR name)
	{
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		
		std::shared_ptr<DescriptorHeapElement> samplerHeapElement = std::make_shared<DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		DX12Interface::Get()->GetDevice()->CreateSampler(&samplerDesc, samplerHeapElement->GetCPUHandle());
				
		return std::make_shared<Sampler>(samplerHeapElement);
	}
}
