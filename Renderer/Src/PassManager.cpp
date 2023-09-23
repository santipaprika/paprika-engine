#include <PassManager.h>
#include <Renderer.h>
#include <Mesh.h>
#include <Camera.h>


using namespace PPK;

PassManager::PassManager(std::shared_ptr<Renderer> renderer)
	: m_renderer(renderer)
{
	AddPasses();
}

void PassManager::AddPasses()
{
	Logger::Info("Adding passes...");

	m_passes.push_back(Pass(DX12Interface::Get()->GetDevice()));
	// ... more passes here ...

	Logger::Info("Passes added successfully!");
}

void PassManager::AddPass(Pass* pass)
{
	m_passes.push_back(*pass);
}

void PassManager::RecordPasses(Mesh& mesh, Camera& camera)
{
	std::shared_ptr<RHI::CommandContext> renderContext = m_renderer->GetCommandContext();

	for (Pass& pass : m_passes)
	{
		// Record all the commands we need to render the scene into the command list.
		pass.PopulateCommandList(renderContext, *m_renderer.get(), mesh, camera);
	}
}
