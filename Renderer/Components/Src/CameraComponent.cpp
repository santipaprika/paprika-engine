#include <stdafx_renderer.h>
#include <CameraComponent.h>
#include <Logger.h>
#include <string>

namespace PPK
{
	CameraComponent::CameraComponent(uint32_t cameraIdx)
		: m_speed(5.f), m_sensitivity(100.f),
		  m_constantBuffer(std::move(*RHI::ConstantBuffer::CreateConstantBuffer(sizeof(CameraMatrices),
		  	std::wstring(L"CameraCB_" + std::to_wstring(cameraIdx)).c_str(), true)))
	{
        // CreateCameraConstantBuffer();
        // UpdateCameraMatrices(cameraDescriptor);

        //m_cameras.push_back(*this);
	}

	// CameraComponent::~CameraComponent()
	// {
 //        Logger::Info("Removing camera");
 //        //m_cameras.pop_back();
	// }

	//std::vector<Camera&> CameraComponent::m_cameras{};

    // void CameraComponent::CreateCameraConstantBuffer()
    // {
    //     static uint32_t cameraIdx = 0;
    //     const std::wstring bufferName = L"CameraCB_" + std::to_wstring(cameraIdx++);
    //
    //     // m_constantBuffer = std::move(*RHI::ConstantBuffer::CreateConstantBuffer(sizeof(CameraMatrices), bufferName.c_str(), true));
    // }

  //   void CameraComponent::UpdateCameraMatrices(const CameraDescriptor& cameraDescriptor)
  //   {
  // //       CameraMatrices cameraMatrices;
  // //       cameraMatrices.m_viewToWorld = cameraDescriptor.m_transform;
  // //       cameraMatrices.m_worldToView =  cameraMatrices.m_viewToWorld.GetInverse();
  // //       cameraMatrices.m_viewToClip = Matrix::CreatePerspectiveFieldOfView(
  // //           cameraDescriptor.m_cameraInternals.m_fov, cameraDescriptor.m_cameraInternals.m_aspectRatio,
  // //           cameraDescriptor.m_cameraInternals.m_near,
  // //           cameraDescriptor.m_cameraInternals.m_far);
  // //
		// // UpdateConstantBufferData(cameraMatrices);
  //   }

    // void CameraComponent::UpdateConstantBufferData(const CameraMatrices& cameraMatrices)
    // {
    //     m_constantBuffer.SetConstantBufferData((void*)&cameraMatrices, sizeof(CameraMatrices));
    // }

    // Camera* CameraComponent::Create(std::unique_ptr<MeshData> meshData)
    // {
    //     m_meshes.push_back(Mesh(std::move(meshData)));
    //     return GetLastMesh();
    // }
}
