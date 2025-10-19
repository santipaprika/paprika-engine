#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/DepthPass.h>
#include <CameraComponent.h>
#include <MeshComponent.h>
#include <RenderingSystem.h>


namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/DepthVS.hlsl";

	DepthPass::DepthPass(const wchar_t* name)
		: Pass(name)
	{
		m_depthPassData.reserve(64); // 64 objects expected. If heavier scenes are added, increase this. TODO: Generalize
		DepthPass::InitPass();
	}

	void DepthPass::InitPass()
	{
		{
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(2, 0, 0); // 0 constant at b0

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)),
				{}, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "DepthPassRS");
		}

		// Create MS depth stencil texture (where depth writes go)
		m_depthTarget = RHI::CreateMSDepthTextureResource(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "RT_Depth_MS");

		IDxcBlob* vsCode;
		gRenderer->CompileShader(vertexShaderPath, L"MainVS", L"vs_6_6", &vsCode);

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS.BytecodeLength = vsCode->GetBufferSize();
		psoDesc.VS.pShaderBytecode = vsCode->GetBufferPointer();
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.MultisampleEnable = TRUE;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 0;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT_CUSTOM(m_pipelineState, L"DepthPassPSO");
	}

	void DepthPass::BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext)
	{
		Pass::BeginPass(context, sceneRenderContext);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0xfa, 0x00, 0x00), L"Begin Depth Pass");

		{
			SCOPED_TIMER("DepthPass::BeginPass::1_TransitionAndClearResources")
			
			// Record commands.
			gRenderer->TransitionResources(commandList, {
				{ m_depthTarget.get(), D3D12_RESOURCE_STATE_DEPTH_WRITE },
			});

			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV).GetCPUHandle();
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// Indicate that the output of the Depth Pass will be used as a PS resource now.

			commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);
		}

		{
			SCOPED_TIMER("DepthPass::BeginPass::2_SetPSO_RS")
			
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetGraphicsRootSignature(m_rootSignature.Get());

			const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
			commandList->RSSetViewports(1, &viewport);
			const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
			commandList->RSSetScissorRects(1, &scissorRect);
		}

		{
			SCOPED_TIMER("DepthPass::BeginPass::3_SetPerPassDescriptorTables")
			
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			commandList->SetGraphicsRoot32BitConstant(0, sceneRenderContext.m_mainCameraRdhIndex, 0); // Per View
			// commandList->SetGraphicsRootDescriptorTable(0, cbvSrvHeap->GetHeapLocationGPUHandle(RHI::HeapLocation::VIEWS)); // Per View
		}
	}

	void DepthPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
	{
		SCOPED_TIMER("DepthPass::PopulateCommandList")

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0xff, 0x00, 0x00), L"Depth Pass");

		uint32_t frameIdx = context->GetFrameIndex();

		// Fill root parameters
		for (const DepthPassData& depthPassData : m_depthPassData)
		{
			BYTE colorIntensity = std::min(1.f, depthPassData.m_indexCount / 50000.f) * 0xff; 
			PIXScopedEvent(commandList.Get(), PIX_COLOR(colorIntensity, 0x00, 0x00), depthPassData.m_name);

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &depthPassData.m_vertexBufferView);
			commandList->IASetIndexBuffer(&depthPassData.m_indexBufferView);

			commandList->SetGraphicsRoot32BitConstant(0, depthPassData.m_objectRdhIndex, 1); // Per View
			// commandList->SetGraphicsRootDescriptorTable(2, depthPassData.m_objectHandle[frameIdx]); // Per object
			commandList->DrawIndexedInstanced(depthPassData.m_indexCount, 1, 0, 0, 0);
		}
	}

	void DepthPass::AddDepthPassRun(const DepthPassData& depthPassData)
	{
		m_depthPassData.push_back(depthPassData);
	}
}
