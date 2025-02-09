#pragma once

#include <SimpleMath.h>
#include <RHI/ConstantBuffer.h>
// #include <TransformComponent.h>

#define WIDTH 1280
#define HEIGHT 720
#define ASPECT_RATIO (float(WIDTH) / float(HEIGHT))

using namespace DirectX::SimpleMath;

namespace PPK
{
	struct RenderContext;

	// Low level interpretation of camera which interacts with RHI and sets up the required buffers
	class CameraComponent
	{
	public:
		struct CameraInternals
		{
			float m_near = 0.001f;
			float m_far = 100.f;
			float m_fov = DirectX::XMConvertToRadians(60.f);
			float m_aspectRatio = ASPECT_RATIO;
		};

		// TODO: Simplify to camera internals
		struct CameraDescriptor
		{
			CameraInternals m_cameraInternals;
			CameraDescriptor() = default;
		};

		struct CameraMatrices
		{
			Matrix m_worldToView;
			Matrix m_viewToWorld;
			Matrix m_viewToClip;
		};

		CameraComponent() = default;
        explicit CameraComponent(uint32_t cameraIdx);
		// ~CameraComponent();

		// void CreateCameraConstantBuffer();
		// void UpdateCameraMatrices(const CameraDescriptor& cameraDescriptor);

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

        [[nodiscard]] RHI::ConstantBuffer& GetConstantBuffer() { return m_constantBuffer; };
        [[nodiscard]] RHI::DescriptorHeapHandle GetConstantBufferViewHandle() const { return static_cast<RHI::DescriptorHeapHandle>(*m_constantBuffer.GetDescriptorHeapElement().get()); };

		float m_speed;
		float m_sensitivity;
		
	private:
		// void UpdateConstantBufferData(const CameraMatrices& cameraMatrices);

		RHI::ConstantBuffer m_constantBuffer;
	};
}
