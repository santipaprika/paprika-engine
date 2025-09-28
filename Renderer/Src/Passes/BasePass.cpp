#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/BasePass.h>
#include <CameraComponent.h>
#include <MeshComponent.h>


namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/BasePassVS.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"Shaders/BasePassPS.hlsl";

	BasePass::BasePass(const wchar_t* name)
		: Pass(name),
		  m_numSamples(1)
	{
		m_basePassData.reserve(64); // 4 objects expected. If heavier scenes are added, increase this.
		BasePass::InitPass();
	}

	constexpr float g_shadowsClearValue[] = { 0.f };

	void BasePass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(4, 0, 0); // 3 constant at b0-b2

			// Per scene (BLAS...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerScene[1];
			CD3DX12_ROOT_PARAMETER1 perSceneRP;
			DescRangePerScene[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // BLAS/Lights - t0-t1
			perSceneRP.InitAsDescriptorTable(1, &DescRangePerScene[0]); // 1 ranges t0

			// Per view (Camera...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerView[1];
			CD3DX12_ROOT_PARAMETER1 perViewRP;
			DescRangePerView[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // CamTransform - b1
			perViewRP.InitAsDescriptorTable(1, &DescRangePerView[0]); // 1 ranges b1

			// Per pass (noise texture, common textures...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerPass[1];
			CD3DX12_ROOT_PARAMETER1 perPassRP;
			DescRangePerPass[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Noise texture - t1-t2
			perPassRP.InitAsDescriptorTable(1, &DescRangePerPass[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges b1

			// Per object (transform...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerObject[1];
			CD3DX12_ROOT_PARAMETER1 perObjectRP;
			DescRangePerObject[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Mesh transform - b2
			perObjectRP.InitAsDescriptorTable(1, &DescRangePerObject[0]); // 1 ranges b2

			// Per material (pbr textures...)
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerMaterial[1];
			CD3DX12_ROOT_PARAMETER1 perMaterialRP;
			DescRangePerMaterial[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Material texture - t3
			perMaterialRP.InitAsDescriptorTable(1, &DescRangePerMaterial[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges t1

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[2];
			staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
			staticSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants, perSceneRP, perViewRP, perPassRP, perObjectRP, perMaterialRP };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "BasePassRS");
		}

		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

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
				RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);

				// Copy per-pass textures in contiguous memory in heap
				D3D12_GPU_DESCRIPTOR_HANDLE heapStartHandle = cbvSrvHeap->CopyDescriptors(m_noiseTexture, RHI::HeapLocation::TEXTURES);
				m_shadowVarianceTarget = GetGlobalGPUResource("RT_ShadowVariancePass_Resolved");
				m_shadowVarianceTargetHandle = cbvSrvHeap->CopyDescriptors(m_shadowVarianceTarget, RHI::HeapLocation::TEXTURES);
			
				m_perPassHeapHandle[frameIdx] = heapStartHandle;
			}
		}

		IDxcBlob* vsCode;
		gRenderer->CompileShader(vertexShaderPath, L"MainVS", L"vs_6_6", &vsCode);
		IDxcBlob* psCode;
		gRenderer->CompileShader(pixelShaderPath, L"MainPS", L"ps_6_6", &psCode);


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
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT_CUSTOM(m_pipelineState, L"BasePassPSO");
	}

	void BasePass::BeginPass(std::shared_ptr<RHI::CommandContext> context, uint32_t cameraRdhIndex)
	{
		Pass::BeginPass(context, cameraRdhIndex);

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
				m_renderTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandle(),
				m_rayTracedShadowsTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandle(),
			};
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->GetCPUHandle();
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
			
			commandList->SetGraphicsRoot32BitConstant(0, cameraRdhIndex, 2); // Per View
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::TLAS)); // Per scene
			commandList->SetGraphicsRootDescriptorTable(3, m_perPassHeapHandle[frameIdx]); // Per Pass
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

			commandList->SetGraphicsRoot32BitConstant(0, basePassData.m_objectRdhIndex, 3); // Per View
			// commandList->SetGraphicsRootDescriptorTable(4, basePassData.m_objectHandle[frameIdx]); // Per object
			commandList->SetGraphicsRootDescriptorTable(5, basePassData.m_materialHandle[frameIdx]); // Per material
			commandList->DrawIndexedInstanced(basePassData.m_indexCount, 1, 0, 0, 0);
		}

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
