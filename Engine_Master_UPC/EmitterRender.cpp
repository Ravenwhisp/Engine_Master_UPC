#include "Globals.h"
#include "EmitterRender.h"
#include <imgui.h>

bool EmitterRender::drawUi()
{
    bool parameterChanged = false;

    if (ImGui::CollapsingHeader("Render"))
    {
        const char* options[] = { "Billboard", "Horizontal", "Vertical" };
        int currentIndex = static_cast<int>(m_renderMode);
        if (ImGui::Combo("Orientation##Render", &currentIndex, options, IM_ARRAYSIZE(options)))
        {
            m_renderMode = static_cast<RenderMode>(currentIndex);
            parameterChanged = true;
        }


        {
            int layer = static_cast<int>(m_layer);
            if (ImGui::DragInt("Render layer##Render", &layer, 1.f))
            {
                m_layer = static_cast<uint32_t>(layer);
                parameterChanged = true;
            }
        }

        {
            const char* blendOptions[] = { "Alpha Blend", "Additive" };
            int currentBlendIndex = static_cast<int>(m_blendMode);
            if (ImGui::Combo("Blend Mode##Render", &currentBlendIndex, blendOptions, IM_ARRAYSIZE(blendOptions)))
            {
                m_blendMode = static_cast<BlendMode>(currentBlendIndex);
                parameterChanged = true;
            }
        }
    }

    return parameterChanged;
}

void EmitterRender::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);

    archive.serializeStringEnum(m_renderMode, "RenderMode", 
        [](uint32_t v) -> const char* {
            switch (static_cast<RenderMode>(v)) {
            case RenderMode::BILLBOARD:  return "BILLBOARD";
            case RenderMode::HORIZONTAL: return "HORIZONTAL";
            case RenderMode::VERTICAL:   return "VERTICAL";
            default: return "BILLBOARD";
            }
        },
        [](const char* s) -> uint32_t {
            if (std::strcmp(s, "BILLBOARD") == 0)  return 0;
            if (std::strcmp(s, "HORIZONTAL") == 0) return 1;
            if (std::strcmp(s, "VERTICAL") == 0)   return 2;
            return 0;
        });

    archive.serialize(m_layer, "RenderLayer");

    archive.serializeStringEnum(m_blendMode, "BlendMode",
        [](uint32_t v) -> const char* {
            switch (static_cast<BlendMode>(v)) {
            case BlendMode::ALPHA:    return "ALPHA";
            case BlendMode::ADDITIVE: return "ADDITIVE";
            default: return "ALPHA";
            }
        },
        [](const char* s) -> uint32_t {
            if (std::strcmp(s, "ALPHA") == 0)    return 0;
            if (std::strcmp(s, "ADDITIVE") == 0) return 1;
            return 0;
        });
}