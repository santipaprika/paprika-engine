#include <ApplicationHelper.h>
#include <CameraComponent.h>
#include <dxcapi.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Timer.h>

#include <Passes/ShadowRayTracingPass.h>

namespace PPK
{
	constexpr const wchar_t* computeShaderPath = L"Shaders/ShadowRayTracingCS.hlsl";

	ShadowRayTracingPass::ShadowRayTracingPass(const wchar_t* name)
		: Pass(name)
	{
		ShadowRayTracingPass::InitPass();
	}

	void ShadowRayTracingPass::CreatePSO()
	{
		IDxcBlob* csCode;
		if (!gRenderer->CompileShader(computeShaderPath, L"MainCS", L"cs_6_8", &csCode, !!m_pipelineState))
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
		NAME_D3D12_OBJECT_CUSTOM(pso, L"ShadowRayTracingPassPSO");

		ReloadPSO(pso);
	}

	constexpr float g_shadowsClearValue[] = { 0.f };

	void ShadowRayTracingPass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(7, 0, 0); // 2 constants at b0

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), {},
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "ShadowRayTracingPassRS");
		}

		m_shadowSampleScatterBuffer = GetGlobalGPUResource("ShadowSamples_ScatterBuffer");
		m_shadowRayTracingCommandBuffer = GetGlobalGPUResource("CommandBuffer_ShadowRayTracing");
		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
		m_noiseTexture = GetGlobalGPUResource("Noise");

		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 1;
		textureDesc.MipLevels = 1;
		
		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		m_rayTracedShadowsTarget = RHI::CreateTextureResource(textureDesc, "RT_RayTracedShadows", nullptr, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8_UNORM, g_shadowsClearValue));

		CreatePSO();

		// Create the command signature used for indirect drawing.
		{
			// Each command consists of a CBV update and a DrawInstanced call.
			D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[1] = {};
			argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

			// Data structure to match the command signature used for ExecuteIndirect.
			

			D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
			commandSignatureDesc.pArgumentDescs = argumentDescs;
			commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
			commandSignatureDesc.ByteStride = sizeof(IndirectCommand);

			ThrowIfFailed(gDevice->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&m_commandSignature)));
			NAME_D3D12_OBJECT_CUSTOM(m_commandSignature, L"CommandSignature_ShadowRayTracing");
		}
	}

	void ShadowRayTracingPass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("ShadowRayTracingPass::BeginPass")

		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0xaf, 0x44, 0xfa), L"Begin Shadow Ray Tracing Pass");

		gRenderer->TransitionResources(commandList, {
			{ m_shadowSampleScatterBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
			{ m_shadowRayTracingCommandBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT},
			{ m_depthTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE},
			{ m_rayTracedShadowsTarget.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS }
		});

		// Set necessary state.
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetComputeRootSignature(m_rootSignature.Get());

		// Fill root parameters
		commandList->SetComputeRoot32BitConstant(0, gTotalFrameIndex, 0);
		commandList->SetComputeRoot32BitConstant(0, sceneRenderContext.m_mainCameraRdhIndex, 1);
		commandList->SetComputeRoot32BitConstant(0, m_noiseTexture->GetIndexInRDH(RHI::EResourceViewType::SRV), 2);
		commandList->SetComputeRoot32BitConstant(0, sceneRenderContext.m_lightsRdhIndex, 3);
		commandList->SetComputeRoot32BitConstant(0, m_depthTarget->GetIndexInRDH(RHI::EResourceViewType::SRV), 4);
		commandList->SetComputeRoot32BitConstant(0, m_shadowSampleScatterBuffer->GetIndexInRDH(RHI::EResourceViewType::SRV), 5);
		commandList->SetComputeRoot32BitConstant(0, m_rayTracedShadowsTarget->GetIndexInRDH(RHI::EResourceViewType::UAV), 6);
		
	}

	void ShadowRayTracingPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("ShadowRayTracingPass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0xaf, 0x44, 0xfa), L"Begin Shadow Ray Tracing Pass");

		commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_shadowRayTracingCommandBuffer->GetResource().Get(), 0, nullptr, 0);

		// End pass
		SignalPSOFence();
	}
}
