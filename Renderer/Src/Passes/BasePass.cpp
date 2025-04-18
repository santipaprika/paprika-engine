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

	BasePass::BasePass()
	{
		BasePass::InitPass();
	}

	void BasePass::InitPass()
	{
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			constexpr uint32_t numRootParameters = 5;
			CD3DX12_ROOT_PARAMETER1 RP[numRootParameters];

			RP[0].InitAsConstants(1, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL); // 1 constant at b0
			{
				// Scene descriptor TODO: Make root descriptor
				CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
				DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // BLAS - t0
				RP[1].InitAsDescriptorTable(1, &DescRange[0]); // 1 ranges t0
			}
			{
				// Camera TODO: Make root descriptor
				CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
				DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // CamTransform - b1
				RP[2].InitAsDescriptorTable(1, &DescRange[0]); // 1 ranges b1
			}
			{
				// Mesh descriptor TODO: Make root descriptor
				CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
				DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // Mesh transform - b2
				RP[3].InitAsDescriptorTable(1, &DescRange[0]); // 1 ranges b2
			}
			{
				// Material descriptor
				CD3DX12_DESCRIPTOR_RANGE1 DescRange[1];
				DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // Material texture - t1
				RP[4].InitAsDescriptorTable(1, &DescRange[0]); // 1 ranges t1
			}

			CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
			StaticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(numRootParameters, RP, 1, StaticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
			ComPtr<ID3DBlob> serializedRootSignature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error));
			ThrowIfFailed(gDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
				IID_PPV_ARGS(&m_rootSignature)));

			NAME_D3D12_OBJECT_CUSTOM(m_rootSignature, L"BasePassRS");
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
		// TODO: Abstract this to Renderer and tidy a bit PLS!!!
		IDxcLibrary* library;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));

		IDxcCompiler* compiler;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

		uint32_t codePage = CP_UTF8;
		ComPtr<IDxcBlobEncoding> vsSourceBlob;
		ThrowIfFailed(library->CreateBlobFromFile(GetAssetFullPath(vertexShaderPath).c_str(), &codePage, &vsSourceBlob));

		// TODO: Handle stripping debug and reflection blobs
		const wchar_t* arguments[] = { L"-Zi", L"-Od" }; // Debug + skip optimization

		ComPtr<IDxcOperationResult> result;
		HRESULT hr = compiler->Compile(
			vsSourceBlob.Get(), // pSource
			vertexShaderPath, // pSourceName
			L"VSMain", // pEntryPoint
			L"vs_6_6", // pTargetProfile
			arguments, _countof(arguments), // pArguments, argCount
			NULL, 0, // pDefines, defineCount
			NULL, // pIncludeHandler
			&result); // ppResult
		if(SUCCEEDED(hr))
			result->GetStatus(&hr);
		if(FAILED(hr))
		{
			if(result)
			{
				ComPtr<IDxcBlobEncoding> errorsBlob;
				hr = result->GetErrorBuffer(&errorsBlob);
				if(SUCCEEDED(hr) && errorsBlob)
				{
					Logger::Error((const char*)errorsBlob->GetBufferPointer());
				}
			}
		}
		ComPtr<IDxcBlob> vsCode;
		result->GetResult(&vsCode);

		ComPtr<IDxcBlobEncoding> psSourceBlob;
		ThrowIfFailed(library->CreateBlobFromFile(GetAssetFullPath(pixelShaderPath).c_str(), &codePage, &psSourceBlob));

		ComPtr<IDxcOperationResult> psResult;
		hr = compiler->Compile(
			psSourceBlob.Get(), // pSource
			pixelShaderPath, // pSourceName
			L"PSMain", // pEntryPoint
			L"ps_6_6", // pTargetProfile
			arguments, _countof(arguments), // pArguments, argCount
			NULL, 0, // pDefines, defineCount
			NULL, // pIncludeHandler
			&psResult); // ppResult
		if(SUCCEEDED(hr))
			psResult->GetStatus(&hr);
		if(FAILED(hr))
		{
			if(psResult)
			{
				ComPtr<IDxcBlobEncoding> errorsBlob;
				hr = psResult->GetErrorBuffer(&errorsBlob);
				if(SUCCEEDED(hr) && errorsBlob)
				{
					Logger::Error((const char*)errorsBlob->GetBufferPointer());
				}
			}
		}
		ComPtr<IDxcBlob> psCode;
		psResult->GetResult(&psCode);

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
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(gDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	void BasePass::BeginPass(std::shared_ptr<RHI::CommandContext> context)
	{
		Pass::BeginPass(context);

		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		const uint32_t frameIdx = context->GetFrameIndex();

		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xfa, 0x00), L"Begin Base Pass");

		{
			// Record commands.
			constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gDescriptorHeapManager->GetFramebufferDescriptorHandle(frameIdx);
			const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapElement()->GetCPUHandle();
			commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

			// const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = gDescriptorHeapManager->GetFramebufferDescriptorHandle(frameIdx);
			// const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthTarget->GetDescriptorHeapElement()->GetCPUHandle();
			commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
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

	void BasePass::PrepareDescriptorTables(std::shared_ptr<RHI::CommandContext> context, CameraComponent& camera, RHI::GPUResource* TLAS)
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
			m_cbvBlockStart[frameIdx] = gDescriptorHeapManager->GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 5, frameIdx);

			// Copy descriptors to shader visible heap
			D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle = m_cbvBlockStart[frameIdx].GetCPUHandle();
			constexpr uint32_t TLASCBVIndex = 0;
			TLAS->CopyDescriptorsToShaderHeap(currentCBVHandle, TLASCBVIndex);
			constexpr uint32_t cameraCBVIndex = 1;
			camera.GetConstantBuffer().CopyDescriptorsToShaderHeap(currentCBVHandle, cameraCBVIndex);
			m_frameDirty[frameIdx] = false;
		}
	}

	void BasePass::PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, uint32_t meshIdx)
	{
		ComPtr<ID3D12GraphicsCommandList4> commandList = context->GetCurrentCommandList();
		PIXScopedEvent(commandList.Get(), PIX_COLOR(0x00, 0xff, 0x00), L"Base Pass");

		// D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc;
		// D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
		// geometryDesc.Triangles.
		// commandList->BuildRaytracingAccelerationStructure()
		{
			float time = Timer::GetApplicationTimeInSeconds();

			uint32_t frameIdx = context->GetFrameIndex();

			// TODO tidy this a bit
			// Copy descriptors to shader visible heap
			D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle = m_cbvBlockStart[frameIdx].GetCPUHandle();
			constexpr uint32_t meshCBVIndex = 2;
			constexpr uint32_t numVariableParametersInHeap = 2;
			const uint32_t meshHeapIndex = meshCBVIndex + meshIdx * numVariableParametersInHeap;
			mesh.GetObjectBuffer().CopyDescriptorsToShaderHeap(currentCBVHandle, meshHeapIndex);
			constexpr uint32_t materialCBVIndex = 3;
			const uint32_t materialHeapIndex = materialCBVIndex + meshIdx * numVariableParametersInHeap;
			if (mesh.m_material.GetTexture(BaseColor))
			{
				mesh.m_material.GetTexture(BaseColor)->CopyDescriptorsToShaderHeap(currentCBVHandle, materialHeapIndex);
			}
			
			// for (const Mesh& mesh : meshes)
			// {
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &mesh.GetVertexBufferView());
			commandList->IASetIndexBuffer(&mesh.GetIndexBufferView());

			// Fill root parameters
			commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<UINT*>(&time), 0);
			//commandList->SetGraphicsRootConstantBufferView(1, camera.GetConstantBuffer()->GetGpuAddress());
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, frameIdx);
			ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };

			commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			commandList->SetGraphicsRootDescriptorTable(1, m_cbvBlockStart[frameIdx].GetGPUHandle());
			constexpr uint32_t cameraHeapIndex = 1;
			commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHeap->GetHeapGPUFromBaseOffset(m_cbvBlockStart[frameIdx].GetGPUHandle(), cameraHeapIndex));
			commandList->SetGraphicsRootDescriptorTable(3, cbvSrvHeap->GetHeapGPUFromBaseOffset(m_cbvBlockStart[frameIdx].GetGPUHandle(), meshHeapIndex));
			commandList->SetGraphicsRootDescriptorTable(4, cbvSrvHeap->GetHeapGPUFromBaseOffset(m_cbvBlockStart[frameIdx].GetGPUHandle(), materialHeapIndex));
			commandList->DrawIndexedInstanced(mesh.GetIndexCount(), 1, 0, 0, 0);
			// }
		}
	}
}
