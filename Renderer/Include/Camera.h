#pragma once

#include <Transform.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/VertexBuffer.h>

namespace PPK
{
	struct RenderContext;

	class Camera
	{
	public:
		struct CameraInternals
		{
			float m_near = 0.001f;
			float m_far = 100.f;
			float m_fov = DirectX::XMConvertToRadians(60.f);
			float m_aspectRatio = 1.f;
		};

		struct CameraDescriptor
		{
			CameraInternals m_cameraInternals;
			DirectX::XMFLOAT3 m_position;
			DirectX::XMFLOAT3 m_front;
		};

		struct CameraMatrices
		{
			Transform m_worldToView;
			Transform m_viewToWorld;
			Transform m_viewToClip;
		};

		Camera() = default;
        explicit Camera(CameraDescriptor&& cameraDescriptor);
		~Camera();

		void Upload(Renderer& renderer);

		//static Camera* Create(std::unique_ptr<CameraMatrices> meshData);
		// static Camera* GetCamera(uint32_t meshId) { return &m_cameras[meshId]; };
		// static Camera* GetLastCamera() { return &m_cameras.back(); };
		// static std::vector<Camera>& GetCameras() { return m_cameras; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		//static std::vector<Camera&> m_cameras;

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};

        [[nodiscard]] const RHI::ConstantBuffer* GetConstantBuffer() const { return m_constantBuffer; };
        [[nodiscard]] RHI::DescriptorHeapHandle GetConstantBufferViewHandle() const { return m_constantBuffer->GetDescriptorHeapHandle(); };

	private:
		CameraMatrices m_cameraMatrices;
		RHI::ConstantBuffer* m_constantBuffer;
	};
}
