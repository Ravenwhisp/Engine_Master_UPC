#include "Globals.h"
#include "HapticEffectLibrary.h"
#include "HapticEffectDefinition.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/filereadstream.h>

#include <filesystem>
#include <fstream>

HapticEffectLibrary& HapticEffectLibrary::get()
{
    static HapticEffectLibrary instance;
    return instance;
}

void HapticEffectLibrary::registerEffect(const HapticEffectDefinition& def)
{
    m_effects[def.id] = def;
}

void HapticEffectLibrary::registerBuiltins()
{
    registerEffect(HapticEffectDefinition::makeImpact());
    registerEffect(HapticEffectDefinition::makeContinuous());
    registerEffect(HapticEffectDefinition::makeExplosion());
    registerEffect(HapticEffectDefinition::makeGunshot());
    registerEffect(HapticEffectDefinition::makeUIClick());
    registerEffect(HapticEffectDefinition::makeEngineLoop());
}

const HapticEffectDefinition* HapticEffectLibrary::findEffect(const std::string& id) const
{
    std::unordered_map<std::string, HapticEffectDefinition>::const_iterator it = m_effects.find(id);
    return (it != m_effects.end()) ? &it->second : nullptr;
}

const std::unordered_map<std::string, HapticEffectDefinition>& HapticEffectLibrary::getAll() const
{
    return m_effects;
}

static HapticCurve parseCurve(const char* str)
{
    if (!str) return HapticCurve::Linear;
    if (strcmp(str, "Exponential") == 0) return HapticCurve::Exponential;
    if (strcmp(str, "Sustain") == 0) return HapticCurve::Sustain;
    if (strcmp(str, "Punch") == 0) return HapticCurve::Punch;
    return HapticCurve::Linear;
}

static HapticPriority parsePriority(const char* str)
{
    if (!str) return HapticPriority::Normal;
    if (strcmp(str, "Low") == 0) return HapticPriority::Low;
    if (strcmp(str, "High") == 0) return HapticPriority::High;
    if (strcmp(str, "Critical") == 0) return HapticPriority::Critical;
    return HapticPriority::Normal;
}

static const char* curveToString(HapticCurve curve)
{
    switch (curve)
    {
    case HapticCurve::Exponential: return "Exponential";
    case HapticCurve::Sustain: return "Sustain";
    case HapticCurve::Punch: return "Punch";
    default: return "Linear";
    }
}

static const char* priorityToString(HapticPriority priority)
{
    switch (priority)
    {
    case HapticPriority::Low: return "Low";
    case HapticPriority::High: return "High";
    case HapticPriority::Critical: return "Critical";
    default: return "Normal";
    }
}

static bool writeDefaultJSON(const char* path)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

    rapidjson::Value effectsArray(rapidjson::kArrayType);

    const std::unordered_map<std::string, HapticEffectDefinition>& all = HapticEffectLibrary::get().getAll();

    for (std::unordered_map<std::string, HapticEffectDefinition>::const_iterator it = all.begin(); it != all.end(); ++it)
    {
        const HapticEffectDefinition& def = it->second;

        rapidjson::Value entry(rapidjson::kObjectType);
        entry.AddMember("id", rapidjson::Value(def.id.c_str(), alloc), alloc);
        entry.AddMember("duration", rapidjson::Value(static_cast<double>(def.durationSeconds)), alloc);
        entry.AddMember("attack", rapidjson::Value(static_cast<double>(def.attackSeconds)), alloc);
        entry.AddMember("release", rapidjson::Value(static_cast<double>(def.releaseSeconds)), alloc);
        entry.AddMember("delay", rapidjson::Value(static_cast<double>(def.delaySeconds)), alloc);
        entry.AddMember("curve", rapidjson::Value(curveToString(def.curve), alloc), alloc);
        entry.AddMember("priority", rapidjson::Value(priorityToString(def.priority), alloc), alloc);

        rapidjson::Value peak(rapidjson::kObjectType);
        peak.AddMember("leftMotor", rapidjson::Value(static_cast<double>(def.peak.leftMotor)), alloc);
        peak.AddMember("rightMotor", rapidjson::Value(static_cast<double>(def.peak.rightMotor)), alloc);
        peak.AddMember("leftTrigger", rapidjson::Value(static_cast<double>(def.peak.leftTrigger)), alloc);
        peak.AddMember("rightTrigger", rapidjson::Value(static_cast<double>(def.peak.rightTrigger)), alloc);
        entry.AddMember("peak", peak, alloc);

        effectsArray.PushBack(entry, alloc);
    }

    doc.AddMember("effects", effectsArray, alloc);

    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(fsPath.parent_path(), ec);
        if (ec)
        {
            DEBUG_ERROR("[HapticEffectLibrary] Failed to create directory '%s': %s", fsPath.parent_path().string().c_str(), ec.message().c_str());
            return false;
        }
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(path);
    if (!file.is_open())
    {
        DEBUG_ERROR("[HapticEffectLibrary] Failed to create default haptics file at '%s'.", path);
        return false;
    }

    file << buffer.GetString();
    if (!file)
    {
        DEBUG_ERROR("[HapticEffectLibrary] Failed to write '%s'.", path);
        return false;
    }

    DEBUG_LOG("[HapticEffectLibrary] Created default haptics file at '%s'.", path);
    return true;
}

bool HapticEffectLibrary::loadFromJSON(const char* path)
{
    if (!path)
    {
        return false;
    }

    if (!std::filesystem::exists(path))
    {
        DEBUG_WARN("[HapticEffectLibrary] '%s' not found — generating default file.", path);
        if (!writeDefaultJSON(path))
        {
            return false;
        }
    }

    const std::string pathStr(path);

    FILE* fp = std::fopen(pathStr.c_str(), "rb");
    if (!fp)
    {
        DEBUG_ERROR("[HapticEffectLibrary] Could not open '%s' for reading.", pathStr.c_str());
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream stream(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(stream);
    std::fclose(fp);

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[HapticEffectLibrary] JSON parse error in '%s'.", pathStr.c_str());
        return false;
    }

    if (!doc.IsObject() || !doc.HasMember("effects") || !doc["effects"].IsArray())
    {
        DEBUG_ERROR("[HapticEffectLibrary] '%s' is missing the 'effects' array.", pathStr.c_str());
        return false;
    }

    const rapidjson::Value& effects = doc["effects"];

    for (rapidjson::SizeType i = 0; i < effects.Size(); ++i)
    {
        const rapidjson::Value& e = effects[i];

        if (!e.IsObject() || !e.HasMember("id") || !e["id"].IsString())
        {
            DEBUG_WARN("[HapticEffectLibrary] Effect at index %u is missing 'id' — skipped.", i);
            continue;
        }

        HapticEffectDefinition def;
        def.id = e["id"].GetString();

        if (e.HasMember("duration") && e["duration"].IsNumber())
            def.durationSeconds = e["duration"].GetFloat();

        if (e.HasMember("attack") && e["attack"].IsNumber())
            def.attackSeconds = e["attack"].GetFloat();

        if (e.HasMember("release") && e["release"].IsNumber())
            def.releaseSeconds = e["release"].GetFloat();

        if (e.HasMember("delay") && e["delay"].IsNumber())
            def.delaySeconds = e["delay"].GetFloat();

        if (e.HasMember("curve") && e["curve"].IsString())
            def.curve = parseCurve(e["curve"].GetString());

        if (e.HasMember("priority") && e["priority"].IsString())
            def.priority = parsePriority(e["priority"].GetString());

        if (e.HasMember("peak") && e["peak"].IsObject())
        {
            const rapidjson::Value& peak = e["peak"];

            if (peak.HasMember("leftMotor") && peak["leftMotor"].IsNumber())
                def.peak.leftMotor = peak["leftMotor"].GetFloat();

            if (peak.HasMember("rightMotor") && peak["rightMotor"].IsNumber())
                def.peak.rightMotor = peak["rightMotor"].GetFloat();

            if (peak.HasMember("leftTrigger") && peak["leftTrigger"].IsNumber())
                def.peak.leftTrigger = peak["leftTrigger"].GetFloat();

            if (peak.HasMember("rightTrigger") && peak["rightTrigger"].IsNumber())
                def.peak.rightTrigger = peak["rightTrigger"].GetFloat();
        }

        registerEffect(def);
    }

    DEBUG_LOG("[HapticEffectLibrary] Loaded %u effect(s) from '%s'.", effects.Size(), pathStr.c_str());
    return true;
}

void HapticEffectLibrary::logRegisteredEffects() const
{
    DEBUG_LOG("[HapticEffectLibrary] %zu effect(s) registered:", m_effects.size());

    for (std::unordered_map<std::string, HapticEffectDefinition>::const_iterator it = m_effects.begin(); it != m_effects.end(); ++it)
    {
        const HapticEffectDefinition& def = it->second;
        const HapticEffectDefinition& def = it->second;
        const HapticEffectDefinition& def = it->second;
    }
}