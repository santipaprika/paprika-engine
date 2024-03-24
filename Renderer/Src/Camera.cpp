#include <Camera.h>

namespace PPK
{
	Camera::Camera(const CameraDescriptor& cameraDescriptor)
	{
        CreateCameraConstantBuffer();
        UpdateCameraMatrices(cameraDescriptor);

        //m_cameras.push_back(*this);
	}

	Camera::~Camera()
	{
        Logger::Info("Removing camera");
        //m_cameras.pop_back();
	}

	//std::vector<Camera&> Camera::m_cameras{};

    void Camera::CreateCameraConstantBuffer()
    {
        static uint32_t cameraIdx = 0;
        const std::wstring bufferName = L"CameraCB_" + std::to_wstring(cameraIdx++);

        m_constantBuffer = std::shared_ptr<RHI::ConstantBuffer>(
            RHI::ConstantBuffer::CreateConstantBuffer(sizeof(CameraMatrices),
                bufferName.c_str(),
                true)
        );
    }

    void Camera::UpdateCameraMatrices(const CameraDescriptor& cameraDescriptor)
    {
        CameraMatrices cameraMatrices;
        cameraMatrices.m_viewToWorld = cameraDescriptor.m_transform;
        cameraMatrices.m_worldToView =  cameraMatrices.m_viewToWorld.GetInverse();
        cameraMatrices.m_viewToClip = Matrix::CreatePerspectiveFieldOfView(
            cameraDescriptor.m_cameraInternals.m_fov, cameraDescriptor.m_cameraInternals.m_aspectRatio,
            cameraDescriptor.m_cameraInternals.m_near,
            cameraDescriptor.m_cameraInternals.m_far);

		UpdateConstantBufferData(cameraMatrices);
    }

    void Camera::UpdateConstantBufferData(const CameraMatrices& cameraMatrices)
    {
        m_constantBuffer->SetConstantBufferData((void*)&cameraMatrices, sizeof(CameraMatrices));
    }

    // Camera* Camera::Create(std::unique_ptr<MeshData> meshData)
    // {
    //     m_meshes.push_back(Mesh(std::move(meshData)));
    //     return GetLastMesh();
    // }
}
