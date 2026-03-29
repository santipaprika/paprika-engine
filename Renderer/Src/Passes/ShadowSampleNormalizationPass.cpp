#include <ApplicationHelper.h>
#include <CameraComponent.h>
#include <dxcapi.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/ShadowSampleNormalizationPass.h>

namespace PPK
{
	constexpr const wchar_t* computeShaderPath = L"Shaders/ShadowSampleNormalizationCS.hlsl";

	ShadowSampleNormalizationPass::ShadowSampleNormalizationPass(const wchar_t* name)
		: Pass(name), m_numSamples(1)
	{
		ShadowSampleNormalizationPass::CreatePSO();
		ShadowSampleNormalizationPass::CreatePassResources();
	}

	void ShadowSampleNormalizationPass::CreatePSO()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(3, 0, 0); // 2 constants at b0

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), {},
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "ShadowSampleNormalizationPassRS");
		}

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
		NAME_D3D12_OBJECT_CUSTOM(pso, L"ShadowSampleNormalizationPassPSO");

		ReloadPSO(pso);
	}

	void ShadowSampleNormalizationPass::InitPassParams()
	{
		m_shadowSampleScatterBuffer = GetGlobalGPUResource("ShadowSamples_ScatterBuffer");
		m_shadowRayTracingCommandBuffer = GetGlobalGPUResource("CommandBuffer_ShadowRayTracing");
	}

	void ShadowSampleNormalizationPass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		if (!gSmartSampleAllocation || m_numSamples == 0)
		{
			return;
		}

		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0xfa), L"Begin Shadow Variance Pass");

		{
			// UAV barriers needed because we write to resources UAV in the last pass (ClearBuffer) and this one.
			SCOPED_TIMER("ShadowSampleNormalizationPass::BeginPass::1_TransitionAndClearResources")
			gRenderer->TransitionResources(commandList, {
				{ m_shadowSampleScatterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
				{ m_shadowRayTracingCommandBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE}
			}, {m_shadowSampleScatterBuffer});
		}

		{
			SCOPED_TIMER("ShadowSampleNormalizationPass::BeginPass::2_SetPSO_RS")
			
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetComputeRootSignature(m_rootSignature.Get());
		}

		{
			SCOPED_TIMER("ShadowSampleNormalizationPass::BeginPass::3_SetPerPassDescriptorTables")
			
			// Fill root parameters
			commandList->SetComputeRoot32BitConstant(0, m_shadowSampleScatterBuffer->GetIndexInRDH(RHI::EResourceViewType::UAV), 0);
			commandList->SetComputeRoot32BitConstant(0, m_shadowRayTracingCommandBuffer->GetIndexInRDH(RHI::EResourceViewType::SRV), 1);
			commandList->SetComputeRoot32BitConstant(0, *reinterpret_cast<UINT*>(&m_numSamples), 2);
		}
	}

	void ShadowSampleNormalizationPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		if (!gSmartSampleAllocation || m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("ShadowSampleNormalizationPass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0xff), L"Shadow Variance Pass");

		commandList->Dispatch(gRenderer->GetWidth() * gRenderer->GetHeight() / (8 * 8), 1, 1);

		// End pass
		SignalPSOFence();
	}
}
