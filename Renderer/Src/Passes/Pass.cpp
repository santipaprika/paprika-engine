#include <Passes/Pass.h>

#include <ApplicationHelper.h>
#include <Renderer.h>

namespace PPK::PassUtils
{
	ComPtr<ID3D12RootSignature> CreateRootSignature(const std::span<CD3DX12_ROOT_PARAMETER1> parameters,
	                                                const std::span<CD3DX12_STATIC_SAMPLER_DESC>
	                                                staticSamplers,
	                                                D3D12_ROOT_SIGNATURE_FLAGS flags,
	                                                const std::string& rsName)
	{
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSig(parameters.size(), parameters.data(),
													  staticSamplers.size(), staticSamplers.data(), flags);

		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> error;
		HRESULT HR = D3D12SerializeVersionedRootSignature(&RootSig, &serializedRootSignature, &error);
		ThrowIfFailed(HR, error.Get());

		ComPtr<ID3D12RootSignature> rootSignature;
		ThrowIfFailed(gDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature)));

		NAME_D3D12_OBJECT_CUSTOM(rootSignature, rsName.c_str());

		return rootSignature;
	}
}


using namespace PPK;

Pass::Pass(const wchar_t* name)
	: m_name(name)
{
	m_frameDirty[0] = true;
	m_frameDirty[1] = true;

	m_fencePSOValue = 1;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	ThrowIfFailed(gDevice->CreateFence(m_fencePSOValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fencePSO)));
}

Pass::~Pass()
{
	CloseHandle(m_fenceEvent);
	m_fencePSO.Reset();
}

void Pass::ReloadPSO(ComPtr<ID3D12PipelineState> newPSO)
{
	// Wait until the current PSO is done being used on GPU.
	ThrowIfFailed(m_fencePSO->SetEventOnCompletion(m_fencePSOValue, m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	m_pipelineState = newPSO;
	
}

void Pass::SignalPSOFence()
{
	gRenderer->AddSignal(m_fencePSO, ++m_fencePSOValue);
}
