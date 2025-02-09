#pragma once

#include <Passes/BasePass.h>

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
		void RecordPasses(MeshComponent& mesh, CameraComponent& camera);

	private:
		BasePass m_basePass;
	};
}