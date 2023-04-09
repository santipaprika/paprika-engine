#include <Mesh.h>
#include <Renderer.h>
#include <Passes/Pass.h>

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

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature,
		                                          &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
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
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(vertexShaderPath).c_str(), nullptr, nullptr, "VSMain", "vs_5_0",
	                                 compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(pixelShaderPath).c_str(), nullptr, nullptr, "PSMain", "ps_5_0",
	                                 compileFlags, 0, &pixelShader, nullptr));

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

void Pass::PopulateCommandList(const RenderContext& context, const PPK::Renderer& renderer, std::vector<Mesh>& meshes) const
{
	PIXScopedEvent(context.m_commandList.Get(), PIX_COLOR(0x00, 0xff, 0x00), L"Depth Pass");

	// Set necessary state.
	context.m_commandList->SetPipelineState(m_pipelineState.Get());
	context.m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	
	{
		const CD3DX12_VIEWPORT viewport = renderer.GetViewport();
		context.m_commandList->RSSetViewports(1, &viewport);
		const CD3DX12_RECT scissorRect = renderer.GetScissorRect();
		context.m_commandList->RSSetScissorRects(1, &scissorRect);
	}

	{
		// Indicate that the back buffer will be used as a render target.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = renderer.GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context.m_commandList->ResourceBarrier(1, &framebufferBarrier);
	}

	{
		const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderer.GetRtvDescriptorHandle();
		context.m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record commands.
		constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		context.m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		for (const Mesh& mesh : meshes)
		{
			context.m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context.m_commandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
			context.m_commandList->DrawInstanced(3, 1, 0, 0);
		}
	}

	{
		// Indicate that the back buffer will now be used to present.
		const CD3DX12_RESOURCE_BARRIER framebufferBarrier = renderer.GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		context.m_commandList->ResourceBarrier(1, &framebufferBarrier);
	}
}
