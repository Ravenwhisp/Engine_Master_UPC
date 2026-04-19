#pragma once

#include "CharacterBase.h"

class ArrowPool;

class LyrielCharacter : public CharacterBase
{
    DECLARE_SCRIPT(LyrielCharacter)

public:
    explicit LyrielCharacter(GameObject* owner);

    void Start() override;
    ScriptFieldList getExposedFields() const override;

    ArrowPool* getArrowPool() const { return m_arrowPool; }

public:
    std::string m_arrowSpawnChildName = "ArrowSpawn";

private:
    ArrowPool* m_arrowPool = nullptr;
};