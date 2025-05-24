#pragma once

#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>
#include <unordered_map>

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
		void RecordPasses(MeshComponent& mesh, CameraComponent& camera, uint32_t meshIdx, RHI::GPUResource* TLAS);
		void RecordPPFXPasses();
		void BeginPasses();

		BasePass m_basePass;
		DenoisePPFXPass m_denoisePpfxPass;
	};

	extern PassManager* gPassManager;
}
