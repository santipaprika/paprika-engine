#include <windows.h>
#include <Logger.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Passes/BasePass.h>
#include <Passes/DenoisePPFXPass.h>

#include <EntityUtils.h>

using namespace PPK;

PassManager* PPK::gPassManager;

PassManager::PassManager() :
	m_depthPass(DepthPass(L"DepthPass")), m_customClearBuffersPass(CustomClearBuffersPass(L"CustomClearBuffersPass")),
	m_shadowVariancePass(ShadowVariancePass(L"ShadowVariancePass")), m_shadowSampleNormalizationPass(L"ShadowSampleNormalizationPass"),
	m_shadowRayTracingPass(ShadowRayTracingPass(L"ShadowRayTracingPass")), m_basePass(BasePass(L"BasePass")),
	m_denoisePpfxPass(DenoisePPFXPass(L"DenoisePPFXPass"))
{
	InitAllPassParams();	
}

void PassManager::RecordPasses(const SceneRenderContext sceneRenderContext)
{
	std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

	// Record all the commands we need to render the scene into the command list.
	m_depthPass.BeginPass(renderContext, sceneRenderContext);
	m_depthPass.PopulateCommandList(renderContext);

	m_customClearBuffersPass.BeginPass(renderContext, sceneRenderContext);
	m_customClearBuffersPass.PopulateCommandList(renderContext);
	
	m_shadowVariancePass.BeginPass(renderContext, sceneRenderContext);
	m_shadowVariancePass.PopulateCommandList(renderContext);

	m_shadowSampleNormalizationPass.BeginPass(renderContext, sceneRenderContext);
	m_shadowSampleNormalizationPass.PopulateCommandList(renderContext);

	m_shadowRayTracingPass.BeginPass(renderContext, sceneRenderContext);
	m_shadowRayTracingPass.PopulateCommandList(renderContext);

	m_basePass.BeginPass(renderContext, sceneRenderContext);
	m_basePass.PopulateCommandList(renderContext);

	// m_denoisePpfxPass.BeginPass(gRenderer->GetCommandContext(), sceneRenderContext);
	// m_denoisePpfxPass.PopulateCommandListPPFX(renderContext);

	// ... other passes here ...
}

void PassManager::RecompileShaders()
{
	m_depthPass.CreatePSO();
	m_customClearBuffersPass.CreatePSO();
	m_shadowVariancePass.CreatePSO();
	m_shadowSampleNormalizationPass.CreatePSO();
	m_shadowRayTracingPass.CreatePSO();
	m_basePass.CreatePSO();
	m_denoisePpfxPass.CreatePSO();
}

void PassManager::OnResizeWindow()
{
    gRenderer->GetCurrentCommandListReset();

	// GPU should have flushed all work by the time it gets here
	if (m_depthPass.m_bIsScreenSizeDependent)
	{
		m_depthPass.DestroyPassResources();
		m_depthPass.CreatePassResources();
	}
	if (m_customClearBuffersPass.m_bIsScreenSizeDependent)
	{
		m_customClearBuffersPass.DestroyPassResources();
		m_customClearBuffersPass.CreatePassResources();
	}
	if (m_shadowVariancePass.m_bIsScreenSizeDependent)
	{
		m_shadowVariancePass.DestroyPassResources();
		m_shadowVariancePass.CreatePassResources();
	}
	if (m_shadowSampleNormalizationPass.m_bIsScreenSizeDependent)
	{
		m_shadowSampleNormalizationPass.DestroyPassResources();
		m_shadowSampleNormalizationPass.CreatePassResources();
	}
	if (m_shadowRayTracingPass.m_bIsScreenSizeDependent)
	{
		m_shadowRayTracingPass.DestroyPassResources();
		m_shadowRayTracingPass.CreatePassResources();
	}
	if (m_basePass.m_bIsScreenSizeDependent)
	{
		m_basePass.DestroyPassResources();
		m_basePass.CreatePassResources();
	}
	if (m_denoisePpfxPass.m_bIsScreenSizeDependent)
	{
		m_denoisePpfxPass.DestroyPassResources();
		m_denoisePpfxPass.CreatePassResources();
	}

	InitAllPassParams();

    gRenderer->GetCommandContext()->GetCurrentCommandList()->Close();
	gRenderer->ExecuteCommandListOnce();
}


void PassManager::InitAllPassParams()
{
	m_depthPass.InitPassParams();
	m_customClearBuffersPass.InitPassParams();
	m_shadowVariancePass.InitPassParams();
	m_shadowSampleNormalizationPass.InitPassParams();
	m_shadowRayTracingPass.InitPassParams();
	m_basePass.InitPassParams();
	m_denoisePpfxPass.InitPassParams();
}