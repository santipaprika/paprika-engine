#include <Scene.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>

namespace PPK
{
	void Scene::InitializeScene(const Microsoft::glTF::Document& document, float windowAspectRatio)
	{
		// Initialize and add meshes
		for (const Microsoft::glTF::Mesh& mesh : document.meshes.Elements())
		{
			std::unique_ptr<MeshEntity> meshEntity = std::move(MeshEntity::CreateFromGltfMesh(mesh, document));
			m_meshEntities.push_back(std::move(meshEntity));
		}

		// Initialize and add camera
		CameraEntity::CameraInternals camInternals;
		camInternals.m_aspectRatio = windowAspectRatio;

		CameraEntity::CameraGenerationData camGenerationData;
		camGenerationData.m_cameraInternals = camInternals;
		camGenerationData.m_position = DirectX::XMFLOAT3{ 0.f, 0.5f, -1.f };
		camGenerationData.m_front = DirectX::XMFLOAT3{ 0.f, 0.f, 1.f };

		m_cameraEntity = std::make_unique<CameraEntity>(camGenerationData);

		// Initialize and add lights
		// ...

		Logger::Info("Scene initialized successfully!");
	}
}
