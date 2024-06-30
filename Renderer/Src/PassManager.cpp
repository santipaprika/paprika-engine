#include <windows.h>
#include <Logger.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Passes/BasePass.h>


using namespace PPK;

PassManager::PassManager()
{
	AddPasses();
}

void PassManager::AddPasses()
{
	Logger::Info("Adding passes...");

	m_basePass = BasePass(); // No need to do explicitly, but useful for debugging.
	// ... more passes here ...

	Logger::Info("Passes added successfully!");
}

void PassManager::RecordPasses(Mesh& mesh, Camera& camera)
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Record all the commands we need to render the scene into the command list.
	m_basePass.PopulateCommandList(renderContext, mesh, camera);

}
