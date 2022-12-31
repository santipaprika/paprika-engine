#pragma once

#include <ApplicationHelper.h>

namespace PPK
{
	class Renderer;
	struct RenderContext;

	class Pass
	{
	public:
		Pass(ComPtr<ID3D12Device> device);
		~Pass() = default;

		// Initialize root signature, PSO and shaders
		void InitPass();
		void PopulateCommandList(const RenderContext& context, const Renderer& renderer) const;

	private:
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pipelineState;

		ComPtr<ID3D12Device> m_device;
	};
}