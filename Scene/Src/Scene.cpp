#include <DirectXTex.h>
#include <GLTFReader.h>
#include <Scene.h>
#include <GLTFSDK/MeshPrimitiveUtils.h>
#include <imgui.h>
#include <InputController.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <Logger.h>
#include <Timer.h>
#include <codecvt>

#include <WinPixEventRuntime/pix3.h>
#if PPK_DEBUG
#include <WinPixEventRuntime/PIXEvents.h>
#endif
namespace PPK
{
	Scene::Scene()
		: m_numEntities(0)
	{
	}

	Scene::~Scene()
	{
		// m_cameraEntity = nullptr;
		// m_lightEntities.clear();
		// m_meshEntities.clear();

		Logger::Info("Removing Scene");
		//Camera::GetCameras().clear();
		// MeshComponent::GetMeshes().clear();
	}

	static DirectX::ScratchImage LoadTextureFromDisk(const std::wstring& filePath)
	{
		DirectX::ScratchImage image;
		HRESULT hr = DirectX::LoadFromWICFile(filePath.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to load texture from disk");
		}
		return image;
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
		MeshComponent::MeshBuildData* meshData = new MeshComponent::MeshBuildData();

		// TODO : extract this to separate function
		constexpr int numVertexPerDimension = 256;
		uint32_t nIndices = 6 * (numVertexPerDimension * numVertexPerDimension - numVertexPerDimension * 2 + 1);
		uint32_t nVerts = numVertexPerDimension * numVertexPerDimension;
		meshData->m_indices.reserve(nIndices);
		meshData->m_vertices.reserve(nVerts * 3);
		meshData->m_normals.reserve(nVerts * 3);
		meshData->m_uvs.reserve(nVerts * 2);
		// meshData->m_uvs.reserve(numVertexPerDimension * 3);
		for (int i = 0; i < numVertexPerDimension; i++)
		{
			constexpr float start = -20.f;
			constexpr float end = 20.f;
			constexpr float verticalOffset = -0.5f;
			constexpr float step = (end - start) / (numVertexPerDimension - 1);
				
			for (int j = 0; j < numVertexPerDimension; j++)
			{
				meshData->m_vertices.push_back(start + i * step);
				meshData->m_vertices.push_back(verticalOffset);
				meshData->m_vertices.push_back(start + j * step);
				
				meshData->m_uvs.push_back((i) / static_cast<float>(numVertexPerDimension));
				meshData->m_uvs.push_back((j) / static_cast<float>(numVertexPerDimension));
				// meshData->m_uvs.push_back(0.f);

				meshData->m_normals.push_back(0.f);
				meshData->m_normals.push_back(1.f);
				meshData->m_normals.push_back(0.f);

				if (j >= numVertexPerDimension - 1 || i >= numVertexPerDimension - 1)
				{
					continue;
				}

				uint32_t currentIndex = i * numVertexPerDimension + j;
				meshData->m_indices.push_back(currentIndex);
				meshData->m_indices.push_back(currentIndex + 1);
				meshData->m_indices.push_back(currentIndex + numVertexPerDimension);

				meshData->m_indices.push_back(currentIndex + 1);
				meshData->m_indices.push_back(currentIndex + numVertexPerDimension + 1);
				meshData->m_indices.push_back(currentIndex + numVertexPerDimension);
				
			}
		}
		meshData->m_nIndices = nIndices; 
		meshData->m_nVertices = nVerts; 

		// Mesh::Create(std::move(meshData));
		// MeshComponent::m_meshes.push_back(MeshComponent(meshData));

		Entity entity = m_numEntities++;

		{
			// std::make_unique<MeshEntity>(std::move(meshData), Matrix::Identity);
			DirectX::ScratchImage scratchImage = LoadTextureFromDisk(GetAssetFullFilesystemPath("Textures/checkerboard.png"));
			// TODO: Handle mips/slices/depth
			std::shared_ptr<PPK::RHI::Texture> texture = PPK::RHI::Texture::CreateTextureResource(
				scratchImage.GetMetadata(),
				L"Checkerboard",
				scratchImage.GetImage(0, 0, 0)
			);
			Material material = Material();
			material.SetTexture(texture, BaseColor);

			// LoadFromGLTFMaterial(material, document, gltfMaterial);
			m_componentManager.AddComponent<MeshComponent>(entity, std::move(m_renderingSystem.CreateMeshComponent(meshData, material, entity)));
		}
		m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{Matrix::Identity}));
		// groundEntity->UploadMesh();
		//m_meshEntities.push_back(std::move(groundEntity));
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

		CreateGPUAccelerationStructure();
		
		// Initialize and add lights
		// ...

		// Create pass manager (this adds all passes in order)
		m_passManager = std::make_unique<PassManager>();

		Logger::Info("Scene initialized successfully!");
	}

	static MeshComponent::MeshBuildData* CreateFromGltfMesh(const Microsoft::glTF::Document& document,
	                                                        const Microsoft::glTF::Mesh& gltfMesh)
	{
		MeshComponent::MeshBuildData* meshBuildData = new MeshComponent::MeshBuildData();

		meshBuildData->m_indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

		meshBuildData->m_nIndices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
			document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size();

		if (gltfMesh.primitives[0].HasAttribute("POSITION"))
		{
			meshBuildData->m_vertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
				document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);

			meshBuildData->m_nVertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
				document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]).size() / 3;
		}
		if (gltfMesh.primitives[0].HasAttribute("TEXCOORD_0"))
		{
			meshBuildData->m_uvs = Microsoft::glTF::MeshPrimitiveUtils::GetTexCoords_0(
				document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
		}
		if (gltfMesh.primitives[0].HasAttribute("NORMAL"))
		{
			meshBuildData->m_normals = Microsoft::glTF::MeshPrimitiveUtils::GetNormals(
				document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
		}
		if (gltfMesh.primitives[0].HasAttribute("COLOR"))
		{
			meshBuildData->m_colors = Microsoft::glTF::MeshPrimitiveUtils::GetColors_0(
				document, *GLTFReader::m_gltfResourceReader, gltfMesh.primitives[0]);
		}

		Timer::BeginTimer();
		Timer::EndAndReportTimer("Load GLTF attributes");

		return meshBuildData;
	}

	// Based on https://github.com/microsoft/glTF-Toolkit/blob/master/glTF-Toolkit/src/GLTFTextureUtils.cpp
	DirectX::ScratchImage ReadGLTFTexture(const Microsoft::glTF::Document& document, const Microsoft::glTF::Texture* texture, std::wstring& name)
	{
		const Microsoft::glTF::Image* image = &document.images.Get(texture->imageId);
		name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(image->name);
		std::vector<uint8_t> imageData = PPK::GLTFReader::m_gltfResourceReader->ReadBinaryData(document, *image);

		DirectX::TexMetadata info;
		constexpr bool treatAsLinear = true;
		DirectX::ScratchImage scratchImage;
		if (FAILED(DirectX::LoadFromDDSMemory(imageData.data(), imageData.size(), DirectX::DDS_FLAGS_NONE, &info, scratchImage)))
		{
			// DDS failed, try WIC
			// Note: try DDS first since WIC can load some DDS (but not all), so we wouldn't want to get 
			// a partial or invalid DDS loaded from WIC.
			if (FAILED(DirectX::LoadFromWICMemory(imageData.data(), imageData.size(), treatAsLinear ? DirectX::WIC_FLAGS_IGNORE_SRGB : DirectX::WIC_FLAGS_NONE, &info, scratchImage)))
			{
				throw Microsoft::glTF::GLTFException("Failed to load image - Image could not be loaded as DDS or read by WIC.");
			}
		}

		return scratchImage;
		/*if (info.format == DXGI_FORMAT_R32G32B32A32_FLOAT && treatAsLinear)
		{
			return scratchImage;
		}
		else
		{
			DirectX::ScratchImage converted;
			if (FAILED(DirectX::Convert(*scratchImage.GetImage(0, 0, 0), DXGI_FORMAT_R32G32B32A32_FLOAT, treatAsLinear ? DirectX::TEX_FILTER_DEFAULT : DirectX::TEX_FILTER_SRGB_IN, DirectX::TEX_THRESHOLD_DEFAULT, converted)))
			{
				throw Microsoft::glTF::GLTFException("Failed to convert texture to DXGI_FORMAT_R32G32B32A32_FLOAT for processing.");
			}
	
			return converted;
		}*/
	}

	static void LoadFromGLTFMaterial(Material& material, const Microsoft::glTF::Document& document, const Microsoft::glTF::Material* gltfMaterial)
	{
		// Load and initialize texture resources from GLTF material
		const std::array<std::string, TextureSlot::COUNT> gltfTexturesId = {
			gltfMaterial->metallicRoughness.baseColorTexture.textureId,
			gltfMaterial->metallicRoughness.metallicRoughnessTexture.textureId,
			gltfMaterial->normalTexture.textureId,
			gltfMaterial->occlusionTexture.textureId,
			gltfMaterial->emissiveTexture.textureId
		};

		constexpr std::array<const wchar_t*, TextureSlot::COUNT> slotNames = {
			L"_BaseColor",
			L"_MetallicRoughness",
			L"_Normal",
			L"_Occlusion",
			L"_Emissive"
		};

		for (int slot = 0; slot < TextureSlot::COUNT; slot++)
		{
			// Upload texture
			std::wstring textureName;

			if (!gltfTexturesId[slot].empty())
			{
				const Microsoft::glTF::Texture* gltfTexture = &document.textures.Get(gltfTexturesId[slot]);
				DirectX::ScratchImage scratchImage = ReadGLTFTexture(document, gltfTexture, textureName);
				textureName += slotNames[slot];
				//defaultSampler = RHI::Sampler::CreateSampler();
				// TODO: Handle mips/slices/depth
				std::shared_ptr<PPK::RHI::Texture> texture = PPK::RHI::Texture::CreateTextureResource(
					scratchImage.GetMetadata(),
					textureName.c_str(),
					scratchImage.GetImage(0, 0, 0)
				);

				material.SetTexture(texture, static_cast<TextureSlot>(slot));
			}
		}
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
			Entity entity = m_numEntities++;
			CameraComponent::CameraDescriptor cameraDescriptor;
			cameraDescriptor.m_cameraInternals.m_aspectRatio = ASPECT_RATIO;
			// cameraDescriptor.m_transform = nodeGlobalTransform;

			// Hardcoded internal camera parameters. To load from gltf scene, use document.cameras[node.cameraId]
			static uint32_t cameraIdx = 0;
			m_componentManager.AddComponent<CameraComponent>(entity, std::move(CameraComponent{cameraIdx++}));
			m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{nodeGlobalTransform}));
			m_renderingSystem.UpdateCameraMatrices(cameraDescriptor,
				m_componentManager.GetComponent<CameraComponent>(entity).value(),
				m_componentManager.GetComponent<TransformComponent>(entity).value());
		}

		// Create mesh if node has mesh
		if (!node.meshId.empty())
		{
			Entity entity = m_numEntities++;

			// Todo make renderer static so that it's called where necessary without having to carry it unused over many func calls
			MeshComponent::MeshBuildData* meshBuildData = CreateFromGltfMesh(document, document.meshes[node.meshId]);
			// Hardcoded to singl primitive per mesh. TODO: Add support for segments
			// for gltfMesh.primitives ... etc
			const Microsoft::glTF::Material* gltfMaterial = &document.materials.Get(document.meshes[node.meshId].primitives[0].materialId);
			Material material = Material();
			LoadFromGLTFMaterial(material, document, gltfMaterial);
			m_componentManager.AddComponent<MeshComponent>(entity, std::move(m_renderingSystem.CreateMeshComponent(meshBuildData, material, entity)));
			m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{nodeGlobalTransform}));
		}

		return nodeGlobalTransform;
	}

	void Scene::OnUpdate(float deltaTime)
	{
		for (int entity = 0; entity < m_numEntities; entity++)
		{
			// Handle camera
			std::optional<CameraComponent>& cameraComponent = m_componentManager.GetComponent<CameraComponent>(entity);
			std::optional<TransformComponent>& transformComponent = m_componentManager.GetComponent<TransformComponent>(entity);
			if (cameraComponent && transformComponent)
			{
				m_renderingSystem.MoveCamera(cameraComponent.value(), transformComponent.value(), deltaTime);
			}

			// Handle meshes
			std::optional<MeshComponent>& meshComponent = m_componentManager.GetComponent<MeshComponent>(entity);
			if (meshComponent && transformComponent && transformComponent.value().m_dirty)
			{
				// Update mesh object buffer
				m_renderingSystem.UpdateConstantBufferData(meshComponent->GetObjectBuffer(),
					(void*)&transformComponent.value().m_objectToWorldMatrix, sizeof(MeshComponent::ObjectData));
				// meshComponent.value().UpdateObjectBuffer(transformComponent.value());
				transformComponent.value().m_dirty = false;
			}
		}
	}

	void Scene::OnRender()
	{
		gRenderer->BeginFrame();
		m_passManager->BeginPasses();

		// Iterate to find cameras
		for (int entity = 0; entity < m_numEntities; entity++)
		{
			if (std::optional<CameraComponent>& cameraComponent = m_componentManager.GetComponent<CameraComponent>(entity))
			{
				// Iterate to find meshes
				// TODO: Figure out better way to index meshes into heaps. This is a bit hacky-ish.
				uint32_t meshIdx = 0;
				for (int meshEntity = 0; meshEntity < m_numEntities; meshEntity++)
				{
					if (std::optional<MeshComponent>& meshComponent = m_componentManager.GetComponent<MeshComponent>(meshEntity))
					{
						m_passManager->RecordPasses(meshComponent.value(), cameraComponent.value(), meshIdx++, TLAS);
					}
				}
			}
		}

		// Render ImGui
		RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);
		ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };

		{
			ComPtr<ID3D12GraphicsCommandList4> commandList = gRenderer->GetCommandContext()->GetCurrentCommandList();
			PIXScopedEvent(commandList.Get(), PIX_COLOR(0x22, 0x22, 0x22), L"ImGui");
			commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gRenderer->GetCommandContext()->GetCurrentCommandList().Get());
		}

		gRenderer->EndFrame();
	}

	void Scene::CreateGPUAccelerationStructure()
	{
		BLAS = m_renderingSystem.BuildBottomLevelAccelerationStructure(m_componentManager.GetComponentTypeSpan<MeshComponent>());
		TLAS = m_renderingSystem.BuildTopLevelAccelerationStructure(BLAS);
	}
}
