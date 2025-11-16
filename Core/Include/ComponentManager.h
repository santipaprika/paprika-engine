#pragma once

#include <stdafx.h>
#include <CameraComponent.h>
#include <MeshComponent.h>
#include <TransformComponent.h>
#include <PointLightComponent.h>
#include <Material.h>
#include <optional>
#include <TransformComponent.h>
#include <typeindex>
#include <span>

#include <EntityUtils.h>

using namespace PPK;

#define MAX_ENTITIES 128

// Based on https://austinmorlan.com/posts/entity_component_system
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
};

template<typename T>
class ComponentArray : public IComponentArray
{
public:
    ComponentArray() : m_size(0) {}

    T& AddComponent(Entity entity, T&& component)
    {
        uint32_t newIndex;

        {
            std::lock_guard lock(m_componentArrayMutex);
            // Logger::Assert(!m_entityToIndexMap.contains(entity), "Component added to same entity more than once.");

            // Put new entry at end and update the maps
            newIndex = m_size;
            m_size++;
        }

        // The index is now reserved so no need to write atomically anymore
        m_entityToIndexMap[entity] = newIndex;
        m_indexToEntityMap[newIndex] = entity;
        m_componentArray[newIndex] = std::move(component);

        return m_componentArray[newIndex];
    }

    // Not thread-safe if writes are being done in other threads
    T& GetComponent(Entity entity)
    {
        Logger::Assert(m_entityToIndexMap.contains(entity), "Retrieving non-existent component.");

        // Return a reference to the entity's component
        return m_componentArray[m_entityToIndexMap[entity]];
    }

    T& operator[](Entity entity)
    {
        return GetComponent(entity);
    }

    Entity GetEntityFromComponentIndex(int32_t componentIdx)
    {
        return m_indexToEntityMap[componentIdx];
    }

    std::span<T> GetSpan_ThreadSafe()
    {
        std::lock_guard lock(m_componentArrayMutex);
        return std::span<T>(m_componentArray.data(), m_size);
    }

    T& GetComponentByArrayIndex(uint32_t arrayIndex)
    {
        // TODO: Should have thread safe version?
        return m_componentArray[arrayIndex];
    }

private:
    // The packed array of components (of generic type T),
    // set to a specified maximum amount, matching the maximum number
    // of entities allowed to exist simultaneously, so that each entity
    // has a unique spot.
    // TODO: Use vector if we expect levels with very different densities
    std::array<T, MAX_ENTITIES> m_componentArray;

    // TODO: Should these maps be arrays for better cache usage?
    // Map from an entity ID to an array index.
    std::unordered_map<Entity, size_t> m_entityToIndexMap;

    // Map from an array index to an entity ID.
    std::unordered_map<size_t, Entity> m_indexToEntityMap;

    // Total size of valid entries in the array. Can only be updated by one thread at a time.
    uint32_t m_size;

    std::mutex m_componentArrayMutex;
};

class ComponentManager {
public:
    ComponentManager();

    template<typename T>
    ComponentArray<T>& GetComponentArray()
    {
        Logger::Assert(m_componentArrays.contains(std::type_index(typeid(T))));
        return *static_cast<ComponentArray<T>*>(m_componentArrays.at(std::type_index(typeid(T))));
    }

    template<typename T>
    std::span<T> GetComponentSpan_ThreadSafe()
    {
        return GetComponentArray<T>().GetSpan_ThreadSafe();
    }

    template<typename T>
    T& GetFirstComponentOfType()
    {
        return GetComponentArray<T>().GetComponentByArrayIndex(0);
    }
    
    template<typename T>
    T& AddComponent(Entity entity, T&& component)
    {
        return GetComponentArray<T>().AddComponent(entity, std::forward<T>(component));
    }

    template<typename T>
    T& GetComponent(Entity entity)
    {
        return GetComponentArray<T>().GetComponent(entity);
    }

    template<typename T>
    Entity GetEntityFromComponentIndex(int32_t componentIdx)
    {
        return GetComponentArray<T>().GetEntityFromComponentIndex(componentIdx);
    }

private:
    // Store components in vectors (contiguous memory)
    ComponentArray<TransformComponent> m_transformComponents;
    ComponentArray<MeshComponent> m_meshComponents;
    ComponentArray<CameraComponent> m_cameraComponents;
    ComponentArray<PointLightComponent> m_pointLightComponents;

    std::unordered_map<std::type_index, void*> m_componentArrays = {
        {std::type_index(typeid(TransformComponent)), &m_transformComponents},
        {std::type_index(typeid(MeshComponent)), &m_meshComponents},
        {std::type_index(typeid(CameraComponent)), &m_cameraComponents},
        {std::type_index(typeid(PointLightComponent)), &m_pointLightComponents}
    };
};
