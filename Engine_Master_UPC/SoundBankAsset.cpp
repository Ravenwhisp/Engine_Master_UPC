#include "Globals.h"
#include "SoundBankAsset.h"
#include "IArchive.h"

void SoundBankAsset::serialize(IArchive& archive)
{
    archive.serialize(m_bankName, "bankName");

    uint32_t eventCount = static_cast<uint32_t>(m_events.size());
    archive.beginArray(eventCount, "events");
    if (archive.mode() == ArchiveMode::Input)
        m_events.resize(eventCount);

    for (uint32_t i = 0; i < eventCount; ++i)
    {
        archive.serialize(m_events[i].name, "name");
        archive.serialize(m_events[i].id, "id");
    }

    archive.endArray();
}
