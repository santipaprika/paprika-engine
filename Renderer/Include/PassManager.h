#pragma once

#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>
#include <Passes/DepthPass.h>

namespace PPK
{
	class CameraEntity;
	class MeshEntity;
	class Renderer;
	class PassManager
	{
	public:
		PassManager();

		void RecordPasses();

		// Pass declaration needs to be in the right dependency order! TODO: Maybe find a less-dangerous way?
		DepthPass m_depthPass;
		BasePass m_basePass;
		DenoisePPFXPass m_denoisePpfxPass;
	};

	extern PassManager* gPassManager;
}
