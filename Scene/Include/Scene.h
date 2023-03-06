#pragma once

#include <GLTFSDK/Document.h>

#include <Entities/MeshEntity.h>
#include <Entities/CameraEntity.h>
#include <Entities/LightEntity.h>

namespace PPK
{
    class Scene
    {
    public:
        Scene() = default;

        void InitializeScene(const Microsoft::glTF::Document& document, float windowAspectRatio);

    private:
        std::vector<std::shared_ptr<MeshEntity>> m_meshEntities;
        std::vector<std::shared_ptr<LightEntity>> m_lightEntities;

        std::shared_ptr<CameraEntity> m_cameraEntity;
    };
}
