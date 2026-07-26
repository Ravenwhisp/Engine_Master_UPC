#include "Globals.h"
#include "NavMeshBuildSettings.h"
#include "IArchive.h"

void NavMeshBuildSettings::serialize(IArchive& archive)
{
    archive.serialize(cellSize, "cellSize");
    archive.serialize(cellHeight, "cellHeight");
    archive.serialize(agentHeight, "agentHeight");
    archive.serialize(agentRadius, "agentRadius");
    archive.serialize(agentMaxClimb, "agentMaxClimb");
    archive.serialize(agentMaxSlope, "agentMaxSlope");
    archive.serialize(regionMinSize, "regionMinSize");
    archive.serialize(regionMergeSize, "regionMergeSize");
    archive.serialize(edgeMaxLen, "edgeMaxLen");
    archive.serialize(edgeMaxError, "edgeMaxError");
    {
        uint32_t vpp = static_cast<uint32_t>(vertsPerPoly);
        archive.serialize(vpp, "vertsPerPoly");
        vertsPerPoly = static_cast<int>(vpp);
    }
    archive.serialize(detailSampleDist, "detailSampleDist");
    archive.serialize(detailSampleMaxError, "detailSampleMaxError");
}

std::unique_ptr<ImportSettings> NavMeshBuildSettings::clone() const
{
    auto c = std::make_unique<NavMeshBuildSettings>();
    c->cellSize = cellSize;
    c->cellHeight = cellHeight;
    c->agentHeight = agentHeight;
    c->agentRadius = agentRadius;
    c->agentMaxClimb = agentMaxClimb;
    c->agentMaxSlope = agentMaxSlope;
    c->regionMinSize = regionMinSize;
    c->regionMergeSize = regionMergeSize;
    c->edgeMaxLen = edgeMaxLen;
    c->edgeMaxError = edgeMaxError;
    c->vertsPerPoly = vertsPerPoly;
    c->detailSampleDist = detailSampleDist;
    c->detailSampleMaxError = detailSampleMaxError;
    return c;
}
