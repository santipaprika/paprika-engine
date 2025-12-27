#include <ApplicationHelper.h>
#include <CameraComponent.h>
#include <dxcapi.h>
#include <PassManager.h>
#include <Renderer.h>
#include <Timer.h>

#include <Passes/CustomClearBuffersPass.h>

namespace PPK
{
	constexpr const wchar_t* computeShaderPath = L"Shaders/CustomClearBuffersCS.hlsl";

	CustomClearBuffersPass::CustomClearBuffersPass(const wchar_t* name)
		: Pass(name)
	{
		CustomClearBuffersPass::InitPass();
	}

	void CustomClearBuffersPass::CreatePSO()
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
		NAME_D3D12_OBJECT_CUSTOM(pso, L"CustomClearBuffersPassPSO");

		ReloadPSO(pso);
	}

	void CustomClearBuffersPass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(1, 0, 0); // 6 constants at b0

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), {},
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "CustomClearBuffersPassRS");
		}

		m_shadowSampleScatterBuffer = GetGlobalGPUResource("ShadowSamples_ScatterBuffer");
		
		CreatePSO();
	}

	void CustomClearBuffersPass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("CustomClearBuffersPass::BeginPass")

		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0xfa), L"Begin Custom Clear Buffers Pass");

		gRenderer->TransitionResources(commandList, {
			{ m_shadowSampleScatterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS},
		});


		// Set necessary state.
		commandList->SetPipelineState(m_pipelineState.Get());
		commandList->SetComputeRootSignature(m_rootSignature.Get());

		// Fill root parameters
		commandList->SetComputeRoot32BitConstant(0, m_shadowSampleScatterBuffer->GetIndexInRDH(RHI::EResourceViewType::UAV), 0);
	}

	void CustomClearBuffersPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		if (!gSmartSampleAllocation || gPassManager->m_basePass.m_numSamples == 0)
		{
			return;
		}

		SCOPED_TIMER("CustomClearBuffersPass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0x55), L"Custom Clear Buffer Pass");

		commandList->Dispatch(1, 1, 1);

		// End pass
		SignalPSOFence();
	}
}
