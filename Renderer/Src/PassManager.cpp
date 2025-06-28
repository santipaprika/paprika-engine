#include <windows.h>
#include <Logger.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>


using namespace PPK;

PassManager* PPK::gPassManager;

PassManager::PassManager()
{
	AddPasses();
}

void PassManager::AddPasses()
{
	Logger::Info("Adding passes...");

	m_basePass = BasePass(L"BasePass"); // No need to do explicitly, but useful for debugging.
	m_denoisePpfxPass = DenoisePPFXPass(L"DenoisePPFXPass");
	// ... more passes here ...

	Logger::Info("Passes added successfully!");
}

void PassManager::RecordPasses()
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Record all the commands we need to render the scene into the command list.
	m_basePass.PopulateCommandList(renderContext);

	// ... other passes here ...
}

void PassManager::RecordPPFXPasses()
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Record all the commands we need to render the scene into the command list.
	m_denoisePpfxPass.PopulateCommandListPPFX(renderContext);

	// ... other ppfx passes here
}

// unused atm
void PassManager::BeginPasses()
{
	m_basePass.BeginPass(gRenderer->GetCommandContext());
	m_denoisePpfxPass.BeginPass(gRenderer->GetCommandContext());
}
