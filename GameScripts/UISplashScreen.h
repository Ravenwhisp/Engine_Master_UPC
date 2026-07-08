#pragma once

#include "ScriptAPI.h"

class UISplashScreen : public Script
{
    DECLARE_SCRIPT(UISplashScreen)

public:
    explicit UISplashScreen(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    ScriptComponentRef<Transform2D> buttonGlow;
    ScriptComponentRef<Transform2D> logoGlow;
    ScriptComponentRef<Transform2D> lyrielDeath;
    ScriptComponentRef<Transform2D> particles1;
    ScriptComponentRef<Transform2D> particles2;
    std::string nextSceneName;

private:
	float time = 0.0f;
	Transform2D* m_buttonGlow = nullptr;
	Transform2D* m_logoGlow = nullptr;
	Transform2D* m_lyrielDeath = nullptr;
	Transform2D* m_particles1 = nullptr;
	Transform2D* m_particles2 = nullptr;
};