#pragma once

#include <ApplicationHelper.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <Renderer.h>
#include <DxgiHelper.h>

// ----------- IMGUI METHODS ----------------

// Combo box for render targets that we want to expose for debug visualization
inline void VisualizeRenderTargets()
{
    const char* items[] = { "None", "All", "Shadow Variance", "Scene Color" };
    ImTextureID textureHandles[] = {
        gPassManager->m_basePass.m_shadowVarianceTargetHandle.ptr,
        gPassManager->m_denoisePpfxPass.m_denoisePassData[0].m_denoiseResourcesHandle[0].ptr // 3 handles, but keep only first one 'Scene Color' 
    };
        
    static int itemSelectedIdx = 0;
    const char* comboPreviewValue = items[0];
    if (ImGui::BeginCombo("Visualize Texture", comboPreviewValue, 0))
    {
        for (int i = 0; i < IM_ARRAYSIZE(items); i++)
        {
            const bool is_selected = (itemSelectedIdx == i);
            if (ImGui::Selectable(items[i], is_selected))
            {
                itemSelectedIdx = i;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    float displayedTextureWidth = ImGui::GetContentRegionAvail().x;
    float displayedTextureHeight = displayedTextureWidth * VIEWPORT_HEIGHT / VIEWPORT_WIDTH;
    if (itemSelectedIdx == 1)
    {
        for (ImTextureID texHandle : textureHandles)
        {
            ImGui::Image(texHandle, ImVec2(displayedTextureWidth, displayedTextureHeight));
            ImGui::Spacing();
        }
    }
    else if (itemSelectedIdx > 1)
    {
        ImGui::Image(textureHandles[itemSelectedIdx - 2], ImVec2(displayedTextureWidth, displayedTextureHeight));
    }
}

// List all resident resources on the GPU, their total size, and per-category size.
inline void ShowGPUMemory()
{
    if (!ImGui::Begin("PPK GPU Memory")) {
        ImGui::End();
        return;
    }
    
    size_t totalMemory = 0;

    // Sort resources by size descending - pretty expensive but only happens when tree window is open.
    std::vector<std::pair<std::string, PPK::RHI::GPUResource*>> sortedResources(gResourcesMap.begin(), gResourcesMap.end());
    std::sort(sortedResources.begin(), sortedResources.end(), [](const auto& a, const auto& b)
    {
        return a.second->GetSizeInBytes() > b.second->GetSizeInBytes();
    });

    constexpr const char* categoryNames[] = {
        "_BaseColor",
        "_MetallicRoughness",
        "_Normal",
        "_Occlusion",
        "_Emissive",
        "BLASTransform",
        "ObjectCB",
        "VtxBuffer",
        "IdxBuffer",
        "RT_",
        "TLAS"
    };
    uint32_t memoryPerCategory[] = { 0,0,0,0,0,0,0,0,0,0 };
    
    std::for_each(sortedResources.begin(), sortedResources.end(), [&memoryPerCategory, &categoryNames](const auto& resource)
    {
        for (int i = 0; i < _countof(memoryPerCategory); i++)
        {
            if (resource.first.starts_with(categoryNames[i]) || resource.first.ends_with(categoryNames[i]))
            {
                memoryPerCategory[i] += resource.second->GetSizeInBytes();
            }
        }
    });

    // Sort resources by size descending - pretty expensive but only happens when tree window is open.
    for (const auto& [name, resource] : gResourcesMap) {
        totalMemory += resource->GetSizeInBytes();
    }

    ImGui::Text("Total GPU Memory: %s", HumanReadableSize(totalMemory).c_str());
    ImGui::Separator();
    for (int i = 0; i < _countof(memoryPerCategory); i++)
    {
        ImGui::Text((std::string(categoryNames[i]) + ": %s").c_str(), HumanReadableSize(memoryPerCategory[i]).c_str());
    }
    ImGui::Separator();

    for (const auto& [name, resource] : sortedResources) {
        std::string label = name + " (" + HumanReadableSize(resource->GetSizeInBytes()) + ")";
        if (ImGui::TreeNode(label.c_str())) {
            D3D12_RESOURCE_DESC desc = resource->GetResource()->GetDesc();
            ImGui::Text("Format: %s", GetDxgiFormatString(resource->GetResource()->GetDesc().Format));
            ImGui::Text("Dims: %sx%sx%s [Mips:%s][Samples:%s]", std::to_string(desc.Width).c_str(),
                std::to_string(desc.Height).c_str(), std::to_string(desc.DepthOrArraySize).c_str(),
                std::to_string(desc.MipLevels).c_str(), std::to_string(desc.SampleDesc.Count).c_str());
            ImGui::Text("GPU Size: %s", HumanReadableSize(resource->GetSizeInBytes()).c_str());
            // Add more detailed info here if needed
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

// List all registered scope timings sorted
inline void ShowProfilerWindow()
{
    if (!ImGui::Begin("PPK Scope Timings")) {
        ImGui::End();
        return;
    }

    // Convert map to vector and sort by descending time
    // Pretty naive and expensive but only happens when tree window is open.
    std::vector<std::pair<std::string, float>> sorted;
    for (const auto& [name, times] : gTimePerScope) {
        sorted.emplace_back(name, times.GetAverage());
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return b.second < a.second;
    });

#ifdef PPK_DEBUG
    constexpr float debugPerfFactor = 2.f; // allow for double budget on Debug - approx
#else
    constexpr float debugPerfFactor = 1.f;
#endif

    constexpr float budgetPerScope = 200.f * debugPerfFactor; // 0.2ms

    float fullBarWidth = ImGui::GetContentRegionAvail().x;
    float barHeight = 20.0f;
    float spacing = 4.0f;
    for (const auto& [name, time] : sorted) {
        ImGui::Text("[%.2f us]", time);
        ImGui::SameLine(100);
        ImGui::Text("%s", name.c_str());

        // Horizontal bar
        float budgetPercentage = std::min(1.f, time / budgetPerScope);
        constexpr float rightMargin = 10;
        float barWidth = budgetPercentage * (fullBarWidth - rightMargin);
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + barWidth, p0.y + barHeight);
        ImU32 color = IM_COL32((budgetPercentage) * 255, (1.f - budgetPercentage) * 255, 0, 255);
        ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, color, 3.0f);

        ImGui::Dummy(ImVec2(fullBarWidth, barHeight + spacing));
    }

    ImGui::End();
}