#pragma once
#include "Asset.h"

#include <string>
#include <vector>
#include <AssetReference.h>

struct AnimationStateMachineClip
{
    friend class cereal::access;

    std::string name;
    AssetReference animationRef;
    bool loop = true;

#pragma region Serialization
    template <class Archive>
    void serialize(Archive& ar)
	{
        ar(name, animationRef, loop);
	}
#pragma endregion
};

struct AnimationStateMachineState
{
    friend class cereal::access;

    std::string name;
    std::string clipName;
    float speed = 1.0f;
    std::string behaviourScriptName;
    std::string behaviourFieldsJson;

    bool overrideLoop = false;
    bool loop = true;

#pragma region Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
		ar(name, clipName, speed, behaviourScriptName, behaviourFieldsJson, overrideLoop, loop);
	}
    #pragma endregion
};

struct AnimationStateMachineTransition
{
    friend class cereal::access;

    std::string sourceStateName;
    std::string targetStateName;
    std::string triggerName;
    float blendTimeSeconds = 0.0f;

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(sourceStateName, targetStateName, triggerName, blendTimeSeconds);
	}
#pragma endregion
};

class AnimationStateMachineAsset : public Asset
{
public:

    friend class ImporterAnimationStateMachine;
    friend class ImporterGltf;

    AnimationStateMachineAsset() = default;
    explicit AnimationStateMachineAsset(UID id): Asset(id, AssetType::ANIMATION_STATE_MACHINE)
    {
    }

    const std::string& getName() const { return m_name; }
    const std::string& getDefaultStateName() const { return m_defaultStateName; }

    const std::vector<AnimationStateMachineClip>& getClips() const { return m_clips; }
    const std::vector<AnimationStateMachineState>& getStates() const { return m_states; }
    const std::vector<AnimationStateMachineTransition>& getTransitions() const { return m_transitions; }

    std::string& getNameMutable() { return m_name; }
    std::string& getDefaultStateNameMutable() { return m_defaultStateName; }

    std::vector<AnimationStateMachineClip>& getClipsMutable() { return m_clips; }
    std::vector<AnimationStateMachineState>& getStatesMutable() { return m_states; }
    std::vector<AnimationStateMachineTransition>& getTransitionsMutable() { return m_transitions; }

#pragma region Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<Asset>(this), m_name, m_defaultStateName, m_clips, m_states, m_transitions);
    }
#pragma endregion

private:
    std::string m_name;
    std::string m_defaultStateName;

    std::vector<AnimationStateMachineClip> m_clips;
    std::vector<AnimationStateMachineState> m_states;
    std::vector<AnimationStateMachineTransition> m_transitions;
};

CEREAL_REGISTER_TYPE(AnimationStateMachineAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, AnimationStateMachineAsset)