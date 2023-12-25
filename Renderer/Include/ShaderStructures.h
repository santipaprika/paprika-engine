#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

namespace PaprikaEngine
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		Matrix model;
		Matrix view;
		Matrix projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		Vector3 pos;
		Vector3 color;
	};
}