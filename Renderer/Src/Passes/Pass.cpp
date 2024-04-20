#include <ApplicationHelper.h>
#include <Camera.h>
#include <Mesh.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/Pass.h>
#include <RHI/CommandContext.h>

using namespace PPK;

constexpr wchar_t* vertexShaderPath = L"Shaders/SampleVertexShader.hlsl";
constexpr wchar_t* pixelShaderPath = L"Shaders/SamplePixelShader.hlsl";

Pass::Pass()
{
	InitPass();
	m_frameDirty[0] = true;
	m_frameDirty[1] = true;
}

void Pass::InitPass()
{
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// ComPtr<ID3DBlob> signature;
		// ComPtr<ID3DBlob> error;
		// ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature,
		//                                           &error));
		//
		CD3DX12_DESCRIPTOR_RANGE1 DescRange[6];

		//DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_SRV, 6, 2); // t2-t7
		//DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_UAV, 4, 0); // u0-u3
		//DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_SAMPLER, 2, 0); // s0-s1
		//DescRange[3].Init(D3D12_DESCRIPTOR_RANGE_SRV, -1, 8, 0,
		//	D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // t8-unbounded
		//DescRange[4].Init(D3D12_DESCRIPTOR_RANGE_SRV, -1, 0, 1,
		//	D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		//// (t0,space1)-unbounded
		DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1); // b1-b2
		DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0-t0

		CD3DX12_ROOT_PARAMETER1 RP[2];

		RP[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 1 constant at b0
		//RP[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC); // camera cbv at b1
		RP[1].InitAsDescriptorTable(2, &DescRange[0]); // 1 ranges b1-b2
		//RP[3].InitAsDescriptorTable(1, &DescRange[2]); // s0-s1
		//RP[4].InitAsDescriptorTable(1, &DescRange[3]); // t8-unbounded
		//RP[5].InitAsDescriptorTable(1, &DescRange[4]); // (t0,space1)-unbounded
		//RP[6].InitAsDescriptorTable(1, &DescRange[5]); // b1

		//D3D12_STATIC_SAMPLER_DESC StaticSamplers[1];
		//StaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		//StaticSamplers[0].ShaderRegister = 0; // s0
		//StaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
		StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(2, RP, 1, StaticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error));
		ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
		                                            IID_PPV_ARGS(&m_rootSignature)));

		NAME_D3D12_OBJECT_CUSTOM(m_rootSignature, L"DepthPassRS");
	}

	// Create depth stencil texture
	m_depthTarget = RHI::Texture::CreateDepthTextureResource(WIDTH, HEIGHT, L"DepthTarget");

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
	ComPtr<ID3DBlob> errors;
	HRESULT hr = D3DCompileFromFile(GetAssetFullPath(vertexShaderPath).c_str(), nullptr, nullptr, "VSMain", "vs_5_1",
		compileFlags, 0, &vertexShader, &errors);
	ThrowIfFailed(hr, errors.Get());

	hr = D3DCompileFromFile(GetAssetFullPath(pixelShaderPath).c_str(), nullptr, nullptr, "PSMain", "ps_5_1",
	                                 compileFlags, 0, &pixelShader, &errors);
	ThrowIfFailed(hr, errors.Get());


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
	psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
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
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(DX12Interface::Get()->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void Pass::PopulateCommandList(const std::shared_ptr<RHI::CommandContext> context, Mesh& mesh, Camera& camera)
{
	ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
	PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0x00), L"Depth Pass");

	// Set necessary state.
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	{
		const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
		commandList->RSSetViewports(1, &viewport);
		const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	{
		// Indicate that the back buffer will be used as a render target.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = gRenderer->GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &framebufferBarrier);
	}

	{
		// TODO maybe this can be called as constant buffer static method so that heap type is automatically deduced?
		// This is done lazily, try to refactor at some point
		static RHI::DescriptorHeapHandle cbvBlockStart[2]; // Frame count
		const uint32_t frameIdx = context->GetFrameIndex();
		if (m_frameDirty[frameIdx])
		{
			cbvBlockStart[frameIdx] = RHI::GPUResourceManager::Get()->GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, frameIdx);

			// Copy descriptors to shader visible heap
			D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle = cbvBlockStart[frameIdx].GetCPUHandle();
			const uint32_t cbvDescriptorSize = DX12Interface::Get()->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			DX12Interface::Get()->GetDevice()->CopyDescriptorsSimple(1, currentCBVHandle, camera.GetConstantBuffer()->GetDescriptorHeapHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			currentCBVHandle.ptr += cbvDescriptorSize;
			DX12Interface::Get()->GetDevice()->CopyDescriptorsSimple(1, currentCBVHandle, mesh.GetObjectBuffer()->GetDescriptorHeapHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			currentCBVHandle.ptr += cbvDescriptorSize;
			DX12Interface::Get()->GetDevice()->CopyDescriptorsSimple(1, currentCBVHandle,  duckAlbedoTexture->GetDescriptorHeapHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_frameDirty[frameIdx] = false;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = RHI::GPUResourceManager::Get()->GetFramebufferDescriptorHandle(frameIdx);
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapHandle().GetCPUHandle();
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Record commands.
		constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
		float time = Timer::GetApplicationTimeInSeconds();
		// for (const Mesh& mesh : meshes)
		// {
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
			commandList->IASetIndexBuffer(&mesh.GetIndexBufferView());

			// Fill root parameters
			commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);
			//commandList->SetGraphicsRootConstantBufferView(1, camera.GetConstantBuffer()->GetGpuAddress());
			RHI::ShaderDescriptorHeap* cbvSrvHeap = RHI::GPUResourceManager::Get()->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };

			commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			commandList->SetGraphicsRootDescriptorTable(1, cbvBlockStart[frameIdx].GetGPUHandle());
			commandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
		// }
	}

	{
		// Indicate that the back buffer will now be used to present.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = gRenderer->GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &framebufferBarrier);
	}
}
