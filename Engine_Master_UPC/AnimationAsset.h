#pragma once
#include "Globals.h"
#include "Asset.h"
#include "IArchive.h"

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
    explicit AnimationAsset(AssetReference& id) : Asset(id, AssetType::ANIMATION) {}

    const std::string& getName() const { return m_name; }
    float getDurationSeconds() const { return m_durationSeconds; }
    const std::unordered_map<std::string, AnimChannel>& getChannels() const { return m_channels; }

    void serialize(IArchive& archive) override
    {
        archive.serialize(m_name);
        archive.serialize(m_durationSeconds);

        auto serializeKeys = [&](auto& keys)
        {
            uint32_t count = static_cast<uint32_t>(keys.size());
            archive.serialize(count);
            if (archive.mode() == ArchiveMode::Input)
                keys.resize(count);
            for (auto& k : keys)
            {
                archive.serialize(k.time);
                archive.serializeRaw(&k.value, sizeof(k.value));
            }
        };

        if (archive.mode() == ArchiveMode::Input)
        {
            uint32_t channelCount = 0;
            archive.serialize(channelCount);
            m_channels.clear();
            for (uint32_t i = 0; i < channelCount; ++i)
            {
                std::string nodeName;
                archive.serialize(nodeName);
                AnimChannel ch;
                serializeKeys(ch.posKeys);
                serializeKeys(ch.rotKeys);
                serializeKeys(ch.scaleKeys);
                m_channels.emplace(nodeName, std::move(ch));
            }
        }
        else
        {
            uint32_t channelCount = static_cast<uint32_t>(m_channels.size());
            archive.serialize(channelCount);
            for (auto& pair : m_channels)
            {
                std::string nodeName = pair.first;
                archive.serialize(nodeName);
                serializeKeys(pair.second.posKeys);
                serializeKeys(pair.second.rotKeys);
                serializeKeys(pair.second.scaleKeys);
            }
        }
    }

private:
    std::string m_name;
    float m_durationSeconds = 0.0f;
    std::unordered_map<std::string, AnimChannel> m_channels;
};