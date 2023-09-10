#include <Camera.h>
#include <Mesh.h>
#include <Renderer.h>
#include <Timer.h>
#include <Passes/Pass.h>
#include <RHI/CommandContext.h>

using namespace PPK;

constexpr wchar_t* vertexShaderPath = L"Shaders/SampleVertexShader.hlsl";
constexpr wchar_t* pixelShaderPath = L"Shaders/SamplePixelShader.hlsl";

Pass::Pass(ComPtr<ID3D12Device> device)
	: m_device(device)
{
	InitPass();
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
		// CD3DX12_DESCRIPTOR_RANGE1 DescRange[6];

		//DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_SRV, 6, 2); // t2-t7
		//DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_UAV, 4, 0); // u0-u3
		//DescRange[2].Init(D3D12_DESCRIPTOR_RANGE_SAMPLER, 2, 0); // s0-s1
		//DescRange[3].Init(D3D12_DESCRIPTOR_RANGE_SRV, -1, 8, 0,
		//	D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // t8-unbounded
		//DescRange[4].Init(D3D12_DESCRIPTOR_RANGE_SRV, -1, 0, 1,
		//	D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		//// (t0,space1)-unbounded
		//DescRange[5].Init(D3D12_DESCRIPTOR_RANGE_CBV, 1, 1,
		//	D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // b1

		CD3DX12_ROOT_PARAMETER1 RP[2];

		RP[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 1 constant at b0
		RP[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC); // camera cbv at b1
		//RP[1].InitAsDescriptorTable(2, &DescRange[0]); // 2 ranges t2-t7 and u0-u3
		//RP[3].InitAsDescriptorTable(1, &DescRange[2]); // s0-s1
		//RP[4].InitAsDescriptorTable(1, &DescRange[3]); // t8-unbounded
		//RP[5].InitAsDescriptorTable(1, &DescRange[4]); // (t0,space1)-unbounded
		//RP[6].InitAsDescriptorTable(1, &DescRange[5]); // b1

		// CD3DX12_STATIC_SAMPLER StaticSamplers[1];
		// StaticSamplers[0].Init(3, D3D12_FILTER_ANISOTROPIC); // s3
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(2, RP, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
		                                            IID_PPV_ARGS(&m_rootSignature)));
	}

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
	ComPtr<ID3DBlob> errors;
	HRESULT hr = D3DCompileFromFile(GetAssetFullPath(vertexShaderPath).c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
		compileFlags, 0, &vertexShader, &errors);
	ThrowIfFailed(hr, errors.Get());

	hr = D3DCompileFromFile(GetAssetFullPath(pixelShaderPath).c_str(), nullptr, nullptr, "PSMain", "ps_5_0",
	                                 compileFlags, 0, &pixelShader, &errors);
	ThrowIfFailed(hr, errors.Get());


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void Pass::PopulateCommandList(const std::shared_ptr<RHI::CommandContext> context, const PPK::Renderer& renderer, std::vector<Mesh>& meshes, std::vector<Camera>& cameras) const
{
	ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
	PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0x00), L"Depth Pass");

	// Set necessary state.
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	{
		const CD3DX12_VIEWPORT viewport = renderer.GetViewport();
		commandList->RSSetViewports(1, &viewport);
		const CD3DX12_RECT scissorRect = renderer.GetScissorRect();
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	{
		// Indicate that the back buffer will be used as a render target.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = renderer.GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &framebufferBarrier);
	}

	{
		const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = RHI::GPUResourceManager::Get()->GetFramebufferDescriptorHandle(context->GetFrameIndex());
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record commands.
		constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		float time = Timer::GetApplicationTimeInSeconds();
		for (const Mesh& mesh : meshes)
		{
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());

			// Fill root parameters
			commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);
			commandList->SetGraphicsRootConstantBufferView(1, cameras[0].GetConstantBuffer()->GetGpuAddress());
			commandList->DrawInstanced(3, 1, 0, 0);
		}
	}

	{
		// Indicate that the back buffer will now be used to present.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = renderer.GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &framebufferBarrier);
	}
}
