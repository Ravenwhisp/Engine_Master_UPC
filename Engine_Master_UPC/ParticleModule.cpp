#include "Globals.h"
#include "ParticleModule.h"
#include "JsonArchive.h"

void ParticleModule::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Output)
    {
        uint32_t moduleType = static_cast<uint32_t>(m_moduleType);
        archive.serialize(moduleType, "ModuleType");
    }
}
