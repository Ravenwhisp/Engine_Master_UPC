#pragma once

#include "HapticEffectDefinition.h"

#include <string>
#include <unordered_map>

class HapticEffectLibrary
{
public:
    static HapticEffectLibrary& get();

    void registerEffect(const HapticEffectDefinition& def);

    void registerBuiltins();

    bool loadFromJSON(const char* path);

    const HapticEffectDefinition* findEffect(const std::string& id) const;

    const std::unordered_map<std::string, HapticEffectDefinition>& getAll() const;

    void logRegisteredEffects() const;

private:
    HapticEffectLibrary() = default;
    ~HapticEffectLibrary() = default;

    HapticEffectLibrary(const HapticEffectLibrary&) = delete;
    HapticEffectLibrary& operator=(const HapticEffectLibrary&) = delete;

    std::unordered_map<std::string, HapticEffectDefinition> m_effects;
};