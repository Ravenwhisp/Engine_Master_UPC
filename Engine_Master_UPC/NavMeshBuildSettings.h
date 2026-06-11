#pragma once

struct NavMeshBuildSettings
{
    float cellSize = 0.2f;
    float cellHeight = 0.1f;
    int tileSize = 64;

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
};