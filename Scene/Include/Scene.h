#pragma once

#include <Renderer.h>
#include <GLTFSDK/Document.h>

#include <Entities/MeshEntity.h>
#include <Entities/CameraEntity.h>
#include <Entities/LightEntity.h>
#include <PassManager.h>

namespace PPK
{
    class Scene
    {
    public:
        explicit Scene(std::shared_ptr<Renderer> renderer);
        ~Scene();

        void InitializeScene(const Microsoft::glTF::Document& document);
        void ImportGLTFScene(const Microsoft::glTF::Document& document);
        void TraverseGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform);
        Matrix ProcessGLTFNode(const Microsoft::glTF::Document& document, const Microsoft::glTF::Node& node, const Matrix& parentGlobalTransform);
        void OnUpdate(float deltaTime);
        void OnRender();

    private:
        std::vector<std::shared_ptr<MeshEntity>> m_meshEntities;
        std::vector<std::shared_ptr<LightEntity>> m_lightEntities;

        std::shared_ptr<CameraEntity> m_cameraEntity;

        std::shared_ptr<Renderer> m_renderer;
        std::unique_ptr<PassManager> m_passManager;
    };
}
