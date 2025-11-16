#pragma once

#include <stdafx_renderer.h>
#include <memory>
#include <span>
#include <wrl/client.h>

namespace PPK
{
	class MeshComponent;
	class CameraComponent;

	namespace RHI
	{
		class CommandContext;
	}

	struct SceneRenderContext
	{
		uint32_t m_mainCameraRdhIndex;
		uint32_t m_lightsRdhIndex;
	};

	using namespace Microsoft::WRL;
	namespace PassUtils
	{
		ComPtr<ID3D12RootSignature> CreateRootSignature(std::span<CD3DX12_ROOT_PARAMETER1> parameters,
		                                                       std::span<CD3DX12_STATIC_SAMPLER_DESC>
		                                                       staticSamplers, D3D12_ROOT_SIGNATURE_FLAGS
		                                                       flags = D3D12_ROOT_SIGNATURE_FLAG_NONE,
		                                                       const std::string& rsName = "UndefinedRS");
		void ReloadPSO(ComPtr<ID3D12Fence> fencePSO, UINT64 fencePSOValue, HANDLE fenceEvent);
	}

	class Pass
	{
	public:
		Pass(const wchar_t* name = L"UndefinedPass");
		Pass(const Pass&) = default;
		virtual ~Pass();

		// Initialize root signature, PSO and shaders
		virtual void CreatePSO() = 0;
		virtual void InitPass() = 0;
		virtual void BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext) {}
		virtual void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) = 0;

		/**
		 * Reload PSO when the existing one is done being used (signaled by SignalPSOFence())
		 * @param newPSO New PSO to be used 
		 */
		void ReloadPSO(ComPtr<ID3D12PipelineState> newPSO);
		
		/**
		 * Signal end of pass to potentially start PSO update
		 */
		void SignalPSOFence();

	protected:
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		const wchar_t* m_name;

		bool m_frameDirty[2];

		// Attributes required for PSO hot reload 
		ComPtr<ID3D12Fence> m_fencePSO;
		UINT64 m_fencePSOValue;
		HANDLE m_fenceEvent;
	};
}
