#pragma once

#include "ScriptAPI.h"
#include <vector>

class LyrielArrowProjectile;

class ArrowPool : public Script
{
    DECLARE_SCRIPT(ArrowPool)

public:
    explicit ArrowPool(GameObject* owner);

    void Start() override;

    LyrielArrowProjectile* acquireArrow();
    void releaseArrow(LyrielArrowProjectile* arrow);

    ScriptFieldList getExposedFields() const override;

private:
    bool createArrow();

public:
    int m_maxArrows = 5;

    std::string m_arrowPrefabPath = "";

private:
    std::vector<LyrielArrowProjectile*> m_arrows;

};