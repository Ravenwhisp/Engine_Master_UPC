#pragma once

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
    virtual rapidjson::Value getJSON(rapidjson::Document& domTree) { return rapidjson::Value(); }; // for serialization
    virtual bool deserializeJSON(const rapidjson::Value& moduleInfo) { return true; }

private:
    const ParticleModuleType m_moduleType;
};