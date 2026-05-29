#pragma once

#include "ScriptAPI.h"

class ComponentSoundSource;

class MusicSystemTester : public Script
{
    DECLARE_SCRIPT(MusicSystemTester)

public:
    explicit MusicSystemTester(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    enum class TestStep
    {
        None,
        Playing,
        Paused,
        Resumed,
        Stopped
    };

private:
    void playMusicEvent();
    void pauseMusicEvent();
    void resumeMusicEvent();
    void stopMusicEvent();

private:
    ComponentSoundSource* m_soundSource = nullptr;

    std::string m_bankName = "YourBankName";
    std::string m_eventName = "YourEventName";

    uint32_t m_playingID = 0;

    float m_timer = 0.0f;

    float m_pauseAfterSeconds = 3.0f;
    float m_resumeAfterSeconds = 6.0f;
    float m_stopAfterSeconds = 9.0f;

    bool m_playOnStart = true;

    TestStep m_currentStep = TestStep::None;
};