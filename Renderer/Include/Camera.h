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
		struct CameraMatrices
		{
			Transform m_worldToView;
			Transform m_viewToWorld;
			Transform m_viewToClip;
		};

        explicit Camera(std::unique_ptr<CameraMatrices> cameraMatrices);

		void Upload(Renderer& renderer);

		static Camera* Create(std::unique_ptr<CameraMatrices> meshData);
		static Camera* GetCamera(uint32_t meshId) { return &m_cameras[meshId]; };
		static Camera* GetLastCamera() { return &m_cameras.back(); };
		static std::vector<Camera>& GetCameras() { return m_cameras; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		static std::vector<Camera> m_cameras;

		struct Vertex
		{
			DirectX::XMFLOAT3 position;
			DirectX::XMFLOAT4 color;
		};

        [[nodiscard]] RHI::ConstantBuffer* GetConstantBuffer() const { return m_constantBuffer.get(); };
        [[nodiscard]] RHI::DescriptorHeapHandle GetConstantBufferViewHandle() const { return m_constantBuffer->GetConstantBufferViewHandle(); };

	private:
        std::unique_ptr<CameraMatrices> m_cameraMatrices = nullptr;
		std::unique_ptr<RHI::ConstantBuffer> m_constantBuffer = nullptr;
	};
}
