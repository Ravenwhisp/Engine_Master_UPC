#include "pch.h"
#include "UISplashScreen.h"
#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS(UISplashScreen,
    SERIALIZED_COMPONENT_REF(buttonGlow, "Button Glow", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(logoGlow, "Logo Glow", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(lyrielDeath, "Lyriel Death", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(particles1, "Particles 1", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(particles2, "Particles 2", ComponentType::TRANSFORM2D),
	SERIALIZED_STRING(nextSceneName, "Next Scene Name")
)

UISplashScreen::UISplashScreen(GameObject* owner)
    : Script(owner)
{
}

void UISplashScreen::Start()
{
	m_buttonGlow = buttonGlow.getReferencedComponent();
	m_logoGlow = logoGlow.getReferencedComponent();
	m_lyrielDeath = lyrielDeath.getReferencedComponent();
	m_particles1 = particles1.getReferencedComponent();
	m_particles2 = particles2.getReferencedComponent();
}

void UISplashScreen::Update()
{
    if (Input::isFaceButtonLeftJustPressed() ||
        Input::isFaceButtonRightJustPressed() ||
        Input::isFaceButtonTopJustPressed() ||
        Input::isFaceButtonBottomJustPressed() ||
        Input::isLeftShoulderJustPressed() ||
        Input::isRightShoulderJustPressed() ||
        Input::isLeftTriggerJustPressed() ||
        Input::isRightTriggerJustPressed() ||
        Input::isLeftStickJustPressed() ||
        Input::isRightStickJustPressed() ||
        Input::getMoveAxis() != Vector2::Zero ||
        Input::getLookAxis() != Vector2::Zero)
    {
        Input::setPlayerKeyboard(0);
        Input::setPlayerGamepad(1, 0);
        if (!nextSceneName.empty())
        {
            SceneAPI::requestSceneChange(nextSceneName.c_str());
        }
    }
    if (Input::isFaceButtonLeftJustPressed(1) ||
        Input::isFaceButtonRightJustPressed(1) ||
        Input::isFaceButtonTopJustPressed(1) ||
        Input::isFaceButtonBottomJustPressed(1) ||
        Input::isLeftShoulderJustPressed(1) ||
        Input::isRightShoulderJustPressed(1) ||
        Input::isLeftTriggerJustPressed(1) ||
        Input::isRightTriggerJustPressed(1) ||
        Input::isLeftStickJustPressed(1) ||
        Input::isRightStickJustPressed(1) ||
        Input::getMoveAxis(1) != Vector2::Zero ||
        Input::getLookAxis(1) != Vector2::Zero)
    {
        Input::setPlayerGamepad(0, 0);
        Input::setPlayerGamepad(1, 1);
        if (!nextSceneName.empty())
        {
			SceneAPI::requestSceneChange(nextSceneName.c_str());
        }
    }

	time += Time::getDeltaTime();

    if (m_buttonGlow)
    {
		Transform2DAPI::setAlpha(m_buttonGlow, std::abs(std::sin(time * 2.0f)));
    }
    if (m_logoGlow)
    {
        const float t = (std::sin(time * 2.4f) + 1.0f) * 0.5f;
        const float alpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInQuad, t);
        Transform2DAPI::setAlpha(m_logoGlow, alpha);
    }
    if (m_lyrielDeath)
    {
		// Figure-8 movement
        const Vector2 offset = Vector2(
            std::sin(time * 0.8f) * 8.0f, //w-speed // offset // width
			std::sin(time * 0.8f * 2.0f) * 4.0f //h-speed // offset // height
        );
        Transform2DAPI::setPosition(m_lyrielDeath, offset);

        // Breathing scalar
        const float t = (std::sin(time * 1.2f /*speed*/) + 1.0f) * 0.5f;
        const float scale = MathAPI::lerp(0.97f, 1.03f, t);
        Transform2DAPI::setScale(m_lyrielDeath, Vector2(scale, scale));
    }
    if (m_particles1)
    {
        const Vector2 offset = Vector2(
            std::sin(time * 0.3f) * 15.0f,
            std::sin(time * 0.3f * 2.0f) * 20.0f
        );
        Transform2DAPI::setPosition(m_particles2, offset);

        const float t = (std::sin(time * 1.2f) + 1.0f) * 0.5f;
        const float alpha = MathAPI::lerp(0.4f, 0.85f, t);
		Transform2DAPI::setAlpha(m_particles1, alpha);
    }
    if (m_particles2)
    {
        const Vector2 offset = Vector2(
            std::sin(time * 0.3f + 0.4f) * 3.0f,
            std::sin(time * 0.3f * 2.0f + 0.6f) * 5.0f
        );
        Transform2DAPI::setPosition(m_particles1, offset);

        const float t = (std::sin(time * 0.8f + 0.2f) + 1.0f) * 0.5f;
        const float alpha = MathAPI::lerp(0.35f, 0.55f, t);
        Transform2DAPI::setAlpha(m_particles2, alpha);
    }
}

IMPLEMENT_SCRIPT(UISplashScreen)