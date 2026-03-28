#pragma once

#include "Component.h"
#include "AnimationController.h"
#include "MD5Fwd.h"

#include <memory>
#include "AnimationStateMachineAsset.h"
class AnimationAsset;
class GameObject;

class AnimationComponent final : public Component
{
public:
    AnimationComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void update() override;
    bool cleanUp() override;

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

    void setStateMachineUID(const MD5Hash& uid);
    const MD5Hash& getStateMachineUID() const { return m_stateMachineUID; }

    bool SendTrigger(const std::string& triggerName);

    AnimationController& getController() { return m_controller; }
    const AnimationController& getController() const { return m_controller; }

private:

    struct FadingPlayback
    {
        std::string stateName;
        std::shared_ptr<AnimationAsset> animationAsset;
        AnimationController controller;

        float fadeTime = 0.0f;
        float transitionTime = 0.0f;

        std::unique_ptr<FadingPlayback> next;
    };

    bool ensureStateMachineLoaded();
    void startStateMachineIfNeeded();
    void resetRuntime();

    const AnimationStateMachineClip* findClipByName(const std::string& clipName) const;
    const AnimationStateMachineState* findStateByName(const std::string& stateName) const;
    const AnimationStateMachineTransition* findTransitionByTrigger(const std::string& triggerName) const;

    bool activateState(const std::string& stateName, bool autoPlay, float transitionTimeSeconds);
    void applyRecursive(GameObject* go);
    void forceWorldRecursive(GameObject* go);

    void pushCurrentPlaybackToHistory(float transitionTimeSeconds);

    void updateFadingPlaybackRecursive(FadingPlayback* playback, float deltaTimeSeconds);
    bool samplePlaybackRecursive(const std::string& channelName, AnimationSample& outSample) const;
    bool samplePlaybackNodeRecursive(const FadingPlayback* playback, const std::string& channelName, AnimationSample& outSample) const;

    void blendSamples(const AnimationSample& fromSample, const AnimationSample& toSample, float weight, AnimationSample& outSample) const;
    void drawStateMachineResourceUi();
    void drawClipsUi();
    void drawStatesUi();
    void drawTransitionsUi();

    void drawStateCombo(const char* label, std::string& value);
    void drawClipCombo(const char* label, std::string& value);

    void sanitizeStateMachineAfterEdit();

    void debugDrawRecursive(GameObject* go);
    void drawAxisTriad(const Matrix& worldMatrix, float axisLength);

private:

    MD5Hash m_stateMachineUID = INVALID_ASSET_ID;

    std::shared_ptr<AnimationStateMachineAsset> m_stateMachineAsset;
    std::shared_ptr<AnimationAsset> m_currentAnimationAsset;
    AnimationController m_controller;

    std::string m_activeStateName;
    float m_currentFadeTime = 0.0f;
    float m_currentTransitionTime = 0.0f;

    std::unique_ptr<FadingPlayback> m_previousPlayback;

    bool m_playOnStart = true;
    bool m_applyScale = false;
    bool m_forceWorldAfterApply = true;
    bool m_hasStartedPlayback = false;

    std::string m_stateMachineUIDInput;
    std::string m_triggerInput;

    bool m_debugDrawHierarchy = false;
};
