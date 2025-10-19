#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

class PointLightComponent
{
public:
    struct RenderData
    {
        Vector3 m_worldPos;
        float m_radius;
        Vector3 m_color;
        float m_intensity;
    } m_renderData;
    
    float m_dirty = false;
};
