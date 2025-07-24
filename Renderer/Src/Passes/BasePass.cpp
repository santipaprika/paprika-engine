#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/BasePass.h>
#include <CameraComponent.h>
#include <MeshComponent.h>


namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/SampleVertexShader.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"Shaders/SamplePixelShader.hlsl";

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
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			constexpr uint32_t numRootParameters = 5;
			CD3DX12_ROOT_PARAMETER1 RP[numRootParameters];

			RP[0].InitAsConstants(2, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 2 constant at b0-b1
			
			// Scene descriptor TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangeBlas[1];
			DescRangeBlas[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // BLAS - t0
			RP[1].InitAsDescriptorTable(1, &DescRangeBlas[0]); // 1 ranges t0
			
			// Camera TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangeView[1];
			DescRangeView[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // CamTransform - b1
			RP[2].InitAsDescriptorTable(1, &DescRangeView[0]); // 1 ranges b1
			
			// Mesh descriptor TODO: Make root descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangeObjects[1];
			DescRangeObjects[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // Mesh transform - b2
			RP[3].InitAsDescriptorTable(1, &DescRangeObjects[0]); // 1 ranges b2
			
			// Material descriptor
			CD3DX12_DESCRIPTOR_RANGE1 DescRangeTextures[1];
			DescRangeTextures[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // Material texture - t1
			RP[4].InitAsDescriptorTable(1, &DescRangeTextures[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges t1


			CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
			StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(numRootParameters, RP, 1, StaticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			ComPtr<ID3DBlob> serializedRootSignature;
			ComPtr<ID3DBlob> error;
			HRESULT HR = D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error);
			ThrowIfFailed(HR, error.Get());
			ThrowIfFailed(gDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature)));

			NAME_D3D12_OBJECT_CUSTOM(m_rootSignature, L"BasePassRS");
		}

		// Create depth stencil texture
		m_depthTarget = RHI::CreateDepthTextureResource(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "RT_BasePassDepth");
		D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		// TODO: Can't use MSAA until issue is solved for depth resource (see comment in CreateDepthTextureResource)
		// Use MSAA and check supported quality modes
		// D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaLevels;
		// msaaLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Replace with your render target format.
		// msaaLevels.SampleCount = gMSAA ? gMSAACount : 1; // Replace with your sample count.
		// msaaLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		// ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaLevels, sizeof(msaaLevels)));
		// textureDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		// textureDesc.SampleDesc.Quality = gMSAA ? msaaLevels.NumQualityLevels - 1 : 1; // Max quality
		textureDesc.MipLevels = 1;

		m_renderTarget = RHI::CreateTextureResource(textureDesc, "RT_BasePassRT");

		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		m_rayTracedShadowsTarget = RHI::CreateTextureResource(textureDesc, "RT_RayTracedShadowsRT", nullptr, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8_UNORM, g_shadowsClearValue));

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
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		//psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		//psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		//psoDesc.DepthStencilState.StencilEnable = TRUE;
		//psoDesc.DepthStencilState..StencilWriteMask = 0xFF;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1; // TODO: Try increasing for MSAA;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	void BasePass::BeginPass(std::shared_ptr<RHI::CommandContext> context)
	{
		Pass::BeginPass(context);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0x00), L"Begin Base Pass");

		{
			SCOPED_TIMER("BasePass::BeginPass::1_TransitionAndClearResources")
			
			// Record commands.
			gRenderer->TransitionResources(commandList, {
				{ m_renderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET },
				{ m_rayTracedShadowsTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET },
				{ m_depthTarget.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE }
			});

			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = {
				m_renderTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandle(),
				m_rayTracedShadowsTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandle(),
			};
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->GetCPUHandle();
			commandList->ClearRenderTargetView(rtvHandles[0], PPK::g_clearColor, 0, nullptr);
			commandList->ClearRenderTargetView(rtvHandles[1], PPK::g_shadowsClearValue, 0, nullptr);
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Indicate that the output of the base pass will be used as a PS resource now.

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
			
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::TLAS));
			commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::VIEWS));
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
		commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);
		commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&m_numSamples), 1);

		for (const BasePassData& basePassData : m_basePassData)
		{
			PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfb, 0x00), basePassData.m_name);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &basePassData.m_vertexBufferView);
			commandList->IASetIndexBuffer(&basePassData.m_indexBufferView);

			commandList->SetGraphicsRootDescriptorTable(3, basePassData.m_objectHandle[frameIdx]);
			commandList->SetGraphicsRootDescriptorTable(4, basePassData.m_materialHandle[frameIdx]);
			commandList->DrawIndexedInstanced(basePassData.m_indexCount, 1, 0, 0, 0);
		}
	}

	void BasePass::AddBasePassRun(BasePassData& basePassData)
	{
		m_basePassData.push_back(basePassData);
	}
}
