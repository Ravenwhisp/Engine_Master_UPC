#pragma once
#include <cstring>

#include "IArchive.h"
#include "JsonArchive.h"

#include <memory>

class EmitterInstance;
class Transform;

// Update when needed
enum class ParticleRenderMode {
    BILLBOARD = 0,
    HORIZONTAL = 1,
    VERTICAL = 2
};

enum class ParticleModuleType {
    BASE,
    AREA,
    SPAWN,
    COLOR,
    LIFETIME,
    VELOCITY,
    SIZE,
    ROTATION,
    ANIMATION,
    RENDER
};

enum class ParameterType {
    CONSTANT,
    RANDOM_BETWEEN_TWO,
    CURVE,
    TOTAL_TYPES
};

inline const char* ParameterTypeToString(uint32_t v)
{
    switch (static_cast<ParameterType>(v))
    {
    case ParameterType::CONSTANT:           return "CONSTANT";
    case ParameterType::RANDOM_BETWEEN_TWO: return "RANDOM_BETWEEN_TWO";
    case ParameterType::CURVE:              return "CURVE";
    default: return "CONSTANT";
    }
}

inline uint32_t StringToParameterType(const char* s)
{
    if (std::strcmp(s, "CONSTANT") == 0)            return 0;
    if (std::strcmp(s, "RANDOM_BETWEEN_TWO") == 0)  return 1;
    if (std::strcmp(s, "CURVE") == 0)               return 2;
    return 0;
}

class ParticleModule
{
public:
    ParticleModule(ParticleModuleType type) : m_moduleType(type) {}
    //virtual ~ParticleModule() = default; 

    virtual std::unique_ptr<ParticleModule> clone() const = 0;

    virtual void spawn(EmitterInstance* particleData) { return; } // Not being used right now...
    virtual void update(EmitterInstance* particleData) { return; }

    ParticleModuleType getType() { return m_moduleType; }

    // Interface and saving/loading functions
    virtual bool drawUi() { return false; }
    virtual void debugDraw(Transform* parent) {}
    virtual void serialize(IArchive& archive);
    virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization
    virtual bool deserializeJSON(const rapidjson::Value& moduleInfo) { return true; }

private:
    const ParticleModuleType m_moduleType;
};