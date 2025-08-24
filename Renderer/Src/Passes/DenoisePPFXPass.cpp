#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/DenoisePPFXPass.h>
#include <MeshComponent.h>
#include <PassManager.h>


namespace PPK
{
	constexpr const wchar_t* denoiserVSPath = L"Shaders/DenoiseVS.hlsl";
	constexpr const wchar_t* denoiserPSPath = L"Shaders/DenoisePS.hlsl";

	DenoisePPFXPass::DenoisePPFXPass(const wchar_t* name)
		: Pass(name)
	{
		m_denoisePassData.reserve(1); // 1 denoise pass expected for now
		DenoisePPFXPass::InitPass();
	}

	void DenoisePPFXPass::InitPass()
	{
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			constexpr uint32_t numRootParameters = 2;
			CD3DX12_ROOT_PARAMETER1 RP[numRootParameters];

			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 1 constant at b0

			// Scene descriptor TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangeInputTextures[1];
			CD3DX12_ROOT_PARAMETER1 perSceneRP;
			DescRangeInputTextures[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Input texture - t0/t2
			perSceneRP.InitAsDescriptorTable(1, &DescRangeInputTextures[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges t0/t2

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1];
			staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants, perSceneRP };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "DenoisePPFXPassRS");
		}

		// Create depth stencil texture
		// m_inputTexture = RHI::Texture::CreateDepthTextureResource(WIDTH, HEIGHT, L"BasePassDepth");
		IDxcBlob* vsCode;
		gRenderer->CompileShader(denoiserVSPath, L"MainVS", L"vs_6_6", &vsCode);
		IDxcBlob* psCode;
		gRenderer->CompileShader(denoiserPSPath, L"MainPS", L"ps_6_6", &psCode);


		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); //< Blend = False
		// psoDesc.SampleDesc.Count = 
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT_CUSTOM(m_pipelineState, L"DenoisePPFXPassPSO");

		// Copy descriptors to shader visible heap (TODO: Maybe can batch this to minimize CopyDescriptorsSimple calls?)
		// Descriptors in object location (only transform for now)
		DenoisePassData denoisePassData;
		denoisePassData.m_sceneColorTexture = GetGlobalGPUResource("RT_BasePass_Resolved");
		denoisePassData.m_rtShadowsTexture = GetGlobalGPUResource("RT_RayTracedShadowsRT");
		denoisePassData.m_depthTexture = GetGlobalGPUResource("RT_Depth_MS");

		// Copy descriptors to shader-visible heap
		for (int frameIdx = 0; frameIdx < gFrameCount; frameIdx++)
		{
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);

			// Descriptors in texture location
			D3D12_GPU_DESCRIPTOR_HANDLE descriptorsHandle = cbvSrvHeap->CopyDescriptors(denoisePassData.m_sceneColorTexture, RHI::HeapLocation::TEXTURES);
			cbvSrvHeap->CopyDescriptors(denoisePassData.m_rtShadowsTexture, RHI::HeapLocation::TEXTURES);
			cbvSrvHeap->CopyDescriptors(denoisePassData.m_depthTexture, RHI::HeapLocation::TEXTURES);
			denoisePassData.m_denoiseResourcesHandle[frameIdx] = descriptorsHandle;
			
		}

		AddDenoisePassRun(denoisePassData);
	}

	void DenoisePPFXPass::BeginPass(std::shared_ptr<RHI::CommandContext> context)
	{
		SCOPED_TIMER("DenoisePPFXPass::BeginPass")
		
		Pass::BeginPass(context);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0x00), L"Begin Denoise Pass");

		{
			const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
			commandList->RSSetViewports(1, &viewport);
			const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
			commandList->RSSetScissorRects(1, &scissorRect);
		}
	}

	void DenoisePPFXPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
	}

	void DenoisePPFXPass::PopulateCommandListPPFX(std::shared_ptr<RHI::CommandContext> context)
	{
		SCOPED_TIMER("DenoisePPFXPass::PopulateCommandListPPFX")
	
		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0x00, 0xff), L"Denoise Pass");

		float time = Timer::GetApplicationTimeInSeconds();
		uint32_t frameIdx = context->GetFrameIndex();

		for (DenoisePassData& denoisePassData : m_denoisePassData)
		{
			// Record commands.
			// constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gDescriptorHeapManager->GetFramebufferDescriptorHandle(frameIdx);
			// commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
			// commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			gRenderer->TransitionResources(commandList, {
				{denoisePassData.m_sceneColorTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
				{denoisePassData.m_rtShadowsTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
				{denoisePassData.m_depthTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE},
			});

			commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetGraphicsRootSignature(m_rootSignature.Get());

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 0, nullptr);
			commandList->IASetIndexBuffer(nullptr);

			// Fill root parameters
			commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);
			commandList->SetGraphicsRoot32BitConstant(0, gDenoise * 1u, 1);
			commandList->SetGraphicsRootDescriptorTable(1, denoisePassData.m_denoiseResourcesHandle[frameIdx]);

			commandList->DrawInstanced(3, 1, 0, 0);
		}
	}

	void DenoisePPFXPass::AddDenoisePassRun(const DenoisePassData& denoisePassData)
	{
		m_denoisePassData.push_back(denoisePassData);
	}
}
