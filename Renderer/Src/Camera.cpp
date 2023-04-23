#include <Camera.h>

namespace PPK
{
	Camera::Camera(std::unique_ptr<CameraMatrices> cameraMatrices)
		: m_cameraMatrices(std::move(cameraMatrices))
	{
	}

    std::vector<Camera> Camera::m_cameras{};

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
