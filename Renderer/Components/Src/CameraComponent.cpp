#include <stdafx_renderer.h>
#include <CameraComponent.h>
#include <Logger.h>
#include <string>

namespace PPK
{
	CameraComponent::CameraComponent(uint32_t cameraIdx)
		: m_speed(5.f), m_sensitivity(250.f),
		  m_constantBuffer(std::move(RHI::ConstantBuffer::CreateConstantBuffer(sizeof(CameraMatrices),
		  	std::wstring(L"CameraCB_" + std::to_wstring(cameraIdx)).c_str(), true)))
	{
	}
}
