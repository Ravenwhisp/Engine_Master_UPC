#pragma once
#include "Globals.h"
#include "Asset.h"

#include <string>
#include <vector>
#include <unordered_map>

class ImporterGltf;
class ImporterAnimation;

struct AnimKeyVec3
{
    float time = 0.0f;
    Vector3 value = Vector3::Zero;
};

struct AnimKeyQuat
{
    float time = 0.0f;
    Quaternion value = Quaternion::Identity;
};

struct AnimChannel
{
    std::vector<AnimKeyVec3> posKeys;
    std::vector<AnimKeyQuat> rotKeys;
    std::vector<AnimKeyVec3> scaleKeys;
};

class AnimationAsset : public Asset
{
public:
    friend class ImporterAnimation;
    friend class ImporterGltf;

    AnimationAsset() = default;
    explicit AnimationAsset(MD5Hash id) : Asset(id, AssetType::ANIMATION) {}

    const std::string& getName() const { return m_name; }
    float getDurationSeconds() const { return m_durationSeconds; }
    const std::unordered_map<std::string, AnimChannel>& getChannels() const { return m_channels; }

private:
    std::string m_name;
    float m_durationSeconds = 0.0f;
    std::unordered_map<std::string, AnimChannel> m_channels;
};