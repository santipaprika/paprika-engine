#pragma once

#include <SimpleMath.h>
#include <Passes/Pass.h>
#include <RHI/ConstantBuffer.h>

#define VIEWPORT_WIDTH 1920
#define VIEWPORT_HEIGHT 1080
#define ASPECT_RATIO (float(VIEWPORT_WIDTH) / float(VIEWPORT_HEIGHT))

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

        [[nodiscard]] RHI::ConstantBuffer& GetConstantBuffer(uint32_t frameIndex) { return m_constantBuffer[frameIndex]; }

		void InitScenePassData();

		float m_speed;
		float m_sensitivity;
		bool m_dirtyRenderState[gFrameCount];
        CameraInternals m_cameraInternals;
		
	private:
		RHI::ConstantBuffer m_constantBuffer[gFrameCount];
	};
}
