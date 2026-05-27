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

        uint32_t channelCount = static_cast<uint32_t>(m_channels.size());
        archive.serialize(channelCount);

        if (archive.mode() == ArchiveMode::Input)
        {
            m_channels.clear();
            for (uint32_t i = 0; i < channelCount; ++i)
            {
                std::string nodeName;
                archive.serialize(nodeName);
                AnimChannel ch;

                uint32_t posCount = 0;
                archive.serialize(posCount);
                ch.posKeys.resize(posCount);
                for (auto& k : ch.posKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Vector3));
                }

                uint32_t rotCount = 0;
                archive.serialize(rotCount);
                ch.rotKeys.resize(rotCount);
                for (auto& k : ch.rotKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Quaternion));
                }

                uint32_t scaleCount = 0;
                archive.serialize(scaleCount);
                ch.scaleKeys.resize(scaleCount);
                for (auto& k : ch.scaleKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Vector3));
                }

                m_channels.emplace(nodeName, std::move(ch));
            }
        }
        else
        {
            for (auto& pair : m_channels)
            {
                std::string nodeName = pair.first;
                archive.serialize(nodeName);

                uint32_t posCount = static_cast<uint32_t>(pair.second.posKeys.size());
                archive.serialize(posCount);
                for (auto& k : pair.second.posKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Vector3));
                }

                uint32_t rotCount = static_cast<uint32_t>(pair.second.rotKeys.size());
                archive.serialize(rotCount);
                for (auto& k : pair.second.rotKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Quaternion));
                }

                uint32_t scaleCount = static_cast<uint32_t>(pair.second.scaleKeys.size());
                archive.serialize(scaleCount);
                for (auto& k : pair.second.scaleKeys)
                {
                    archive.serialize(k.time);
                    archive.serializeRaw(&k.value, sizeof(Vector3));
                }
            }
        }
    }

private:
    std::string m_name;
    float m_durationSeconds = 0.0f;
    std::unordered_map<std::string, AnimChannel> m_channels;
};