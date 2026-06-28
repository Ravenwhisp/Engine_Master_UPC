#include "pch.h"
#include "PlayerDownState.h"
#include "Damageable.h"
#include "PlayerState.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerDownState,
    SERIALIZED_FLOAT(m_selfReviveTime, "Self Revive Time", 0.1f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_assistRadius, "Assist Radius", 0.0f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_assistSpeedMultiplier, "Assist Speed Multiplier", 1.0f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_reviveHp, "Revive HP", 1.0f, 999999.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_teammateTransform, "Teammate Transform", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_downedSprite, "Downed Sprite Transform", ComponentType::TRANSFORM)
)

PlayerDownState::PlayerDownState(GameObject* owner)
    : Script(owner)
{
}

void PlayerDownState::Start()
{
    m_damageable = GameObjectAPI::findScript<Damageable>(getOwner());

    if (!m_damageable)
    {
        Debug::warn("PlayerDownState on '%s' could not find a Damageable on the same GameObject.", GameObjectAPI::getName(m_owner));
    }

    m_playerState = GameObjectAPI::findScript<PlayerState>(getOwner());

    if (!m_playerState)
    {
        Debug::warn("PlayerDownState on '%s' could not find PlayerState on the same GameObject.", GameObjectAPI::getName(m_owner));
    }

    setupUI();
}

void PlayerDownState::Update()
{
    if (m_reviveBlocked)
    {
        return;
    }

    if (!isDowned())
    {
        return;
    }

    if (!m_damageable)
    {
        return;
    }

    float speedMultiplier = 1.0f;
    if (isTeammateInAssistRange()) 
    {
        speedMultiplier *= m_assistSpeedMultiplier;
    }

    m_reviveProgress += Time::getDeltaTime() * speedMultiplier;

    if (m_reviveProgress >= m_selfReviveTime)
    {
        completeRevive();
    }

    updateReviveUI();
}

void PlayerDownState::drawGizmo()
{
    Transform* transform = GameObjectAPI::getTransform(m_owner);
    Vector3 position = TransformAPI::getGlobalPosition(transform);

    Vector3 circleColor = Vector3(0.0f, 1.0f, 0.0f);
    if (isDowned())
    {
        circleColor = Vector3(1.0f, 1.0f, 0.0f);
    }

    DebugDrawAPI::drawCircle(position, Vector3(0.0f, 1.0f, 0.0f), circleColor, m_assistRadius, 32.0f, 0, true);
}

void PlayerDownState::enterDownState()
{
    if (isDowned())
    {
        return;
    }

    m_reviveBlocked = false;

    if (m_playerState)
    {
        m_playerState->setState(PlayerStateType::Downed);
    }

    m_reviveProgress = 0.0f;

    if (m_damageable)
    {
        m_damageable->setInvulnerable(true);
    }

    if (m_downedSpriteTransform)
    {
        GameObjectAPI::setActive(m_downedSpriteTransform->getOwner(), true);
	}

    if (m_reviveSliderCanvas)
    {
		GameObjectAPI::setActive(m_reviveSliderCanvas->getOwner(), true);
	}
    if (m_reviveHandleCanvas)
    {
		GameObjectAPI::setActive(m_reviveHandleCanvas->getOwner(), true);
    }
    if (m_reviveIconCanvas)
    {
		GameObjectAPI::setActive(m_reviveIconCanvas->getOwner(), true);
    }

    Debug::log("%s entered down state.", GameObjectAPI::getName(m_owner));
}

void PlayerDownState::enterDefeatedState()
{
    blockRevive();

    if (m_damageable)
    {
        m_damageable->kill();
    }
}

bool PlayerDownState::isDowned() const
{
    return m_playerState && m_playerState->isDowned();
}

float PlayerDownState::getReviveProgress() const
{
    if (m_selfReviveTime <= 0.0f)
    {
        return 1.0f;
    }

    float progress = m_reviveProgress/m_selfReviveTime;

    if (progress < 0.0f)
    {
        return 0.0f;
    }

    if (progress > 1.0f)
    {
        return 1.0f;
    }

    return progress;
}

void PlayerDownState::blockRevive()
{
    m_reviveBlocked = true;
    m_reviveProgress = 0.0f;
}

bool PlayerDownState::isTeammateInAssistRange() const
{
    Transform* ownTransform = GameObjectAPI::getTransform(m_owner);
    Transform* teammateTransform = m_teammateTransform.getReferencedComponent();

    if (!ownTransform || !teammateTransform)
    {
        return false;
    }

    Vector3 ownPosition = TransformAPI::getGlobalPosition(ownTransform);
    Vector3 teammatePosition = TransformAPI::getGlobalPosition(teammateTransform);

    float distance = (teammatePosition - ownPosition).Length();

    return distance <= m_assistRadius;
}

void PlayerDownState::completeRevive()
{
    if (m_reviveBlocked)
    {
        return;
    }

    m_reviveProgress = 0.0f;

    if (m_playerState)
    {
        m_playerState->setState(PlayerStateType::Normal);
        m_playerState->setUsingAbility(false);
    }

    if (m_damageable)
    {
        m_damageable->setInvulnerable(false);
        m_damageable->revive(m_reviveHp);
    }

    if (m_downedSpriteTransform)
    {
        GameObjectAPI::setActive(m_downedSpriteTransform->getOwner(), false);
    }

    if (m_reviveSliderCanvas)
    {
        GameObjectAPI::setActive(m_reviveSliderCanvas->getOwner(), false);
    }
    if (m_reviveHandleCanvas)
    {
		GameObjectAPI::setActive(m_reviveHandleCanvas->getOwner(), false);
    }
    if (m_reviveIconCanvas)
    {
		GameObjectAPI::setActive(m_reviveIconCanvas->getOwner(), false);
    }

    Debug::log("%s completed revive from down state.", GameObjectAPI::getName(m_owner));
}

void PlayerDownState::setupUI()
{
    Transform* parentTransform = TransformAPI::findChildByName(GameObjectAPI::getTransform(getOwner()), "Revive UI");
    if (parentTransform)
    {
        m_reviveSliderCanvas = TransformAPI::findChildByName(parentTransform, "Ground Canvas");
        if (m_reviveSliderCanvas)
        {
            GameObjectAPI::setActive(m_reviveSliderCanvas->getOwner(), false);

            Transform* sliderTransform = TransformAPI::findChildByName(m_reviveSliderCanvas, "Slider");
            GameObject* sliderGO = sliderTransform->getOwner();
            if (sliderGO)
            {
                m_reviveSlider = static_cast<UISlider*>(GameObjectAPI::getComponent(sliderGO, ComponentType::UISLIDER));
            }
            Transform* slider2Transform = TransformAPI::findChildByName(m_reviveSliderCanvas, "Slider 2");
            GameObject* slider2GO = slider2Transform->getOwner();
            if (slider2GO)
            {
                m_reviveSlider2 = static_cast<UISlider*>(GameObjectAPI::getComponent(slider2GO, ComponentType::UISLIDER));
            }
        }

        m_revivePivotTransform = TransformAPI::findChildByName(parentTransform, "Canvas Pivot");
        if (m_revivePivotTransform)
        {
            m_reviveHandleCanvas = TransformAPI::findChildByName(m_revivePivotTransform, "Handle");
            if (m_reviveHandleCanvas)
            {
                GameObjectAPI::setActive(m_reviveHandleCanvas->getOwner(), false);

                m_reviveHandleTransform = TransformAPI::findChildByName(m_reviveHandleCanvas, "Sprite");
                m_reviveHandle2Transform = TransformAPI::findChildByName(m_reviveHandleCanvas, "Sprite 2");
            }
            m_reviveIconCanvas = TransformAPI::findChildByName(m_revivePivotTransform, "Icon");
            if (m_reviveIconCanvas)
            {
				GameObjectAPI::setActive(m_reviveIconCanvas->getOwner(), false);

                m_reviveIconTransform = TransformAPI::findChildByName(m_reviveIconCanvas, "Sprite");
                m_reviveIcon2Transform = TransformAPI::findChildByName(m_reviveIconCanvas, "Sprite 2");
            }
        }
    }

	m_downedSpriteTransform = m_downedSprite.getReferencedComponent();
    if (m_downedSpriteTransform)
    {
		GameObjectAPI::setActive(m_downedSpriteTransform->getOwner(), false);
    }
    else
    {
        Debug::warn("PlayerDownState on '%s' has no downed sprite transform set up.", GameObjectAPI::getName(m_owner));
	}

    if (!parentTransform)
    {
        Debug::warn("PlayerDownState on '%s' has no revive UI parent transform set up.", GameObjectAPI::getName(m_owner));
    }
    else
    {
        if (!m_revivePivotTransform)
        {
            Debug::warn("PlayerDownState on '%s' has no revive pivot transform set up.", GameObjectAPI::getName(m_owner));
        }
        else
        {
            if (!m_reviveHandleCanvas)
            {
                Debug::warn("PlayerDownState on '%s' has no revive handle canvas set up.", GameObjectAPI::getName(m_owner));
			}
            if (!m_reviveIconCanvas)
            {
                Debug::warn("PlayerDownState on '%s' has no revive icon canvas set up.", GameObjectAPI::getName(m_owner));
            }
            if (!m_reviveIconTransform)
            {
                Debug::warn("PlayerDownState on '%s' has no revive icon transform set up.", GameObjectAPI::getName(m_owner));
            }
            if (!m_reviveHandleTransform)
            {
                Debug::warn("PlayerDownState on '%s' has no revive handle transform set up.", GameObjectAPI::getName(m_owner));
            }
            if (!m_reviveIcon2Transform)
            {
                Debug::warn("PlayerDownState on '%s' has no revive icon 2 transform set up.", GameObjectAPI::getName(m_owner));
            }
            if (!m_reviveHandle2Transform)
            {
                Debug::warn("PlayerDownState on '%s' has no revive handle 2 transform set up.", GameObjectAPI::getName(m_owner));
            }
        }

        if (!m_reviveSlider)
        {
            Debug::warn("PlayerDownState on '%s' has no revive circle slider set up.", GameObjectAPI::getName(m_owner));
        }
        if (!m_reviveSlider2)
        {
            Debug::warn("PlayerDownState on '%s' has no revive circle slider 2 set up.", GameObjectAPI::getName(m_owner));
        }
    }
}

void PlayerDownState::updateReviveUI()
{
    if (!m_reviveSliderCanvas || !m_reviveHandleCanvas || !m_reviveIconCanvas || !m_reviveSlider || !m_reviveSlider2 || !m_revivePivotTransform || !m_reviveIcon2Transform || !m_reviveHandle2Transform)
    {
        return;
    }

    SliderAPI::setFillAmount(m_reviveSlider, 1.0f - getReviveProgress());
    SliderAPI::setFillAmount(m_reviveSlider2, 1.0f - getReviveProgress());

	float angle = getReviveProgress() * 360.0f;
	TransformAPI::setRotationEuler(m_revivePivotTransform, Vector3(0.0f, angle, 0.0f));

    if (isTeammateInAssistRange())
    {
        GameObjectAPI::setActive(m_reviveIconTransform->getOwner(), false);
        GameObjectAPI::setActive(m_reviveHandleTransform->getOwner(), false);
        GameObjectAPI::setActive(m_reviveSlider->getOwner(), false);
		GameObjectAPI::setActive(m_reviveIcon2Transform->getOwner(), true);
        GameObjectAPI::setActive(m_reviveHandle2Transform->getOwner(), true);
        GameObjectAPI::setActive(m_reviveSlider2->getOwner(), true);
    }
     else
    {
        GameObjectAPI::setActive(m_reviveIconTransform->getOwner(), true);
        GameObjectAPI::setActive(m_reviveHandleTransform->getOwner(), true);
        GameObjectAPI::setActive(m_reviveSlider->getOwner(), true);
        GameObjectAPI::setActive(m_reviveIcon2Transform->getOwner(), false);
        GameObjectAPI::setActive(m_reviveHandle2Transform->getOwner(), false);
        GameObjectAPI::setActive(m_reviveSlider2->getOwner(), false);
	}
}

IMPLEMENT_SCRIPT(PlayerDownState)