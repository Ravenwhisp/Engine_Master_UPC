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

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(time, value);
	}
#pragma endregion
};

struct AnimKeyQuat
{
    float time = 0.0f;
    Quaternion value = Quaternion::Identity;

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(time, value);
	}
#pragma endregion
};
struct AnimChannel
{
    std::vector<AnimKeyVec3> posKeys;
    std::vector<AnimKeyQuat> rotKeys;
    std::vector<AnimKeyVec3> scaleKeys;

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(posKeys, rotKeys, scaleKeys);
	}
#pragma endregion
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

#pragma region Serialization
    template <class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Asset>(this), m_name, m_durationSeconds, m_channels);
	}
#pragma endregion
};

CEREAL_REGISTER_TYPE(AnimationAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, AnimationAsset)