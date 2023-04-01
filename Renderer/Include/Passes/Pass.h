#pragma once

#include <stdafx_renderer.h>

namespace PPK
{
	class Renderer;
	class Mesh;
	struct RenderContext;

	class Pass
	{
	public:
		Pass(Microsoft::WRL::ComPtr<ID3D12Device> device);
		~Pass() = default;

		// Initialize root signature, PSO and shaders
		void InitPass();
		void PopulateCommandList(const RenderContext& context, const Renderer& renderer, std::vector<Mesh>& meshes) const;

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	};
}