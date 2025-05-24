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
		DenoisePPFXPass::InitPass();
	}

	void DenoisePPFXPass::InitPass()
	{
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			constexpr uint32_t numRootParameters = 2;
			CD3DX12_ROOT_PARAMETER1 RP[numRootParameters];

			RP[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 1 constant at b0
			{
				// Scene descriptor TODO: Make root descriptor
				CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
				DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // Input texture - t0
				RP[1].InitAsDescriptorTable(1, &DescRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // 1 ranges t0
			}

			CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
			StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(numRootParameters, RP, 1, StaticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			ComPtr<ID3DBlob> serializedRootSignature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error));
			ThrowIfFailed(gDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature)));

			NAME_D3D12_OBJECT_CUSTOM(m_rootSignature, L"DenoisePPFXPassRS");
		}

		// Create depth stencil texture
		// m_inputTexture = RHI::Texture::CreateDepthTextureResource(WIDTH, HEIGHT, L"DepthTarget");
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
	}

	void DenoisePPFXPass::BeginPass(std::shared_ptr<RHI::CommandContext> context)
	{
		Pass::BeginPass(context);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0x00), L"Begin Base Pass");

		{
			// Record commands.
			// constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gDescriptorHeapManager->GetFramebufferDescriptorHandle(frameIdx);
			// commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
			// commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			gResourcesMap[L"BasePassRT"]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		}

		{
			// Set necessary state.
			commandList->SetPipelineState(m_pipelineState.Get());
			commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		}

		{
			const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
			commandList->RSSetViewports(1, &viewport);
			const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
			commandList->RSSetScissorRects(1, &scissorRect);
		}
	}

	void DenoisePPFXPass::PrepareDescriptorTables(std::shared_ptr<RHI::CommandContext> context)
	{
		const uint32_t frameIdx = context->GetFrameIndex();
		// Dirty way to copy descriptors from resources that won't 
		if (!m_frameDirty[frameIdx])
		{
			return;
		}

		// TODO maybe this can be called as constant buffer static method so that heap type is automatically deduced?
		// This is done lazily, try to refactor at some point
		// for (int frameIdx = 0; frameIdx < RHI::gFrameCount; frameIdx++)
		{
			m_cbvBlockStart[frameIdx] = gDescriptorHeapManager->GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, frameIdx);

			// Copy descriptors to shader visible heap
			D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle = m_cbvBlockStart[frameIdx].GetCPUHandle();
			constexpr uint32_t InputTextureIndex = 0;
			gResourcesMap[L"BasePassRT"]->CopyDescriptorsToShaderHeap(currentCBVHandle, InputTextureIndex, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_frameDirty[frameIdx] = false;
		}
	}

	void DenoisePPFXPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, uint32_t meshIdx)
	{
	}

	void DenoisePPFXPass::PopulateCommandListPPFX(std::shared_ptr<RHI::CommandContext> context)
	{
		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0x00, 0xff), L"Denoise Pass");

		{
			float time = Timer::GetApplicationTimeInSeconds();
			uint32_t frameIdx = context->GetFrameIndex();

			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 0, nullptr);
			commandList->IASetIndexBuffer(nullptr);

			// Fill root parameters
			commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);

			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };
			commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHeap->GetHeapGPUFromBaseOffset(m_cbvBlockStart[frameIdx].GetGPUHandle(), 0));

			commandList->DrawInstanced(3, 1, 0, 0);
		}
	}
}
