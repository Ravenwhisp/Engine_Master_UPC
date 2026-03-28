#pragma once
#include "Asset.h"
#include "MD5Fwd.h"

#include <string>
#include <vector>

class ImporterAnimationStateMachine;
class ImporterGltf;

struct AnimationStateMachineClip
{
    std::string name;
    MD5Hash animationUID = INVALID_ASSET_ID;
    bool loop = true;
};

struct AnimationStateMachineState
{
    std::string name;
    std::string clipName;
    float speed = 1.0f;
};

struct AnimationStateMachineTransition
{
    std::string sourceStateName;
    std::string targetStateName;
    std::string triggerName;
    float blendTimeSeconds = 0.0f;
};

class AnimationStateMachineAsset : public Asset
{
public:
    friend class ImporterAnimationStateMachine;
    friend class ImporterGltf;

    AnimationStateMachineAsset() = default;
    explicit AnimationStateMachineAsset(MD5Hash id)
        : Asset(id, AssetType::ANIMATION_STATE_MACHINE)
    {
    }

    const std::string& getName() const { return m_name; }
    const std::string& getDefaultStateName() const { return m_defaultStateName; }

    const std::vector<AnimationStateMachineClip>& getClips() const { return m_clips; }
    const std::vector<AnimationStateMachineState>& getStates() const { return m_states; }
    const std::vector<AnimationStateMachineTransition>& getTransitions() const { return m_transitions; }

private:
    std::string m_name;
    std::string m_defaultStateName;

    std::vector<AnimationStateMachineClip> m_clips;
    std::vector<AnimationStateMachineState> m_states;
    std::vector<AnimationStateMachineTransition> m_transitions;
};