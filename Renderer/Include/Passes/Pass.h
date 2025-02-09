#pragma once

#include <d3d12.h>
#include <memory>
#include <wrl/client.h>

namespace PPK
{
	class MeshComponent;
	class CameraComponent;

	namespace RHI
	{
		class CommandContext;
	}

	class Pass
	{
	public:
		Pass();
		Pass(const Pass&) = default;
		virtual ~Pass() = default;

		// Initialize root signature, PSO and shaders
		virtual void InitPass() = 0;
		virtual void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, CameraComponent& camera) = 0;

	protected:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		bool m_frameDirty[2];
	};
}
