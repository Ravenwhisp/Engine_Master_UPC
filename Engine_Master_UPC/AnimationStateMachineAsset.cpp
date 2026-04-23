#include "Globals.h"
#include "AnimationStateMachineAsset.h"

bool AnimationStateMachineAsset::serialize(rapidjson::Document& doc)
{
    auto& alloc = doc.GetAllocator();

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
    }
    
    return true;
}

bool AnimationStateMachineAsset::desearialize(const rapidjson::Value& json)
{
    return false;
}
