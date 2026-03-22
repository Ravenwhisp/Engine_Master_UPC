#include "Globals.h"
#include "Animation.h"

#include "AnimationAsset.h"

Animation::Animation(const UID uid, const AnimationAsset& asset) : ICacheable(uid)
{
	m_channels = asset.getChannels();
	m_duration = asset.getDuration();
}

Animation::~Animation() = default;