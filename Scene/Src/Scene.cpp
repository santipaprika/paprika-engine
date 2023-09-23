#include <Scene.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>

namespace PPK
{
	Scene::Scene(std::shared_ptr<Renderer> renderer)
		: m_renderer(renderer)
	{
	}

	Scene::~Scene()
	{
		m_cameraEntity = nullptr;
		m_lightEntities.clear();
		m_meshEntities.clear();

		Logger::Info("Removing Scene");
		//Camera::GetCameras().clear();
		Mesh::GetMeshes().clear();
	}

	void Scene::InitializeScene(const Microsoft::glTF::Document& document)
	{
		// Initialize and add meshes
		for (const Microsoft::glTF::Mesh& mesh : document.meshes.Elements())
		{
			std::unique_ptr<MeshEntity> meshEntity = std::move(MeshEntity::CreateFromGltfMesh(mesh, document));
			meshEntity->UploadMesh(*m_renderer.get());
			m_meshEntities.push_back(std::move(meshEntity));
		}
	
		// Initialize and add camera
		Camera::CameraInternals camInternals;
		camInternals.m_aspectRatio = m_renderer->GetAspectRatio();
	
		Camera::CameraGenerationData camGenerationData;
		camGenerationData.m_cameraInternals = camInternals;
		camGenerationData.m_position = DirectX::XMFLOAT3{ 0.f, 0.5f, -1.f };
		camGenerationData.m_front = DirectX::XMFLOAT3{ 0.f, 0.f, 1.f };
	
		m_cameraEntity = std::make_unique<CameraEntity>(std::move(camGenerationData));
	
		// Initialize and add lights
		// ...

		// Create pass manager (this adds all passes in order)
		m_passManager = std::make_unique<PassManager>(m_renderer);
	
		Logger::Info("Scene initialized successfully!");
	}

	void Scene::OnRender()
	{
		m_renderer->BeginFrame();

		m_passManager->RecordPasses(*m_meshEntities[0]->m_mesh, m_cameraEntity->m_camera);

		m_renderer->EndFrame();
	}
}
