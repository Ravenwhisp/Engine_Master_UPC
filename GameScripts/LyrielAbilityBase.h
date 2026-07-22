#pragma once

#include "AbilityBase.h"

class LyrielCharacter;

class LyrielAbilityBase : public AbilityBase
{
public:
    explicit LyrielAbilityBase(GameObject* owner);

    void Start() override;

protected:
    Transform* findArrowSpawnTransform() const;
    void faceDirection(const Vector3& direction);

protected:
    LyrielCharacter* m_lyrielCharacter = nullptr;
};