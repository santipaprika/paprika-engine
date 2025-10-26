#include <ApplicationHelper.h>
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
#include <execution>
#include <PointLightComponent.h>
#include <TransformUtils.h>

#include <WinPixEventRuntime/pix3.h>
namespace PPK
{
	Scene::Scene()
		: m_numEntities(0), TLAS(nullptr)
	{
		m_renderingSystem = RenderingSystem(&m_componentManager.GetComponentArray<TransformComponent>(),
		                                    &m_componentManager.GetComponentArray<CameraComponent>());
	}

	Scene::~Scene()
	{
		// m_cameraEntity = nullptr;
		// m_lightEntities.clear();
		// m_meshEntities.clear();

		Logger::Verbose("Removing Scene");

		delete gPassManager;
		delete TLAS;
		//Camera::GetCameras().clear();
		// MeshComponent::GetMeshes().clear();
	}

	void Scene::InitializeLights()
	{
		PointLightComponent lightComponent;
		lightComponent.m_renderData.m_color = Vector3(1.f, 1.f, 1.f);
		lightComponent.m_renderData.m_radius = 2.f;
		lightComponent.m_renderData.m_worldPos = Vector3(0.f, 20.f, 0.f);
		lightComponent.m_renderData.m_intensity = 1.f;

		
		m_componentManager.AddComponent(m_numEntities++, std::move(lightComponent));
		m_lightsBuffer = std::move(m_renderingSystem.CreateLightsBuffer(&m_componentManager.GetComponentArray<PointLightComponent>()));
	}

	static MeshComponent::MeshBuildData* CreateFromGltfMesh(const Microsoft::glTF::Document& document,
	                                                        const Microsoft::glTF::MeshPrimitive& gltfMeshPrimitive)
	{
		MeshComponent::MeshBuildData* meshBuildData = new MeshComponent::MeshBuildData();

		meshBuildData->m_indices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
			document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive);

		meshBuildData->m_nIndices = Microsoft::glTF::MeshPrimitiveUtils::GetIndices32(
			document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive).size();

		if (gltfMeshPrimitive.HasAttribute("POSITION"))
		{
			meshBuildData->m_vertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
				document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive);

			meshBuildData->m_nVertices = Microsoft::glTF::MeshPrimitiveUtils::GetPositions(
				document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive).size() / 3;
		}
		if (gltfMeshPrimitive.HasAttribute("TEXCOORD_0"))
		{
			meshBuildData->m_uvs = Microsoft::glTF::MeshPrimitiveUtils::GetTexCoords_0(
				document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive);
		}
		if (gltfMeshPrimitive.HasAttribute("NORMAL"))
		{
			meshBuildData->m_normals = Microsoft::glTF::MeshPrimitiveUtils::GetNormals(
				document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive);
		}
		if (gltfMeshPrimitive.HasAttribute("COLOR"))
		{
			meshBuildData->m_colors = Microsoft::glTF::MeshPrimitiveUtils::GetColors_0(
				document, *GLTFReader::m_gltfResourceReader, gltfMeshPrimitive);
		}

		Timer::BeginTimer();
		Timer::EndAndReportTimer("Load GLTF attributes");

		return meshBuildData;
	}

	// Based on https://github.com/microsoft/glTF-Toolkit/blob/master/glTF-Toolkit/src/GLTFTextureUtils.cpp
	DirectX::ScratchImage ReadGLTFTexture(const Microsoft::glTF::Document& document, const Microsoft::glTF::Texture* texture, std::string& name)
	{
		const Microsoft::glTF::Image* image = &document.images.Get(texture->imageId);
		name = image->name;
		std::vector<uint8_t> imageData = PPK::GLTFReader::m_gltfResourceReader->ReadBinaryData(document, *image);

		DirectX::TexMetadata info;
		constexpr bool treatAsLinear = true;
		DirectX::ScratchImage scratchImage;
		if (FAILED(DirectX::LoadFromDDSMemory(imageData.data(), imageData.size(), DirectX::DDS_FLAGS_NONE, &info, scratchImage)))
		{
			// DDS failed, try WIC
			// Note: try DDS first since WIC can load some DDS (but not all), so we wouldn't want to get 
			// a partial or invalid DDS loaded from WIC.

			// TODO: This is very slow, figure out why, or parallelize.
			// Maybe it's fighting for GPU resources while the gpu is flushing work - Try to batch resource creation.
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

		constexpr std::array<const char*, TextureSlot::COUNT> slotNames = {
			"_BaseColor",
			"_MetallicRoughness",
			"_Normal",
			"_Occlusion",
			"_Emissive"
		};

		for (int slot = 0; slot < TextureSlot::COUNT; slot++)
		{
			// Upload texture
			std::string textureName;

			if (!gltfTexturesId[slot].empty())
			{
				const Microsoft::glTF::Texture* gltfTexture = &document.textures.Get(gltfTexturesId[slot]);
				DirectX::ScratchImage scratchImage = ReadGLTFTexture(document, gltfTexture, textureName);
				textureName += slotNames[slot];
				//defaultSampler = RHI::Sampler::CreateSampler();
				// TODO: Handle mips/slices/depth
				std::shared_ptr<PPK::RHI::Texture> texture = PPK::RHI::CreateTextureResource(
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
		std::for_each(std::execution::par, document.scenes[0].nodes.begin(), document.scenes[0].nodes.end(), [&](const std::string& node) {
			TraverseGLTFNode(document, document.nodes[stoi(node)], Matrix::Identity);
		});

		Entity entity = m_numEntities++;

		// TODO: Hardcoded internal camera parameters. To load from gltf scene, use document.cameras[node.cameraId] in TraverseGLTFNode
		static uint32_t cameraIdx = 0;
		m_componentManager.AddComponent<CameraComponent>(entity, std::move(CameraComponent{cameraIdx++}));
		TransformComponent& transformComponent = m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{}));
		Vector3 StartPosition = {10.3f, 6.4f, 0.8f};
		TransformUtils::RotateAndMove(Vector3(-PI / 12.f, PI / 2.f,0.f), StartPosition, transformComponent.m_renderData.m_objectToWorldMatrix);

		// Camera constant buffer will be updated later, in Scene::OnRender since render state is marked dirty.
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
		Matrix nodeNormalMat = nodeGlobalTransform.Invert().Transpose();

		DirectX::XMFLOAT3X4A nodeNormalTransform = {
			nodeNormalMat._11, nodeNormalMat._12, nodeNormalMat._13, 0.f,
			nodeNormalMat._21, nodeNormalMat._22, nodeNormalMat._23, 0.f,
			nodeNormalMat._31, nodeNormalMat._32, nodeNormalMat._33, 0.f
		};
		
		// Create mesh if node has mesh
		if (!node.meshId.empty())
		{
			// TODO: Probably not worth parallelizing small sections - consider doing custom work distribution.
			std::for_each(std::execution::par, document.meshes[node.meshId].primitives.begin(), document.meshes[node.meshId].primitives.end(), [&](const Microsoft::glTF::MeshPrimitive& primitive)
			{
				const Microsoft::glTF::Material* gltfMaterial = &document.materials.Get(primitive.materialId);
				// Don't create mesh if material has no base color - it's probably translucent which is not supported yet
				if (gltfMaterial->metallicRoughness.baseColorTexture.textureId.empty())
				{
					return;
				}

				Entity entity = m_numEntities++; // atomic
				MeshComponent::MeshBuildData* meshBuildData = CreateFromGltfMesh(document, primitive);
				
				Material material(document, gltfMaterial);
				const auto& transformComponent = m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{nodeGlobalTransform, nodeNormalTransform}));
				m_componentManager.AddComponent<MeshComponent>(entity, std::move(m_renderingSystem.CreateMeshComponent(meshBuildData, transformComponent, material, entity, node.name + "_" + gltfMaterial->name)));
			});
		}

		return nodeGlobalTransform;
	}

	// UNUSED ATM
	void Scene::CreateGridMesh()
	{
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

		Entity entity = m_numEntities++;

		{
			DirectX::ScratchImage scratchImage = LoadTextureFromDisk(GetAssetFullFilesystemPath("Textures/checkerboard.png"));
			// TODO: Handle mips/slices/depth
			std::shared_ptr<PPK::RHI::Texture> texture = PPK::RHI::CreateTextureResource(
				scratchImage.GetMetadata(),
				"Checkerboard",
				scratchImage.GetImage(0, 0, 0)
			);
			Material material = Material();
			material.SetTexture(texture, BaseColor);

			// LoadFromGLTFMaterial(material, document, gltfMaterial);
			m_componentManager.AddComponent<MeshComponent>(entity, std::move(m_renderingSystem.CreateMeshComponent(meshData, {}, material, entity, "Ground")));
		}
		m_componentManager.AddComponent<TransformComponent>(entity, std::move(TransformComponent{}));
	}

	void Scene::InitializeScene(const Microsoft::glTF::Document& document)
	{
		// Load meshes, materials, and textures from GLTF scene
		ImportGLTFScene(document);
		InitializeLights();

		gRenderer->WaitForGpu();

		CreateGPUAccelerationStructure();
		
		// Initialize and add lights
		// ...

		// Create pass manager (this adds all passes in order)
		gPassManager = new PassManager();

		// Init scene render data and allocate descriptors to shader-visible heap
		for (CameraComponent& cameraComponent : m_componentManager.GetComponentSpan<CameraComponent>())
		{
			cameraComponent.InitScenePassData();
		}

		for (MeshComponent& meshComponent : m_componentManager.GetComponentSpan<MeshComponent>())
		{
			meshComponent.InitScenePassData();
		}

		Logger::Info("Scene initialized successfully!");
	}

	void Scene::OnUpdate(float deltaTime)
	{
		SCOPED_TIMER("Scene::OnUpdate")

		// Should go to CameraSystem
		std::span<CameraComponent> cameraComponentSpan = m_componentManager.GetComponentSpan<CameraComponent>();
		for (int i = 0; i < cameraComponentSpan.size(); i++)
		{
			CameraComponent& cameraComponent = cameraComponentSpan[i];
			Entity entity = m_componentManager.GetEntityFromComponentIndex<CameraComponent>(i);
			TransformComponent& transformComponent = m_componentManager.GetComponent<TransformComponent>(entity);
			m_controllerSystem.MoveCamera(cameraComponent, transformComponent, deltaTime);
		}

		// Should go to Mesh System
		std::span<MeshComponent> meshComponentSpan = m_componentManager.GetComponentSpan<MeshComponent>();
		for (int i = 0; i < meshComponentSpan.size(); i++)
		{
			MeshComponent& meshComponent = meshComponentSpan[i];
			Entity entity = m_componentManager.GetEntityFromComponentIndex<CameraComponent>(i);
			TransformComponent& transformComponent = m_componentManager.GetComponent<TransformComponent>(entity);

			if (transformComponent.m_dirty)
			{
				// Update mesh object buffer
				RHI::ConstantBufferUtils::UpdateConstantBufferData(meshComponent.GetObjectBuffer(),
																   (void*)&transformComponent.m_renderData, sizeof(MeshComponent::ObjectData));
				// meshComponent.value().UpdateObjectBuffer(transformComponent.value());
				transformComponent.m_dirty = false;
			}
		}
	}

	void Scene::OnRender()
	{
		gRenderer->BeginFrame();

		std::shared_ptr<RHI::CommandContext> renderContext = gRenderer->GetCommandContext();

		{
			// Different passes use the same shader-visible heap for the whole frame so we can set it once here
			RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, renderContext->GetFrameIndex());
			ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };
			renderContext->GetCurrentCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		}

		Entity mainCameraId = m_renderingSystem.GetMainCameraId();
		m_renderingSystem.UpdateCameraRenderData(mainCameraId, renderContext->GetFrameIndex());

		SceneRenderContext sceneRenderContext;
		sceneRenderContext.m_mainCameraRdhIndex = m_renderingSystem.GetCameraIndexInResourceDescriptorHeap(mainCameraId, renderContext->GetFrameIndex());
		sceneRenderContext.m_lightsRdhIndex = m_lightsBuffer.GetIndexInRDH(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		gPassManager->RecordPasses(sceneRenderContext);

		// Render ImGui
		RHI::ShaderDescriptorHeap* cbvSrvHeap = gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);
		ID3D12DescriptorHeap* ppHeaps[] = { cbvSrvHeap->GetHeap() /*, Sampler heap would go here */ };

		{
			SCOPED_TIMER("Scene::OnRender::ImGui")
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
		BLAS = m_renderingSystem.BuildBottomLevelAccelerationStructure(m_componentManager.GetComponentSpan<MeshComponent>());
		TLAS = m_renderingSystem.BuildTopLevelAccelerationStructure(BLAS);
	}

	PointLightComponent& Scene::GetFirstLightComponent()
	{
		return m_componentManager.GetFirstComponentOfType<PointLightComponent>();
	}
}
