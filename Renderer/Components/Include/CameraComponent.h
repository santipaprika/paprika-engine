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

		struct Vertex
		{
			Vector3 position;
			Vector4 color;
		};

        [[nodiscard]] RHI::ConstantBuffer& GetConstantBuffer() { return m_constantBuffer; };
        [[nodiscard]] RHI::DescriptorHeapHandle GetConstantBufferViewHandle() const { return static_cast<RHI::DescriptorHeapHandle>(*m_constantBuffer.GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).get()); };

		void InitScenePassData();

		float m_speed;
		float m_sensitivity;
		
	private:
		RHI::ConstantBuffer m_constantBuffer;
	};
}
