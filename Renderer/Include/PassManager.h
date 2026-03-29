#pragma once

#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>
#include <Passes/DepthPass.h>
#include <Passes/ShadowVariancePass.h>
#include <Passes/CustomClearBuffersPass.h>
#include <Passes/ShadowRayTracingPass.h>
#include <Passes/ShadowSampleNormalizationPass.h>

namespace PPK
{
	class CameraEntity;
	class MeshEntity;
	class Renderer;
	class PassManager
	{
	public:
		PassManager();

		void RecordPasses(SceneRenderContext sceneRenderContext);
		void RecompileShaders();
		void OnResizeWindow();

		DepthPass m_depthPass;
		ShadowVariancePass m_shadowVariancePass;
		ShadowSampleNormalizationPass m_shadowSampleNormalizationPass;
		ShadowRayTracingPass m_shadowRayTracingPass;
		CustomClearBuffersPass m_customClearBuffersPass;
		BasePass m_basePass;
		DenoisePPFXPass m_denoisePpfxPass;

	private:
		void InitAllPassParams();
	};

	extern PassManager* gPassManager;
}
