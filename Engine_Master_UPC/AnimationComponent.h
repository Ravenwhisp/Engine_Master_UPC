#pragma once

#include "Component.h"
#include "AnimationController.h"
#include "MD5Fwd.h"

#include <memory>

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

    void setAnimationUID(const MD5Hash& uid);
    const MD5Hash& getAnimationUID() const { return m_animationUID; }

    AnimationController& getController() { return m_controller; }
    const AnimationController& getController() const { return m_controller; }

private:
    bool ensureAnimationLoaded();
    void startPlaybackIfNeeded();

    void applyRecursive(GameObject* go);
    void forceWorldRecursive(GameObject* go);

    void debugDrawRecursive(GameObject* go);
    void drawAxisTriad(const Matrix& worldMatrix, float axisLength);

private:
    MD5Hash m_animationUID = INVALID_ASSET_ID;

    std::shared_ptr<AnimationAsset> m_animationAsset;
    AnimationController m_controller;

    bool m_loop = true;
    bool m_playOnStart = true;
    bool m_applyScale = false;
    bool m_forceWorldAfterApply = true;
    bool m_hasStartedPlayback = false;

    std::string m_animationUIDInput;

    bool m_debugDrawHierarchy = false;
};
