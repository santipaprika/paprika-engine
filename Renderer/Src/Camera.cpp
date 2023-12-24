#include <Camera.h>

namespace PPK
{
	Camera::Camera(CameraDescriptor&& cameraDescriptor)
	{
		auto camPos = DirectX::XMLoadFloat3(&cameraDescriptor.m_position);
		auto camFrontDir = DirectX::XMLoadFloat3(&cameraDescriptor.m_front);
		m_cameraMatrices.m_viewToWorld = DirectX::XMMatrixLookToRH(camPos, camFrontDir,
		                                                           DirectX::FXMVECTOR{0.f, 1.f, 0.f});
		m_cameraMatrices.m_worldToView = m_cameraMatrices.m_viewToWorld.GetInverse();
		m_cameraMatrices.m_viewToClip = DirectX::XMMatrixPerspectiveFovRH(
			cameraDescriptor.m_cameraInternals.m_fov, cameraDescriptor.m_cameraInternals.m_aspectRatio,
			cameraDescriptor.m_cameraInternals.m_near,
			cameraDescriptor.m_cameraInternals.m_far);

		m_constantBuffer = RHI::ConstantBuffer::CreateConstantBuffer(sizeof(CameraMatrices));
		m_constantBuffer->SetConstantBufferData((void*)&m_cameraMatrices, sizeof(CameraMatrices));

        //m_cameras.push_back(*this);
	}

	Camera::~Camera()
	{
        Logger::Info("Removing camera");
        delete m_constantBuffer;
        //m_cameras.pop_back();
	}

	//std::vector<Camera&> Camera::m_cameras{};

    void Camera::Upload(Renderer& renderer)
    {
        // Create the vertex buffer.
        constexpr float aspectRatio = 1.f;

		// Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { -1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 1.f, -1.f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -1.f, -1.f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        // m_vertexBuffer.reset(RHI::VertexBuffer::CreateVertexBuffer(triangleVertices, sizeof(Vertex), sizeof(triangleVertices), renderer));
    }

    // Camera* Camera::Create(std::unique_ptr<MeshData> meshData)
    // {
    //     m_meshes.push_back(Mesh(std::move(meshData)));
    //     return GetLastMesh();
    // }
}
