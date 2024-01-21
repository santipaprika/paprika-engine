#include <Scene.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>

namespace PPK
{
	Scene::Scene()
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
		// for (const Microsoft::glTF::Mesh& mesh : document.meshes.Elements())
		// {
		// 	std::unique_ptr<MeshEntity> meshEntity = std::move(MeshEntity::CreateFromGltfMesh(mesh, document));
		// 	meshEntity->UploadMesh(*gRenderer.get());
		// 	m_meshEntities.push_back(std::move(meshEntity));
		// }

		ImportGLTFScene(document);

		// Initialize and add camera
		// m_cameraEntity = CameraEntity::CreateFromGltfMesh(document.cameras[0], document, gRenderer->GetAspectRatio());

		/*Camera::CameraInternals camInternals;
		camInternals.m_aspectRatio = gRenderer->GetAspectRatio();*/

		//Camera::CameraDescriptor cameraDescriptor;
		//cameraDescriptor.m_cameraInternals = camInternals;
		//const Vector3 camPos = Vector3( 10.f, 0.5f, 5.f );
		////Vector3 camFront = Vector3( 0.f, 0.f, 1.f );
		//cameraDescriptor.m_transform = Matrix::CreateLookAt(camPos, camPos + Vector3::Left, Vector3::Up);
		//// CreateLookAt returns worldToX matrix (where X is view in this case), but transform should be XToWorld
		//cameraDescriptor.m_transform = cameraDescriptor.m_transform.GetInverse();
		//m_cameraEntity = std::make_unique<CameraEntity>(cameraDescriptor);

		// Initialize and add lights
		// ...

		// Create pass manager (this adds all passes in order)
		m_passManager = std::make_unique<PassManager>();

		Logger::Info("Scene initialized successfully!");
	}

	void Scene::ImportGLTFScene(const Microsoft::glTF::Document& document)
	{
		TraverseGLTFNode(document, document.nodes[stoi(document.scenes[0].nodes[0])], Matrix::Identity);
	}

	void Scene::TraverseGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform)
	{
		// Process the node's transform and check for attached meshes, cameras, etc.
		const Matrix nodeTransform = ProcessGLTFNode(document, node, parentGlobalTransform);

		// Recursively process all children
		for (auto& childIdx : node.children) {
			const Microsoft::glTF::Node& childNode = document.nodes[stoi(childIdx)];
			TraverseGLTFNode(document, childNode, nodeTransform);
		}
	}

	Matrix Scene::ProcessGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform)
	{
		Matrix nodeGlobalTransform;

		// Get node's local transform first
		if (node.GetTransformationType() == Microsoft::glTF::TRANSFORMATION_TRS)
		{
			Matrix rotation = Matrix::CreateFromQuaternion(Quaternion(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));
			Matrix scale = Matrix::CreateScale(node.scale.x, node.scale.y, node.scale.z);
			nodeGlobalTransform = scale * rotation;
			nodeGlobalTransform.Translation(Vector3(node.translation.x, node.translation.y, node.translation.z));
		}
		else if (node.GetTransformationType() == Microsoft::glTF::TRANSFORMATION_MATRIX)
		{
			nodeGlobalTransform = Matrix(node.matrix.values.data());
		}
		else
		{
			Logger::Warning("No suitable transformation type found in GLTF document");
		}

		// Apply parent global transform chain; nodeTransform is now in world space
		nodeGlobalTransform *= parentGlobalTransform;

		// Create camera if node has camera
		if (!node.cameraId.empty())
		{
			m_cameraEntity = CameraEntity::CreateFromGltfCamera(document.cameras[node.cameraId], nodeGlobalTransform);
		}

		// Create mesh if node has mesh
		if (!node.meshId.empty())
		{
			// Todo make renderer static so that it's called where necessary without having to carry it unused over many func calls
			std::unique_ptr<MeshEntity> meshEntity = MeshEntity::CreateFromGltfMesh(document, document.meshes[node.meshId], nodeGlobalTransform);
			meshEntity->UploadMesh();
			m_meshEntities.push_back(std::move(meshEntity));
		}

		return nodeGlobalTransform;
	}

	void Scene::OnUpdate(float deltaTime)
	{
		m_cameraEntity->MoveCamera(deltaTime);
		for (std::shared_ptr<MeshEntity> meshEntity : m_meshEntities)
		{
			meshEntity->Update();
		}
	}

	void Scene::OnRender()
	{
		gRenderer->BeginFrame();

		m_passManager->RecordPasses(*m_meshEntities[0]->m_mesh, m_cameraEntity->m_camera);

		gRenderer->EndFrame();
	}
}
