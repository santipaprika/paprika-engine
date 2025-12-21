#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/BasePass.h>
#include <CameraComponent.h>
#include <MeshComponent.h>
#include <Passes/ShadowVariancePass.h>


namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/BasePassVS.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"Shaders/BasePassPS.hlsl";

	BasePass::BasePass(const wchar_t* name)
		: Pass(name), m_noiseTextureIndex(INVALID_INDEX), m_shadowVarianceTargetIndex(INVALID_INDEX), m_numSamples(1)
	{
		m_basePassData.reserve(64); // 4 objects expected. If heavier scenes are added, increase this.
		BasePass::InitPass();
	}

	void BasePass::CreatePSO()
	{
		IDxcBlob* vsCode;
		gRenderer->CompileShader(vertexShaderPath, L"MainVS", L"vs_6_6", &vsCode, m_pipelineState == nullptr);
		IDxcBlob* psCode;
		gRenderer->CompileShader(pixelShaderPath, L"MainPS", L"ps_6_6", &psCode, m_pipelineState == nullptr);


		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS.BytecodeLength = vsCode->GetBufferSize();
		psoDesc.VS.pShaderBytecode = vsCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = psCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = psCode->GetBufferPointer();
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;

		ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		NAME_D3D12_OBJECT_CUSTOM(pso, L"BasePassPSO");

		ReloadPSO(pso);
	}

	constexpr float g_shadowsClearValue[] = { 0.f };

	void BasePass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(9, 0, 0); // 9 constants at b0

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[2];
			staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
			staticSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "BasePassRS");
		}

		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		msaaLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		msaaLevels.SampleCount = gMSAA ? gMSAACount : 1;
		msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		textureDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		textureDesc.SampleDesc.Quality = gMSAA ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality
		textureDesc.MipLevels = 1;

		m_renderTarget = RHI::CreateTextureResource(textureDesc, "RT_BasePass_MS");

		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		m_rayTracedShadowsTarget = RHI::CreateTextureResource(textureDesc, "RT_RayTracedShadowsRT", nullptr, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8_UNORM, g_shadowsClearValue));

		textureDesc.Format = m_renderTarget->GetResource()->GetDesc().Format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		m_resolvedRenderTarget = RHI::CreateTextureResource(textureDesc, "RT_BasePass_Resolved");

		{
			m_noiseTexture = GetGlobalGPUResource("Noise");

			for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
			{
				m_noiseTextureIndex = m_noiseTexture->GetIndexInRDH(RHI::EResourceViewType::SRV);
				m_shadowVarianceTarget = GetGlobalGPUResource("RT_ShadowVariancePass_MS");
				m_shadowVarianceTargetIndex = m_shadowVarianceTarget->GetIndexInRDH(RHI::EResourceViewType::SRV);
			}
		}

		CreatePSO();
	}

	void BasePass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0x00), L"Begin Base Pass");

		{
			SCOPED_TIMER("BasePass::BeginPass::1_TransitionAndClearResources")
			
			// Record commands.
			gRenderer->TransitionResources(commandList, {
				{ m_renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET },
				{ m_rayTracedShadowsTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET },
				{ m_depthTarget, D3D12_RESOURCE_STATE_DEPTH_READ },
				{ m_noiseTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
				{ m_shadowVarianceTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
			});

			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = {
				m_renderTarget->GetDescriptorHeapHandle(RHI::EResourceViewType::RTV).GetCPUHandle(),
				m_rayTracedShadowsTarget->GetDescriptorHeapHandle(RHI::EResourceViewType::RTV).GetCPUHandle(),
			};
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapHandle(RHI::EResourceViewType::DSV).GetCPUHandle();
			commandList->ClearRenderTargetView(rtvHandles[0], PPK::g_clearColor, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandles[1], PPK::g_shadowsClearValue, 0, nullptr);

			commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &dsvHandle);
		}

		{
			SCOPED_TIMER("BasePass::BeginPass::2_SetPSO_RS")
			
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetGraphicsRootSignature(m_rootSignature.Get());

			const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
			commandList->RSSetViewports(1, &viewport);
			const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
			commandList->RSSetScissorRects(1, &scissorRect);
		}

		{
			SCOPED_TIMER("BasePass::BeginPass::3_SetPerPassDescriptorTables")
			
			commandList->SetGraphicsRoot32BitConstant(0, sceneRenderContext.m_mainCameraRdhIndex, 2); // Per View

			// Per Pass
			commandList->SetGraphicsRoot32BitConstant(0, gSmartSampleAllocation, 4);
			commandList->SetGraphicsRoot32BitConstant(0, m_noiseTextureIndex, 5);
			commandList->SetGraphicsRoot32BitConstant(0, m_shadowVarianceTargetIndex, 6);
			
			commandList->SetGraphicsRoot32BitConstant(0, sceneRenderContext.m_lightsRdhIndex, 8); // Per Scene
		}
	}

	void BasePass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		SCOPED_TIMER("BasePass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0x00), L"Base Pass");

		float time = Timer::GetApplicationTimeInSeconds();

		uint32_t frameIdx = context->GetFrameIndex();

		// Fill root parameters
		commandList->SetGraphicsRoot32BitConstant(0, gTotalFrameIndex, 0);
		commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&m_numSamples), 1);

		for (const BasePassData& basePassData : m_basePassData)
		{
			BYTE colorIntensity = std::min(1.f, basePassData.m_indexCount / 50000.f) * 0xff; 
			PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, colorIntensity, 0x00), basePassData.m_name);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &basePassData.m_vertexBufferView);
			commandList->IASetIndexBuffer(&basePassData.m_indexBufferView);

			commandList->SetGraphicsRoot32BitConstant(0, basePassData.m_objectRdhIndex, 3);
			commandList->SetGraphicsRoot32BitConstant(0, basePassData.m_materialRdhIndex, 7);
			commandList->DrawIndexedInstanced(basePassData.m_indexCount, 1, 0, 0, 0);
		}

		// End pass
		SignalPSOFence();

		// Record commands.
		gRenderer->TransitionResources(commandList, {
			{ m_renderTarget.get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE },
			{ m_resolvedRenderTarget.get(), D3D12_RESOURCE_STATE_RESOLVE_DEST }
		});

		// Resolve color
		commandList->ResolveSubresource(m_resolvedRenderTarget->GetResource().Get(), 0,
			m_renderTarget->GetResource().Get(), 0, m_renderTarget->GetResource()->GetDesc().Format);

		// TODO: Resolve Raytraced shadows MS target (or do big refactor to move to an earlier pass)
	}

	void BasePass::AddBasePassRun(const BasePassData& basePassData)
	{
		m_basePassData.push_back(basePassData);
	}
}
