#pragma once

#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>

namespace PPK
{
	class CameraEntity;
	class MeshEntity;
	class Renderer;
	class PassManager
	{
	public:
		PassManager();

		void AddPasses();
		void RecordPasses();
		void RecordPPFXPasses();
		void BeginPasses();

		BasePass m_basePass;
		DenoisePPFXPass m_denoisePpfxPass;
	};

	extern PassManager* gPassManager;
}
