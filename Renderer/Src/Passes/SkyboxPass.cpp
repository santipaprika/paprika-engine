#include <ApplicationHelper.h>
#include <dxcapi.h>
#include <Renderer.h>
#include <Passes/SkyboxPass.h>
#include <RHI/Texture.h>

namespace PPK
{
	constexpr const wchar_t* vertexShaderPath = L"Shaders/SkyboxPassVS.hlsl";
	constexpr const wchar_t* pixelShaderPath = L"Shaders/SkyboxPassPS.hlsl";

    SkyboxPass::SkyboxPass(const wchar_t* name)
        : Pass(name)
    {
        SkyboxPass::CreatePSO();
		SkyboxPass::CreatePassResources();

    	m_bIsScreenSizeDependent = false;
    }

    void SkyboxPass::CreatePSO()
    {
        {
			CD3DX12_ROOT_PARAMETER1 rootConstants;
			rootConstants.InitAsConstants(2, 0, 0); // 10 constants at b0

			CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1];
			staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

			CD3DX12_ROOT_PARAMETER1 RPs[] = { rootConstants };
			m_rootSignature = PassUtils::CreateRootSignature(std::span(RPs, _countof(RPs)), std::span(staticSamplers, _countof(staticSamplers)),
				D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED, "SkyboxPassRS");
		}

		IDxcBlob* vsCode;
		gRenderer->CompileShader(vertexShaderPath, L"MainVS", L"vs_6_8", &vsCode, m_pipelineState == nullptr);
		IDxcBlob* psCode;
		gRenderer->CompileShader(pixelShaderPath, L"MainPS", L"ps_6_8", &psCode, m_pipelineState == nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { nullptr, 0 };
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
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = gMSAA ? gMSAACount : 1;

		ComPtr<ID3D12PipelineState> pso;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
		NAME_D3D12_OBJECT_CUSTOM(pso, L"SkyboxPassPSO");

		ReloadPSO(pso);
    }

    void SkyboxPass::CreatePassResources()
    {
	    Pass::CreatePassResources();

		std::wstring filePaths[] = {
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_right.jpg"),
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_left.jpg"),
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_top.jpg"),
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_bottom.jpg"),
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_front.jpg"),
			GetAssetFullFilesystemPath("Textures/Skybox/skybox_back.jpg")
		};
    	DirectX::ScratchImage scratchImage = LoadTextureCubeFromDisk(filePaths);
	    m_skyboxTexture = RHI::CreateTextureResource("Skybox", &scratchImage);
    	m_skyboxTextureIndex = m_skyboxTexture->GetIndexInRDH(RHI::EResourceViewType::SRV);
    }

    void SkyboxPass::DestroyPassResources()
    {
	    Pass::DestroyPassResources();
    	Logger::Assert(m_skyboxTexture.use_count() == 1);
    	m_skyboxTexture.reset(); //< Trigger destructor
    }

    void SkyboxPass::InitPassParams()
    {
	    Pass::InitPassParams();
	    m_renderTarget = GetGlobalGPUResource("RT_BasePass_MS");
		m_depthTarget = GetGlobalGPUResource("RT_Depth_MS");
    }

    void SkyboxPass::BeginPass(std::shared_ptr<RHI::CommandContext> context,
	    const SceneRenderContext sceneRenderContext)
    {
	    Pass::BeginPass(context, sceneRenderContext);

	    ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
	    PIXScopedEvent(commandList.Get(), PIX_COLOR(0xcc, 0xfa, 0x11), L"Begin Skybox Pass");

	    gRenderer->TransitionResources(commandList, {
			{ m_renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET },
			{ m_depthTarget, D3D12_RESOURCE_STATE_DEPTH_READ },
			{ m_skyboxTexture.get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
       });

    	const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = {
    		m_renderTarget->GetDescriptorHeapHandle(RHI::EResourceViewType::RTV).GetCPUHandle()
		};
		const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapHandle(RHI::EResourceViewType::DSV).GetCPUHandle();
    	
    	commandList->OMSetRenderTargets(_countof(rtvHandles), rtvHandles, FALSE, &dsvHandle);

		// Set necessary state.
		commandList->SetPipelineState(m_pipelineState.Get());
	    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	    const CD3DX12_VIEWPORT viewport = gRenderer->GetViewport();
	    commandList->RSSetViewports(1, &viewport);
	    const CD3DX12_RECT scissorRect = gRenderer->GetScissorRect();
	    commandList->RSSetScissorRects(1, &scissorRect);

    	// Parameters
		commandList->SetGraphicsRoot32BitConstant(0, sceneRenderContext.m_mainCameraRdhIndex, 0);
		commandList->SetGraphicsRoot32BitConstant(0, m_skyboxTextureIndex, 1);
    }

    void SkyboxPass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context)
    {
		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
	    PIXScopedEvent(commandList.Get(), PIX_COLOR(0xcc, 0xfa, 0x11), L"Skybox Pass");

    	// 14-vertex cube strip reconstructed entirely in VS
    	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    	commandList->IASetVertexBuffers(0, 0, nullptr);
    	commandList->IASetIndexBuffer(nullptr);
		commandList->DrawInstanced(14, 1, 0, 0);
    	
    	SignalPSOFence();
    }
}
