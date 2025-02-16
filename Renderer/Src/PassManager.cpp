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

void PassManager::RecordPasses(MeshComponent& mesh, CameraComponent& camera, uint32_t meshIdx)
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Reserve CBV descriptor handle and fill it with camera information (constant across frame)
	m_basePass.PrepareDescriptorTables(renderContext, camera);

	// Record all the commands we need to render the scene into the command list.
	m_basePass.PopulateCommandList(renderContext, mesh, meshIdx);
}

void PassManager::BeginPasses()
{
	m_basePass.BeginPass(gRenderer->GetCommandContext());
}
