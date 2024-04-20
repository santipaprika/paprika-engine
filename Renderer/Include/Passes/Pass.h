#pragma once

#include <stdafx_renderer.h>
#include <RHI/Texture.h>
#include <RHI/Sampler.h>

namespace PPK
{
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
		void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, Mesh& mesh, Camera& camera);

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		std::shared_ptr<RHI::Texture> m_depthTarget;

		bool m_frameDirty[2];
	};

	// TODO: Temp to avoid crash on exit scope, but there's leak on close. Figure out where to store this.
	inline std::shared_ptr<PPK::RHI::Texture> duckAlbedoTexture;
	inline std::shared_ptr<PPK::RHI::Sampler> defaultSampler;
}