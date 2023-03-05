#include <Scene.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>

namespace PPK
{
	void Scene::InitializeScene(const Microsoft::glTF::Document& document)
	{
		 for (const Microsoft::glTF::Mesh& mesh : document.meshes.Elements())
		 {
			std::unique_ptr<MeshEntity> meshEntity = std::move(MeshEntity::CreateFromGltfMesh(mesh, document));
			m_meshEntities.push_back(std::move(meshEntity));
		}
		 Logger::Info("Scene initialized successfully!");
	}
}
