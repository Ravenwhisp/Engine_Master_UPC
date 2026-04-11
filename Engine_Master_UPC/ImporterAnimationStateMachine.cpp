#include "Globals.h"
#include "ImporterAnimationStateMachine.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace
{
    static uint64_t SerializedStringSize(const std::string& s)
    {
        return sizeof(uint32_t) + static_cast<uint64_t>(s.size());
    }
}

uint64_t ImporterAnimationStateMachine::saveTyped(const AnimationStateMachineAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += sizeof(uint32_t); // version
    size += SerializedStringSize(source->m_uid);
    size += SerializedStringSize(source->m_name);
    size += SerializedStringSize(source->m_defaultStateName);

    size += sizeof(uint32_t); // clip count
    for (const AnimationStateMachineClip& clip : source->m_clips)
    {
        size += SerializedStringSize(clip.name);
        size += SerializedStringSize(clip.animationUID);
        size += sizeof(uint8_t);
    }

    size += sizeof(uint32_t); // state count
    for (const AnimationStateMachineState& state : source->m_states)
    {
        size += SerializedStringSize(state.name);
        size += SerializedStringSize(state.clipName);
        size += sizeof(float);
        size += SerializedStringSize(state.behaviourScriptName);
        size += sizeof(uint8_t); // overrideLoop
        size += sizeof(uint8_t); // loop
    }

    size += sizeof(uint32_t); // transition count
    for (const AnimationStateMachineTransition& transition : source->m_transitions)
    {
        size += SerializedStringSize(transition.sourceStateName);
        size += SerializedStringSize(transition.targetStateName);
        size += SerializedStringSize(transition.triggerName);
        size += sizeof(float);
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    const uint32_t version = 2;
    writer.u32(version);

    writer.string(source->m_uid);
    writer.string(source->m_name);
    writer.string(source->m_defaultStateName);

    writer.u32(static_cast<uint32_t>(source->m_clips.size()));
    for (const AnimationStateMachineClip& clip : source->m_clips)
    {
        writer.string(clip.name);
        writer.string(clip.animationUID);
        writer.u8(clip.loop ? 1u : 0u);
    }

    writer.u32(static_cast<uint32_t>(source->m_states.size()));
    for (const AnimationStateMachineState& state : source->m_states)
    {
        writer.string(state.name);
        writer.string(state.clipName);
        writer.bytes(&state.speed, sizeof(float));
        writer.string(state.behaviourScriptName);
        writer.u8(state.overrideLoop ? 1u : 0u);
        writer.u8(state.loop ? 1u : 0u);
    }

    writer.u32(static_cast<uint32_t>(source->m_transitions.size()));
    for (const AnimationStateMachineTransition& transition : source->m_transitions)
    {
        writer.string(transition.sourceStateName);
        writer.string(transition.targetStateName);
        writer.string(transition.triggerName);
        writer.bytes(&transition.blendTimeSeconds, sizeof(float));
    }

    *outBuffer = buffer;
    return size;
}

void ImporterAnimationStateMachine::loadTyped(const uint8_t* buffer, AnimationStateMachineAsset* dst)
{
    BinaryReader reader(buffer);

    const uint32_t version = reader.u32();
    (void)version;

    dst->m_uid = reader.string();
    dst->m_name = reader.string();
    dst->m_defaultStateName = reader.string();

    const uint32_t clipCount = reader.u32();
    dst->m_clips.clear();
    dst->m_clips.resize(clipCount);

    for (uint32_t i = 0; i < clipCount; ++i)
    {
        AnimationStateMachineClip& clip = dst->m_clips[i];
        clip.name = reader.string();
        clip.animationUID = reader.string();
        clip.loop = reader.u8() != 0;
    }

    const uint32_t stateCount = reader.u32();
    dst->m_states.clear();
    dst->m_states.resize(stateCount);

    for (uint32_t i = 0; i < stateCount; ++i)
    {
        AnimationStateMachineState& state = dst->m_states[i];
        state.name = reader.string();
        state.clipName = reader.string();
        reader.bytes(&state.speed, sizeof(float));

        if (version >= 2)
        {
            state.behaviourScriptName = reader.string();
            state.overrideLoop = reader.u8() != 0;
            state.loop = reader.u8() != 0;
        }
        else
        {
            state.behaviourScriptName.clear();
            state.overrideLoop = false;
            state.loop = true;
        }
    }

    const uint32_t transitionCount = reader.u32();
    dst->m_transitions.clear();
    dst->m_transitions.resize(transitionCount);

    for (uint32_t i = 0; i < transitionCount; ++i)
    {
        AnimationStateMachineTransition& transition = dst->m_transitions[i];
        transition.sourceStateName = reader.string();
        transition.targetStateName = reader.string();
        transition.triggerName = reader.string();
        reader.bytes(&transition.blendTimeSeconds, sizeof(float));
    }
}