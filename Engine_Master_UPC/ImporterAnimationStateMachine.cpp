#include "Globals.h"
#include "ImporterAnimationStateMachine.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

#include <rapidjson/document.h>
#include "rapidjson/filereadstream.h"
#include <cstdio>

namespace
{
    static uint64_t SerializedStringSize(const std::string& s)
    {
        return sizeof(uint32_t) + static_cast<uint64_t>(s.size());
    }
}

bool ImporterAnimationStateMachine::importNative(const std::filesystem::path& path, AnimationStateMachineAsset* dst)
{
    if (!dst)
    {
        return false;
    }

    FILE* fp = std::fopen(path.string().c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[ImporterAnimationStateMachine] Could not open '%s'.", path.string().c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);
    std::fclose(fp);

    if (doc.HasParseError() || !doc.IsObject())
    {
        DEBUG_ERROR("[ImporterAnimationStateMachine] JSON parse error in '%s'.", path.string().c_str());
        return false;
    }

    dst->m_name.clear();
    dst->m_defaultStateName.clear();
    dst->m_clips.clear();
    dst->m_states.clear();
    dst->m_transitions.clear();

    if (doc.HasMember("name") && doc["name"].IsString())
    {
        dst->m_name = doc["name"].GetString();
    }

    if (doc.HasMember("defaultState") && doc["defaultState"].IsString())
    {
        dst->m_defaultStateName = doc["defaultState"].GetString();
    }

    if (doc.HasMember("clips") && doc["clips"].IsArray())
    {
        const auto& clips = doc["clips"];
        dst->m_clips.reserve(clips.Size());

        for (rapidjson::SizeType i = 0; i < clips.Size(); ++i)
        {
            const auto& clipJson = clips[i];
            if (!clipJson.IsObject())
                continue;

            AnimationStateMachineClip clip;

            if (clipJson.HasMember("name") && clipJson["name"].IsString())
                clip.name = clipJson["name"].GetString();

            if (clipJson.HasMember("animationUID") && clipJson["animationUID"].IsString())
                clip.animationUID = clipJson["animationUID"].GetString();
            else
                clip.animationUID = INVALID_ASSET_ID;

            if (clipJson.HasMember("loop") && clipJson["loop"].IsBool())
                clip.loop = clipJson["loop"].GetBool();
            else
                clip.loop = true;

            dst->m_clips.push_back(std::move(clip));
        }
    }

    if (doc.HasMember("states") && doc["states"].IsArray())
    {
        const auto& states = doc["states"];
        dst->m_states.reserve(states.Size());

        for (rapidjson::SizeType i = 0; i < states.Size(); ++i)
        {
            const auto& stateJson = states[i];
            if (!stateJson.IsObject())
                continue;

            AnimationStateMachineState state;

            if (stateJson.HasMember("name") && stateJson["name"].IsString())
                state.name = stateJson["name"].GetString();

            if (stateJson.HasMember("clipName") && stateJson["clipName"].IsString())
                state.clipName = stateJson["clipName"].GetString();

            if (stateJson.HasMember("speed") && stateJson["speed"].IsNumber())
                state.speed = stateJson["speed"].GetFloat();
            else
                state.speed = 1.0f;

            if (stateJson.HasMember("behaviourScriptName") && stateJson["behaviourScriptName"].IsString())
                state.behaviourScriptName = stateJson["behaviourScriptName"].GetString();
            else
                state.behaviourScriptName.clear();

            if (stateJson.HasMember("behaviourFieldsJson") && stateJson["behaviourFieldsJson"].IsString())
                state.behaviourFieldsJson = stateJson["behaviourFieldsJson"].GetString();
            else
                state.behaviourFieldsJson.clear();

            if (stateJson.HasMember("overrideLoop") && stateJson["overrideLoop"].IsBool())
                state.overrideLoop = stateJson["overrideLoop"].GetBool();
            else
                state.overrideLoop = false;

            if (stateJson.HasMember("loop") && stateJson["loop"].IsBool())
                state.loop = stateJson["loop"].GetBool();
            else
                state.loop = true;

            dst->m_states.push_back(std::move(state));
        }
    }

    if (doc.HasMember("transitions") && doc["transitions"].IsArray())
    {
        const auto& transitions = doc["transitions"];
        dst->m_transitions.reserve(transitions.Size());

        for (rapidjson::SizeType i = 0; i < transitions.Size(); ++i)
        {
            const auto& transitionJson = transitions[i];
            if (!transitionJson.IsObject())
                continue;

            AnimationStateMachineTransition transition;

            if (transitionJson.HasMember("sourceStateName") && transitionJson["sourceStateName"].IsString())
                transition.sourceStateName = transitionJson["sourceStateName"].GetString();

            if (transitionJson.HasMember("targetStateName") && transitionJson["targetStateName"].IsString())
                transition.targetStateName = transitionJson["targetStateName"].GetString();

            if (transitionJson.HasMember("triggerName") && transitionJson["triggerName"].IsString())
                transition.triggerName = transitionJson["triggerName"].GetString();

            if (transitionJson.HasMember("blendTimeSeconds") && transitionJson["blendTimeSeconds"].IsNumber())
                transition.blendTimeSeconds = transitionJson["blendTimeSeconds"].GetFloat();
            else
                transition.blendTimeSeconds = 0.0f;

            dst->m_transitions.push_back(std::move(transition));
        }
    }

    return true;
}

bool ImporterAnimationStateMachine::saveNative(const std::filesystem::path& path, const AnimationStateMachineAsset* src)
{
    /*auto& alloc = doc.GetAllocator();

    doc.AddMember("name", rapidjson::Value(m_name.c_str(), alloc), alloc);
    doc.AddMember("defaultState", rapidjson::Value(m_defaultStateName.c_str(), alloc), alloc);

    {
        rapidjson::Value clips(rapidjson::kArrayType);
        for (const AnimationStateMachineClip& clip : m_clips)
        {
            rapidjson::Value clipJson(rapidjson::kObjectType);
            clipJson.AddMember("name", rapidjson::Value(clip.name.c_str(), alloc), alloc);
            clipJson.AddMember("animationUID", rapidjson::Value(clip.animationUID.c_str(), alloc), alloc);
            clipJson.AddMember("loop", clip.loop, alloc);
            clips.PushBack(clipJson, alloc);
        }
        doc.AddMember("clips", clips, alloc);
    }

    {
        rapidjson::Value states(rapidjson::kArrayType);
        for (const AnimationStateMachineState& state : m_states)
        {
            rapidjson::Value stateJson(rapidjson::kObjectType);
            stateJson.AddMember("name", rapidjson::Value(state.name.c_str(), alloc), alloc);
            stateJson.AddMember("clipName", rapidjson::Value(state.clipName.c_str(), alloc), alloc);
            stateJson.AddMember("speed", state.speed, alloc);
            stateJson.AddMember("behaviourScriptName", rapidjson::Value(state.behaviourScriptName.c_str(), alloc), alloc);
            stateJson.AddMember("behaviourFieldsJson", rapidjson::Value(state.behaviourFieldsJson.c_str(), alloc), alloc);
            stateJson.AddMember("overrideLoop", state.overrideLoop, alloc);
            stateJson.AddMember("loop", state.loop, alloc);
            states.PushBack(stateJson, alloc);
        }
        doc.AddMember("states", states, alloc);
    }

    {
        rapidjson::Value transitions(rapidjson::kArrayType);
        for (const AnimationStateMachineTransition& transition : m_transitions)
        {
            rapidjson::Value transitionJson(rapidjson::kObjectType);
            transitionJson.AddMember("sourceStateName", rapidjson::Value(transition.sourceStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("targetStateName", rapidjson::Value(transition.targetStateName.c_str(), alloc), alloc);
            transitionJson.AddMember("triggerName", rapidjson::Value(transition.triggerName.c_str(), alloc), alloc);
            transitionJson.AddMember("blendTimeSeconds", transition.blendTimeSeconds, alloc);
            transitions.PushBack(transitionJson, alloc);
        }
        doc.AddMember("transitions", transitions, alloc);
    }*/
    return false;
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
        size += SerializedStringSize(state.behaviourFieldsJson);
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

    const uint32_t version = 3;
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
        writer.string(state.behaviourFieldsJson);
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

        if (version >= 3)
        {
            state.behaviourScriptName = reader.string();
            state.behaviourFieldsJson = reader.string();
            state.overrideLoop = reader.u8() != 0;
            state.loop = reader.u8() != 0;
        }
        else if (version >= 2)
        {
            state.behaviourScriptName = reader.string();
            state.behaviourFieldsJson.clear();
            state.overrideLoop = reader.u8() != 0;
            state.loop = reader.u8() != 0;
        }
        else
        {
            state.behaviourScriptName.clear();
            state.behaviourFieldsJson.clear();
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