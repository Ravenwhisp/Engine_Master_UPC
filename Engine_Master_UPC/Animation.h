#pragma once
#include "ICacheable.h"

#include "Channel.h"

class AnimationAsset;

class Animation : public ICacheable
{
public:
	explicit Animation(const UID uid, const AnimationAsset& asset);
	~Animation();
	
	std::unordered_map<std::string, Channel> getChannels() const noexcept { return m_channels; }

private:
	std::unordered_map<std::string, Channel> m_channels;
	float m_duration = 0.0f;
};