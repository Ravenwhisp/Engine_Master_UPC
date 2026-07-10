#include "pch.h"
#include "HUDFader.h"

HUDFader::HUDFader(GameObject* owner)
    : Script(owner)
{
}

void HUDFader::Start()
{
    findFadeTargets();

    if (m_fadeTargets.empty())
    {
        Debug::warn("HUDFader on '%s' could not find any child Transform2D targets.", GameObjectAPI::getName(getOwner()));
        return;
    }

    m_currentAlpha = Transform2DAPI::getAlpha(m_fadeTargets[0]);
    m_startAlpha = m_currentAlpha;
    m_targetAlpha = m_currentAlpha;
}

void HUDFader::Update()
{
    if (!m_isFading || m_fadeTargets.empty())
    {
        return;
    }

    m_timer += Time::getDeltaTime();

    const float normalizedTime = m_timer >= m_duration ? 1.0f : m_timer / m_duration;
    const float easedTime = MathAPI::smoothStep(0.0f, 1.0f, normalizedTime);

    m_currentAlpha = MathAPI::lerp(m_startAlpha, m_targetAlpha, easedTime);
    applyAlpha(m_currentAlpha);

    if (m_timer >= m_duration)
    {
        m_currentAlpha = m_targetAlpha;
        applyAlpha(m_currentAlpha);

        m_isFading = false;
        m_timer = 0.0f;
    }
}

void HUDFader::fadeTo(float targetAlpha, float duration)
{
    if (m_fadeTargets.empty())
    {
        return;
    }

    m_startAlpha = Transform2DAPI::getAlpha(m_fadeTargets[0]);
    m_currentAlpha = m_startAlpha;
    m_targetAlpha = targetAlpha;

    m_duration = duration;
    m_timer = 0.0f;

    if (m_duration <= 0.0f)
    {
        setAlpha(m_targetAlpha);
        return;
    }

    m_isFading = true;
}

void HUDFader::setAlpha(float alpha)
{
    m_currentAlpha = alpha;
    m_startAlpha = alpha;
    m_targetAlpha = alpha;
    m_timer = 0.0f;
    m_isFading = false;

    applyAlpha(alpha);
}

void HUDFader::findFadeTargets()
{
    m_fadeTargets.clear();

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    const char* targetNames[] =
    {
        "Left Container",
        "Right Container",
        "Center Container"
    };

    for (const char* targetName : targetNames)
    {
        Transform* childTransform = TransformAPI::findChildByName(ownerTransform, targetName);
        if (childTransform == nullptr)
        {
            continue;
        }

        GameObject* childObject = ComponentAPI::getOwner(childTransform);

        Transform2D* childTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(childObject, ComponentType::TRANSFORM2D));

        if (childTransform2D != nullptr)
        {
            m_fadeTargets.push_back(childTransform2D);
        }
    }
}

void HUDFader::applyAlpha(float alpha)
{
    for (Transform2D* target : m_fadeTargets)
    {
        if (target == nullptr)
        {
            continue;
        }

        Transform2DAPI::setAlpha(target, alpha);
    }
}

IMPLEMENT_SCRIPT(HUDFader)