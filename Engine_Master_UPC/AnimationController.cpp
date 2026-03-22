#include "Globals.h"
#include "AnimationController.h"

#include "Animation.h"

Transform* AnimationController::getTransform(const std::string& channelName, float& pos, Quaternion& rot) const
{
	auto it = m_currentAnimation->getChannels().find(channelName);
	if (it == m_currentAnimation->getChannels().end())
	{
		DEBUG_ERROR("Channel %s not found in animation", channelName.c_str());
		return nullptr;
	}
	Channel& channel = it->second;

	
}