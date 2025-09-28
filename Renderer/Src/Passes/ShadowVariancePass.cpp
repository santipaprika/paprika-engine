#include <ApplicationHelper.h>
#include <CameraComponent.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>

#include <Passes/ShadowVariancePass.h>

namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/ShadowVarianceVS.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"Shaders/ShadowVariancePS.hlsl";

	ShadowVariancePass::ShadowVariancePass(const wchar_t* name)
		: Pass(name)
	{
		m_shadowVariancePassData.reserve(64); // 4 objects expected. If heavier scenes are added, increase this.
		ShadowVariancePass::InitPass();
	}

	constexpr float g_shadowsClearValue[] = { 1.f };

	void ShadowVariancePass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(3, 0, 0); // 3 constant at b0
			
			// Per scene (BLAS...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerScene[1];
			CD3DX12_ROOT_PARAMETER1 perSceneRP;
			DescRangePerScene[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // BLAS - t0
			perSceneRP.InitAsDescriptorTable(1, &DescRangePerScene[0]); // 1 ranges t0

			// Per view (Camera...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerView[1];
			CD3DX12_ROOT_PARAMETER1 perViewRP;
			DescRangePerView[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // CamTransform - b1
			perViewRP.InitAsDescriptorTable(1, &DescRangePerView[0]); // 1 ranges b1

			// Per pass (noise texture, common textures...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerPass[1];
			CD3DX12_ROOT_PARAMETER1 perPassRP;
			DescRangePerPass[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Noise texture - t1
			perPassRP.InitAsDescriptorTable(1, &DescRangePerPass[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges b1

			// Per object (transform...) TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangePerObject[1];
			CD3DX12_ROOT_PARAMETER1 perObjectRP;
			DescRangePerObject[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Mesh transform - b2
			perObjectRP.InitAsDescriptorTable(1, &DescRangePerObject[0]); // 1 ranges b2

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1];
			staticSamplers[0].Init(1, D3D12_FILTER_MIN_MAG_MIP_POINT);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants, perSceneRP, perViewRP, perPassRP, perObjectRP };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "ShadowVariancePassRS");
		}

		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
		
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		msaaLevels.Format = DXGI_FORMAT_R8_UNORM; // Replace with your render target format.
		msaaLevels.SampleCount = gMSAA ? gMSAACount : 1; // Replace with your sample count.
		msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		textureDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		textureDesc.SampleDesc.Quality = gMSAA ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality
		textureDesc.MipLevels = 1;
		
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
				RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
				m_noiseTextureHandle[frameIdx] = cbvSrvHeap->CopyDescriptors(m_noiseTexture.get(), RHI::HeapLocation::TEXTURES);
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
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
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
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT_CUSTOM(m_pipelineState, L"ShadowVariancePassPSO");
	}

	void ShadowVariancePass::BeginPass(std::shared_ptr<RHI::CommandContext> context, uint32_t cameraRdhIndex)
	{
		Pass::BeginPass(context, cameraRdhIndex);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0xfa), L"Begin Shadow Variance Pass");

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::1_TransitionAndClearResources")
			
			// Record commands.
			gRenderer->TransitionResources(commandList, {
				{ m_shadowVarianceTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET },
				{ m_depthTarget, D3D12_RESOURCE_STATE_DEPTH_READ },
				{ m_noiseTexture.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
			});
			
			
			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = {
				m_shadowVarianceTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandle()
			};
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->GetCPUHandle();
			commandList->ClearRenderTargetView(rtvHandles[0], PPK::g_shadowsClearValue, 0, nullptr);

			commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &dsvHandle);
		}

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::2_SetPSO_RS")
			
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetGraphicsRootSignature(m_rootSignature.Get());

			const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
			commandList->RSSetViewports(1, &viewport);
			const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
			commandList->RSSetScissorRects(1, &scissorRect);
		}

		{
			SCOPED_TIMER("ShadowVariancePass::BeginPass::3_SetPerPassDescriptorTables")
			
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::TLAS)); // Per scene
			commandList->SetGraphicsRoot32BitConstant(0, cameraRdhIndex, 1); // Per View
			// commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::VIEWS)); // Per View
			commandList->SetGraphicsRootDescriptorTable(3, m_noiseTextureHandle[frameIdx]); // Per Pass
		}
	}

	void ShadowVariancePass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		SCOPED_TIMER("ShadowVariancePass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0xff), L"Shadow Variance Pass");

		uint32_t frameIdx = context->GetFrameIndex();

		// Fill root parameters
		commandList->SetGraphicsRoot32BitConstant(0, gTotalFrameIndex, 0);

		for (const ShadowVariancePassData& shadowVariancePassData : m_shadowVariancePassData)
		{
			BYTE colorIntensity = std::min(1.f, shadowVariancePassData.m_indexCount / 50000.f) * 0xff; 
			PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, colorIntensity, colorIntensity), shadowVariancePassData.m_name);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &shadowVariancePassData.m_vertexBufferView);
			commandList->IASetIndexBuffer(&shadowVariancePassData.m_indexBufferView);

			commandList->SetGraphicsRoot32BitConstant(0, shadowVariancePassData.m_objectRdhIndex, 2); // Per View
			// commandList->SetGraphicsRootDescriptorTable(4, shadowVariancePassData.m_objectHandle[frameIdx]); // Per object
			commandList->DrawIndexedInstanced(shadowVariancePassData.m_indexCount, 1, 0, 0, 0);
		}

		gRenderer->TransitionResources(commandList, {
			{ m_shadowVarianceTarget.get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE },
			{ m_shadowVarianceTargetResolved.get(), D3D12_RESOURCE_STATE_RESOLVE_DEST }
		});
		commandList->ResolveSubresource(m_shadowVarianceTargetResolved->GetResource().Get(), 0,
			m_shadowVarianceTarget->GetResource().Get(), 0, m_shadowVarianceTarget->GetResource()->GetDesc().Format);

		gRenderer->TransitionResources(commandList, {
			{ m_shadowVarianceTargetResolved.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
		});
	}

	void ShadowVariancePass::AddShadowVariancePassRun(const ShadowVariancePassData& shadowVariancePassData)
	{
		m_shadowVariancePassData.push_back(shadowVariancePassData);
	}
}
