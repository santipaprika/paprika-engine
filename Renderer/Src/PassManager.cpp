#include <windows.h>
#include <Logger.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>


using namespace PPK;

PassManager* PPK::gPassManager;

PassManager::PassManager() :
	m_depthPass(DepthPass(L"DepthPass")), m_shadowVariancePass(ShadowVariancePass(L"ShadowVariancePass")),
	m_basePass(BasePass(L"BasePass")), m_denoisePpfxPass(DenoisePPFXPass(L"DenoisePPFXPass"))
{
}

void PassManager::RecordPasses()
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Record all the commands we need to render the scene into the command list.
	m_depthPass.BeginPass(renderContext);
	m_depthPass.PopulateCommandList(renderContext);
	
	m_shadowVariancePass.BeginPass(renderContext);
	m_shadowVariancePass.PopulateCommandList(renderContext);

	m_basePass.BeginPass(renderContext);
	m_basePass.PopulateCommandList(renderContext);

	gPassManager->m_denoisePpfxPass.BeginPass(gRenderer->GetCommandContext());
	m_denoisePpfxPass.PopulateCommandListPPFX(renderContext);

	// ... other passes here ...
}
