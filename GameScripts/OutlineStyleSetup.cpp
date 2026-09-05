#include "pch.h"
#include "OutlineStyleSetup.h"

IMPLEMENT_SCRIPT_FIELDS(OutlineStyleSetup,
    SERIALIZED_BOOL(m_enableOutline, "Enable Outline"),
    SERIALIZED_FLOAT(m_intensity, "Intensity", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_thickness, "Thickness (px)", 0.5f, 6.0f, 0.05f),
    SERIALIZED_FLOAT(m_threshold, "Threshold (silhouette)", 0.001f, 0.5f, 0.001f),
    SERIALIZED_FLOAT(m_normalThreshold, "Threshold (creases)", 0.01f, 2.0f, 0.005f),
    SERIALIZED_FLOAT(m_wobble, "Wobble", 0.0f, 4.0f, 0.05f),
    SERIALIZED_FLOAT(m_breakup, "Breakup", 0.0f, 1.0f, 0.01f),
    SERIALIZED_VEC3(m_color, "Color")
)

OutlineStyleSetup::OutlineStyleSetup(GameObject* owner)
    : Script(owner)
{
}

void OutlineStyleSetup::Start()
{
    PostProcessAPI::setOutlineEnabled(m_enableOutline);
    PostProcessAPI::setOutlineIntensity(m_intensity);
    PostProcessAPI::setOutlineThickness(m_thickness);
    PostProcessAPI::setOutlineThreshold(m_threshold);
    PostProcessAPI::setOutlineNormalThreshold(m_normalThreshold);
    PostProcessAPI::setOutlineWobble(m_wobble);
    PostProcessAPI::setOutlineBreakup(m_breakup);
    PostProcessAPI::setOutlineColor(m_color);
}

IMPLEMENT_SCRIPT(OutlineStyleSetup)
