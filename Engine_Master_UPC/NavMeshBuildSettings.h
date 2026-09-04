#pragma once
#include "ImportSettings.h"

class NavMeshBuildSettings : public ImportSettings
{
public:
    float cellSize = 0.2f;
    float cellHeight = 0.1f;

    float agentHeight = 1.8f;
    float agentRadius = 0.4f;
    float agentMaxClimb = 0.6f;
    float agentMaxSlope = 45.0f;

    float regionMinSize = 8.0f;
    float regionMergeSize = 20.0f;

    float edgeMaxLen = 12.0f;
    float edgeMaxError = 1.3f;

    int vertsPerPoly = 6;

    float detailSampleDist = 6.0f;
    float detailSampleMaxError = 1.0f;

    void serialize(IArchive& archive) override;
    std::unique_ptr<ImportSettings> clone() const override;
    const char* getTypeName() const override { return "NavMeshBuildSettings"; }
};
