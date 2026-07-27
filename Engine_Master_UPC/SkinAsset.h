#pragma once
#include "Asset.h"
#include "Globals.h"
#include "IArchive.h"

#include <string>
#include <vector>

struct SkinJoint
{
    std::string nodeName;
    Matrix inverseBindMatrix = Matrix::Identity;
};

class SkinAsset : public Asset
{
public:
    friend class ImporterGltf;

    SkinAsset() = default;
    explicit SkinAsset(AssetId& id) : Asset(id, AssetType::SKIN) {}

    const std::string& getName() const { return m_name; }
    const std::vector<SkinJoint>& getJoints() const { return m_joints; }

    void serialize(IArchive& archive) override
    {
        archive.serialize(m_name);

        uint32_t jointCount = static_cast<uint32_t>(m_joints.size());
        archive.serialize(jointCount);
        if (archive.mode() == ArchiveMode::Input)
            m_joints.resize(jointCount);

        for (auto& joint : m_joints)
        {
            archive.serialize(joint.nodeName);
            archive.serializeRaw(&joint.inverseBindMatrix, sizeof(Matrix));
        }
    }

private:
    std::string m_name;
    std::vector<SkinJoint> m_joints;
};