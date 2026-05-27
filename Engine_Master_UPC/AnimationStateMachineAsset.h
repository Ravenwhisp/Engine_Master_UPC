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
    friend class ImporterAnimationStateMachine;
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
        uint32_t version = 3;
        archive.serialize(version);

        archive.serialize(m_name);
        archive.serialize(m_defaultStateName);

        uint32_t clipCount = static_cast<uint32_t>(m_clips.size());
        archive.serialize(clipCount);
        if (archive.mode() == ArchiveMode::Input)
            m_clips.resize(clipCount);

        for (auto& clip : m_clips)
        {
            archive.serialize(clip.name);
            archive.serialize(clip.animationUID.m_uid);
            archive.serialize(clip.animationUID.m_libId);
            uint32_t clipType = static_cast<uint32_t>(clip.animationUID.m_type);
            archive.serialize(clipType);
            clip.animationUID.m_type = static_cast<AssetType>(clipType);
            archive.serialize(clip.loop);
        }

        uint32_t stateCount = static_cast<uint32_t>(m_states.size());
        archive.serialize(stateCount);
        if (archive.mode() == ArchiveMode::Input)
            m_states.resize(stateCount);

        for (auto& state : m_states)
        {
            archive.serialize(state.name);
            archive.serialize(state.clipName);
            archive.serialize(state.speed);

            if (version >= 2)
            {
                archive.serialize(state.behaviourScriptName);
                if (version >= 3)
                    archive.serialize(state.behaviourFieldsJson);
                else if (archive.mode() == ArchiveMode::Input)
                    state.behaviourFieldsJson.clear();
                archive.serialize(state.overrideLoop);
                archive.serialize(state.loop);
            }
            else if (archive.mode() == ArchiveMode::Input)
            {
                state.behaviourScriptName.clear();
                state.behaviourFieldsJson.clear();
                state.overrideLoop = false;
                state.loop = true;
            }
        }

        uint32_t transitionCount = static_cast<uint32_t>(m_transitions.size());
        archive.serialize(transitionCount);
        if (archive.mode() == ArchiveMode::Input)
            m_transitions.resize(transitionCount);

        for (auto& transition : m_transitions)
        {
            archive.serialize(transition.sourceStateName);
            archive.serialize(transition.targetStateName);
            archive.serialize(transition.triggerName);
            archive.serialize(transition.blendTimeSeconds);
        }
    }

private:
    std::string m_name;
    std::string m_defaultStateName;

    std::vector<AnimationStateMachineClip> m_clips;
    std::vector<AnimationStateMachineState> m_states;
    std::vector<AnimationStateMachineTransition> m_transitions;
};