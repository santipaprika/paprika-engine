#pragma once

#include <stdafx_renderer.h>
#include <RHI/Texture.h>

namespace PPK
{
	class Renderer;
	class Mesh;
	class Camera;

	namespace RHI
	{
		class CommandContext;
	}

	class Pass
	{
	public:
		Pass();
		Pass(const Pass&) = default;
		~Pass() = default;

		// Initialize root signature, PSO and shaders
		void InitPass();
		void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, const Renderer& renderer, Mesh& mesh, Camera& camera) const;

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		std::shared_ptr<RHI::Texture> m_depthTarget;
	};
}