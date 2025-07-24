#include <stdafx_renderer.h>
#include <CameraComponent.h>
#include <Logger.h>
#include <Renderer.h>
#include <string>

namespace PPK
{
	CameraComponent::CameraComponent(uint32_t cameraIdx)
		: m_speed(1.f), m_sensitivity(250.f),
		  m_constantBuffer(std::move(RHI::ConstantBufferUtils::CreateConstantBuffer(sizeof(CameraMatrices),
		  	std::string("CameraCB_" + std::to_string(cameraIdx)).c_str(), true)))
	{
	}

	void CameraComponent::InitScenePassData()
	{
		for (int frameIdx = 0; frameIdx < RHI::gFrameCount; frameIdx++)
		{
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);

			// Copy descriptors to shader visible heap
			cbvSrvHeap->CopyDescriptors(&GetConstantBuffer(), RHI::HeapLocation::VIEWS);
		}
	}
}
