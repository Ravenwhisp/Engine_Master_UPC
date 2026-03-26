#pragma once
#include "Asset.h"
#include "Globals.h"

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
    friend class ImporterSkin;
    friend class ImporterGltf;

    SkinAsset() = default;
    explicit SkinAsset(MD5Hash id) : Asset(id, AssetType::SKIN) {}

    const std::string& getName() const { return m_name; }
    const std::vector<SkinJoint>& getJoints() const { return m_joints; }

private:
    std::string m_name;
    std::vector<SkinJoint> m_joints;
};