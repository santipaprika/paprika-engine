#include <ApplicationHelper.h>
#include <CameraComponent.h>
#include <dxcapi.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Timer.h>

#include <Passes/ShadowVariancePass.h>

namespace PPK
{
	constexpr const wchar_t* computeShaderPath = L"Shaders/ShadowVarianceCS.hlsl";

	ShadowVariancePass::ShadowVariancePass(const wchar_t* name)
		: Pass(name), m_noiseTextureIndex(INVALID_INDEX)
	{
		m_shadowVariancePassData.reserve(64); // 4 objects expected. If heavier scenes are added, increase this.
		ShadowVariancePass::InitPass();
	}

	void ShadowVariancePass::CreatePSO()
	{
		IDxcBlob* csCode;
		if (!gRenderer->CompileShader(computeShaderPath, L"MainCS", L"cs_6_6", &csCode, !!m_pipelineState))
		{
			return;
		}

		// Describe and create the compute pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.CS.BytecodeLength = csCode->GetBufferSize();
		psoDesc.CS.pShaderBytecode = csCode->GetBufferPointer();
		psoDesc.pRootSignature = m_rootSignature.Get();

		ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(gDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		NAME_D3D12_OBJECT_CUSTOM(pso, L"ShadowVariancePassPSO");

		ReloadPSO(pso);
	}

	constexpr float g_shadowsClearValue[] = { 1.f };

	void ShadowVariancePass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(6, 0, 0); // 6 constants at b0

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1];
			staticSamplers[0].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "ShadowVariancePassRS");
		}

		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
		
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		// TODO: NO MSAA for Variance Target? Rename at least
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		msaaLevels.Format = DXGI_FORMAT_R8_UNORM; // Replace with your render target format.
		msaaLevels.SampleCount = gMSAA ? gMSAACount : 1; // Replace with your sample count.
		msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		textureDesc.SampleDesc.Count = false ? gMSAACount : 1;
		textureDesc.SampleDesc.Quality = false ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality
		textureDesc.MipLevels = 7;
		
		m_shadowVarianceTarget = RHI::CreateTextureResource(textureDesc, "RT_ShadowVariancePass_MS", nullptr, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8_UNORM, g_shadowsClearValue));
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;

		m_shadowVarianceTargetResolved = RHI::CreateTextureResource(textureDesc, "RT_ShadowVariancePass_Resolved", nullptr, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8_UNORM, g_shadowsClearValue));

		// TODO: Compare also with Variance from previous frame, which may help reduce spatial res requirement so maybe
		// we can trace 4x less rays (for 2x downsampled target) with similar results. 
		{
			// Create noise texture resource
			// Generated with https://github.com/electronicarts/fastnoise
			DirectX::ScratchImage scratchImage = LoadTextureFromDisk(GetAssetFullFilesystemPath("Textures/sphere_coshemi_binomial3x3_Gauss10_product_0.png"));
			// TODO: Handle mips/slices/depth
			m_noiseTexture = RHI::CreateTextureResource(scratchImage.GetMetadata(), "Noise", scratchImage.GetImage(0, 0, 0));

			for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
			{
				m_noiseTextureIndex = m_noiseTexture->GetIndexInRDH(RHI::EResourceViewType::SRV);
			}
		}

		CreatePSO();
	}

	void ShadowVariancePass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0xfa), L"Begin Shadow Variance Pass");

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::1_TransitionAndClearResources")
			gRenderer->TransitionResources(commandList, {
				{ m_shadowVarianceTarget.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
				{ m_depthTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
				{ m_noiseTexture.get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
			});
		}

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::2_SetPSO_RS")
			
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetComputeRootSignature(m_rootSignature.Get());
		}

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::3_SetPerPassDescriptorTables")
			
			// Fill root parameters
			commandList->SetComputeRoot32BitConstant(0, gTotalFrameIndex, 0);
			commandList->SetComputeRoot32BitConstant(0, sceneRenderContext.m_mainCameraRdhIndex, 1);
			commandList->SetComputeRoot32BitConstant(0, m_noiseTextureIndex, 2);
			commandList->SetComputeRoot32BitConstant(0, sceneRenderContext.m_lightsRdhIndex, 3);
			commandList->SetComputeRoot32BitConstant(0, m_depthTarget->GetIndexInRDH(RHI::EResourceViewType::SRV), 4);
			commandList->SetComputeRoot32BitConstant(0, m_shadowVarianceTarget->GetIndexInRDH(RHI::EResourceViewType::UAV), 5);
		}
	}

	void ShadowVariancePass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("ShadowVariancePass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0xff), L"Shadow Variance Pass");

		commandList->Dispatch(VIEWPORT_WIDTH / 8, VIEWPORT_HEIGHT / 8, 1);

		// End pass
		SignalPSOFence();
	}

	void ShadowVariancePass::AddShadowVariancePassRun(const ShadowVariancePassData& shadowVariancePassData)
	{
		m_shadowVariancePassData.push_back(shadowVariancePassData);
	}
}
