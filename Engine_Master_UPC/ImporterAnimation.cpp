#include "Globals.h"
#include "ImporterAnimation.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

static uint64_t strSerialSize(const std::string& s)
{
    return sizeof(uint32_t) + s.size();
}

uint64_t ImporterAnimation::saveTyped(const AnimationAsset* a, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += strSerialSize(a->m_uid);
    size += strSerialSize(a->m_name);
    size += sizeof(float);
    size += sizeof(uint32_t);

    for (const auto& [nodeName, ch] : a->m_channels)
    {
        size += strSerialSize(nodeName);

        size += sizeof(uint32_t);
        size += ch.posKeys.size() * (sizeof(float) + sizeof(Vector3));

        size += sizeof(uint32_t);
        size += ch.rotKeys.size() * (sizeof(float) + sizeof(Quaternion));

        size += sizeof(uint32_t);
        size += ch.scaleKeys.size() * (sizeof(float) + sizeof(Vector3));
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter w(buffer);

    w.string(a->m_uid);
    w.string(a->m_name);
    w.bytes(&a->m_durationSeconds, sizeof(float));

    w.u32(static_cast<uint32_t>(a->m_channels.size()));
    for (const auto& [nodeName, ch] : a->m_channels)
    {
        w.string(nodeName);

        w.u32(static_cast<uint32_t>(ch.posKeys.size()));
        for (const auto& k : ch.posKeys)
        {
            w.bytes(&k.time, sizeof(float));
            w.bytes(&k.value, sizeof(Vector3));
        }

        w.u32(static_cast<uint32_t>(ch.rotKeys.size()));
        for (const auto& k : ch.rotKeys)
        {
            w.bytes(&k.time, sizeof(float));
            w.bytes(&k.value, sizeof(Quaternion));
        }

        w.u32(static_cast<uint32_t>(ch.scaleKeys.size()));
        for (const auto& k : ch.scaleKeys)
        {
            w.bytes(&k.time, sizeof(float));
            w.bytes(&k.value, sizeof(Vector3));
        }
    }

    *outBuffer = buffer;
    return size;
}

void ImporterAnimation::loadTyped(const uint8_t* buffer, AnimationAsset* a)
{
    BinaryReader r(buffer);

    a->m_uid = r.string();
    a->m_name = r.string();
    r.bytes(&a->m_durationSeconds, sizeof(float));

    const uint32_t channelCount = r.u32();
    a->m_channels.clear();
    a->m_channels.reserve(channelCount);

    for (uint32_t i = 0; i < channelCount; ++i)
    {
        const std::string nodeName = r.string();
        AnimChannel ch;

        const uint32_t posCount = r.u32();
        ch.posKeys.resize(posCount);
        for (uint32_t k = 0; k < posCount; ++k)
        {
            r.bytes(&ch.posKeys[k].time, sizeof(float));
            r.bytes(&ch.posKeys[k].value, sizeof(Vector3));
        }

        const uint32_t rotCount = r.u32();
        ch.rotKeys.resize(rotCount);
        for (uint32_t k = 0; k < rotCount; ++k)
        {
            r.bytes(&ch.rotKeys[k].time, sizeof(float));
            r.bytes(&ch.rotKeys[k].value, sizeof(Quaternion));
        }

        const uint32_t scaleCount = r.u32();
        ch.scaleKeys.resize(scaleCount);
        for (uint32_t k = 0; k < scaleCount; ++k)
        {
            r.bytes(&ch.scaleKeys[k].time, sizeof(float));
            r.bytes(&ch.scaleKeys[k].value, sizeof(Vector3));
        }

        a->m_channels.emplace(nodeName, std::move(ch));
    }
}