#include "Globals.h"
#include "ImporterSkin.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"

static uint64_t serializedStringSize(const std::string& s)
{
    return sizeof(uint32_t) + static_cast<uint64_t>(s.size());
}

uint64_t ImporterSkin::saveTyped(const SkinAsset* source, uint8_t** outBuffer)
{
    uint64_t size = 0;

    size += serializedStringSize(source->m_uid);
    size += serializedStringSize(source->m_name);

    size += sizeof(uint32_t); // joint count

    for (const SkinJoint& joint : source->m_joints)
    {
        size += serializedStringSize(joint.nodeName);
        size += sizeof(Matrix);
    }

    uint8_t* buffer = new uint8_t[size];
    BinaryWriter writer(buffer);

    writer.string(source->m_uid);
    writer.string(source->m_name);

    writer.u32(static_cast<uint32_t>(source->m_joints.size()));
    for (const SkinJoint& joint : source->m_joints)
    {
        writer.string(joint.nodeName);
        writer.bytes(&joint.inverseBindMatrix, sizeof(Matrix));
    }

    *outBuffer = buffer;
    return size;
}

void ImporterSkin::loadTyped(const uint8_t* buffer, SkinAsset* dst)
{
    BinaryReader reader(buffer);

    dst->m_uid = reader.string();
    dst->m_name = reader.string();

    const uint32_t jointCount = reader.u32();
    dst->m_joints.clear();
    dst->m_joints.resize(jointCount);

    for (uint32_t i = 0; i < jointCount; ++i)
    {
        SkinJoint& joint = dst->m_joints[i];
        joint.nodeName = reader.string();
        reader.bytes(&joint.inverseBindMatrix, sizeof(Matrix));
    }
}