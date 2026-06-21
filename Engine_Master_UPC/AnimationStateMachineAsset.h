#pragma once
#include "Asset.h"
#include "AssetReference.h"
#include "IArchive.h"

#include <string>
#include <vector>

class ImporterAnimationStateMachine;
class ImporterGltf;

struct AnimationStateMachineClip
{
    std::string name;
    AssetReference animationUID;
    bool loop = true;
};

struct AnimationStateMachineState
{
    std::string name;
    std::string clipName;
    float speed = 1.0f;
    std::string behaviourScriptName;
    std::string behaviourFieldsJson;

    bool overrideLoop = false;
    bool loop = true;
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
    friend class ImporterGltf;

    AnimationStateMachineAsset() = default;
    explicit AnimationStateMachineAsset(AssetReference& id)
        : Asset(id, AssetType::ANIMATION_STATE_MACHINE)
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

    void serialize(IArchive& archive) override
    {
        archive.serialize(m_name, "name");
        archive.serialize(m_defaultStateName, "defaultState");

        uint32_t clipCount = static_cast<uint32_t>(m_clips.size());
        archive.beginArray(clipCount, "clips");
        if (archive.mode() == ArchiveMode::Input)
            m_clips.resize(clipCount);

        for (uint32_t i = 0; i < clipCount; ++i)
        {
            archive.beginObject();
            auto& clip = m_clips[i];
            archive.serialize(clip.name, "name");
            archive.serialize(clip.animationUID.m_uid, "uid");
            {
                std::string hash = clip.animationUID.m_libId;
                archive.serialize(hash, "libId");
                if (archive.mode() == ArchiveMode::Input)
                    clip.animationUID.m_libId = hash;
            }
            archive.serializeStringEnum(clip.animationUID.m_type, "type", AssetTypeToString, StringToAssetType);
            archive.serialize(clip.loop, "loop");
            archive.endObject();
        }
        archive.endArray();

        uint32_t stateCount = static_cast<uint32_t>(m_states.size());
        archive.beginArray(stateCount, "states");
        if (archive.mode() == ArchiveMode::Input)
            m_states.resize(stateCount);

        for (uint32_t i = 0; i < stateCount; ++i)
        {
            archive.beginObject();
            auto& state = m_states[i];
            archive.serialize(state.name, "name");
            archive.serialize(state.clipName, "clipName");
            archive.serialize(state.speed, "speed");
            archive.serialize(state.behaviourScriptName, "behaviourScriptName");
            archive.serialize(state.behaviourFieldsJson, "behaviourFieldsJson");
            archive.serialize(state.overrideLoop, "overrideLoop");
            archive.serialize(state.loop, "loop");
            archive.endObject();
        }
        archive.endArray();

        uint32_t transitionCount = static_cast<uint32_t>(m_transitions.size());
        archive.beginArray(transitionCount, "transitions");
        if (archive.mode() == ArchiveMode::Input)
            m_transitions.resize(transitionCount);

        for (uint32_t i = 0; i < transitionCount; ++i)
        {
            archive.beginObject();
            auto& transition = m_transitions[i];
            archive.serialize(transition.sourceStateName, "sourceStateName");
            archive.serialize(transition.targetStateName, "targetStateName");
            archive.serialize(transition.triggerName, "triggerName");
            archive.serialize(transition.blendTimeSeconds, "blendTimeSeconds");
            archive.endObject();
        }
        archive.endArray();
    }

private:
    std::string m_name;
    std::string m_defaultStateName;

    std::vector<AnimationStateMachineClip> m_clips;
    std::vector<AnimationStateMachineState> m_states;
    std::vector<AnimationStateMachineTransition> m_transitions;
};