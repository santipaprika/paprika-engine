#pragma once

#include <stdafx_renderer.h>
#include <Passes/Pass.h>

namespace PPK
{
	class CameraEntity;
	class MeshEntity;
	class Renderer;
	class PassManager
	{
	public:
		PassManager(std::shared_ptr<Renderer> m_renderer);

		void AddPasses();
		void AddPass(Pass* pass);
		void RecordPasses(Mesh& mesh, Camera& camera);

	private:
		// Passes in execution order
		std::vector<Pass> m_passes;
		std::shared_ptr<Renderer> m_renderer;
	};
}