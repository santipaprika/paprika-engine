#pragma once

#include <Renderer.h>
#include <GLTFSDK/Document.h>
#include <ComponentManager.h>
#include <ControllerSystem.h>

#include <Entities/MeshEntity.h>
#include <Entities/CameraEntity.h>
#include <Entities/LightEntity.h>
#include <PassManager.h>
#include <RenderingSystem.h>

namespace PPK
{
    class Scene
    {
    public:
        explicit Scene();
        ~Scene();

        void InitializeLights();
        void InitializeScene(const Microsoft::glTF::Document& document);
        void ImportGLTFScene(const Microsoft::glTF::Document& document);
        void TraverseGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform);
        Matrix ProcessGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform);
        void CreateGridMesh();
        void OnUpdate(float deltaTime);
        void OnRender();
        void CreateGPUAccelerationStructure();
        PointLightComponent& GetFirstLightComponent();

    private:
        ComponentManager m_componentManager;
        // std::vector<MeshEntity*> m_meshEntities;
        // std::vector<std::shared_ptr<LightEntity>> m_lightEntities;
        //
        // std::shared_ptr<CameraEntity> m_cameraEntity;

        std::atomic<Entity> m_numEntities;
        RenderingSystem m_renderingSystem;
        ControllerSystem m_controllerSystem;

        ComPtr<ID3D12Resource> BLAS;
        RHI::GPUResource* TLAS;
        RHI::ConstantBuffer m_lightsBuffer;

        struct LoadingStatus
        {
            uint32_t m_numNodes;
            std::atomic<uint32_t> m_nodesProcessed;
            void ReportProgress() const;
        } m_loadingStatus;
    };
}
