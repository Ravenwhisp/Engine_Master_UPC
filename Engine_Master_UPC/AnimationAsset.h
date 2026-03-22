#pragma once
#include "Globals.h"

#include "Asset.h"
#include "Channel.h"

class AnimationAsset : public Asset
{
public:
	friend class ImporterAnimation;
	friend class ImporterGltf;

	AnimationAsset() {}
	AnimationAsset(MD5Hash id) : Asset(id, AssetType::ANIMATION) {}

	std::unordered_map<std::string, Channel> getChannels() const noexcept { return channels; }
	float getDuration() const noexcept { return duration; }

protected:
	std::unordered_map<std::string, Channel> channels;
	float duration = 0.0f;
};