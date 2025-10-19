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
	}

	class Pass
	{
	public:
		Pass(const wchar_t* name = L"UndefinedPass");
		Pass(const Pass&) = default;
		virtual ~Pass() = default;

		// Initialize root signature, PSO and shaders
		virtual void InitPass() = 0;
		virtual void BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext) {}
		virtual void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) = 0;

	protected:
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		const wchar_t* m_name;

		bool m_frameDirty[2];
	};
}
