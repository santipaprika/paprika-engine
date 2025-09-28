#pragma once

#include <stdafx.h>
#include <CameraComponent.h>
#include <MeshComponent.h>
#include <Material.h>
#include <optional>
#include <TransformComponent.h>
#include <typeindex>
#include <span>

#include <EntityUtils.h>

using namespace PPK;

class ComponentManager {
public:
    ComponentManager();

    template<typename T>
    std::vector<std::optional<T>>& GetComponentTypeVector()
    {
        Logger::Assert(m_componentsTypeMap.contains(std::type_index(typeid(T))));
        return *reinterpret_cast<std::vector<std::optional<T>>*>(m_componentsTypeMap.at(std::type_index(typeid(T))));
    }

    template<typename T>
    std::span<std::optional<T>> GetComponentTypeSpan()
    {
        return std::span{GetComponentTypeVector<T>()};
        // assert(m_componentsTypeMap.contains(std::type_index(typeid(T))));
        // return reinterpret_cast<std::span<std::optional<T>>>(m_componentsTypeMap[std::type_index(typeid(T))]);
    }
    
    template<typename T>
    std::optional<T>& AddComponent(Entity entity, T&& component)
    {
        // Resize the component vector if necessary
        std::vector<std::optional<T>>& ComponentArray = GetComponentTypeVector<T>();
        if (entity >= ComponentArray.size()) {
            ComponentArray.resize(entity + 1);
        }
        ComponentArray[entity].emplace(std::forward<T>(component));
        return ComponentArray[entity];
    }

    template<typename T>
    std::optional<T>& GetComponent(Entity entity)
    {
        Logger::Assert(GetComponentTypeSpan<T>().size() > entity, L"Using more entities than initially allocated. Please increase this value in ComponentManager constructor.");

        std::span<std::optional<T>> ComponentArray = GetComponentTypeSpan<T>();
        return ComponentArray[entity];
    }

    template<typename T>
    bool HasComponent(Entity entity)
    {
        return GetComponentTypeSpan<T>().size() > entity && GetComponentTypeSpan<T>()[entity].has_value();
    }

private:
    // Store components in vectors (contiguous memory)
    // optional vs additional validity array? Need to check perf (https://www.reddit.com/r/cpp_questions/comments/r4rll0/vector_or_unordered_map_for_ecs/?rdt=57650) 
    std::vector<std::optional<TransformComponent>> m_transformComponents;
    std::vector<std::optional<MeshComponent>> m_meshComponents;
    std::vector<std::optional<CameraComponent>> m_cameraComponents;
    
    std::unordered_map<std::type_index, void*> m_componentsTypeMap = {
        {std::type_index(typeid(TransformComponent)), &m_transformComponents},
        {std::type_index(typeid(MeshComponent)), &m_meshComponents},
        {std::type_index(typeid(CameraComponent)), &m_cameraComponents}
    };
};
