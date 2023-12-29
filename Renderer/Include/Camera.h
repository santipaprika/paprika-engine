#pragma once

#include <RHI/ConstantBuffer.h>
#include <RHI/VertexBuffer.h>
#include <Transform.h>

#define WIDTH 1280
#define HEIGHT 720
#define ASPECT_RATIO (float(WIDTH) / float(HEIGHT))

namespace PPK
{
	struct RenderContext;

	// Low level interpretation of camera which interacts with RHI and sets up the required buffers
	class Camera
	{
	public:
		struct CameraInternals
		{
			float m_near = 0.001f;
			float m_far = 100.f;
			float m_fov = DirectX::XMConvertToRadians(60.f);
			float m_aspectRatio = ASPECT_RATIO;
		};

		struct CameraDescriptor
		{
			CameraInternals m_cameraInternals;
			Transform m_transform;

			CameraDescriptor() = default;
			CameraDescriptor(Transform& transform) : m_transform(transform) {}
		};

		struct CameraMatrices
		{
			Transform m_worldToView;
			Transform m_viewToWorld;
			Transform m_viewToClip;
		};

		Camera() = default;
        explicit Camera(const CameraDescriptor& cameraDescriptor);
		~Camera();

		void Upload(Renderer& renderer);
		void CreateCameraConstantBuffer();
		void UpdateCameraMatrices(const CameraDescriptor& cameraDescriptor);

		//static Camera* Create(std::unique_ptr<CameraMatrices> meshData);
		// static Camera* GetCamera(uint32_t meshId) { return &m_cameras[meshId]; };
		// static Camera* GetLastCamera() { return &m_cameras.back(); };
		// static std::vector<Camera>& GetCameras() { return m_cameras; };
		// Most passes will iterate over meshes, so it's better to have them in
		// contiguous memory to optimize cache usage
		//static std::vector<Camera&> m_cameras;

		struct Vertex
		{
			Vector3 position;
			Vector4 color;
		};

        [[nodiscard]] const RHI::ConstantBuffer* GetConstantBuffer() const { return m_constantBuffer; };
        [[nodiscard]] RHI::DescriptorHeapHandle GetConstantBufferViewHandle() const { return m_constantBuffer->GetDescriptorHeapHandle(); };

	private:
		void UpdateConstantBufferData(const CameraMatrices& cameraMatrices);

		RHI::ConstantBuffer* m_constantBuffer;
	};
}
