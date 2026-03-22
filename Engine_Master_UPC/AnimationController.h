#pragma once

#include "UID.h"
#include <string>

class Transform;
class Animation;

class AnimationController
{
public:
	AnimationController();
	
	void play(UID animation, bool loop);
	void stop();
	void update();

	Transform* getTransform(const std::string& channelName, float& pos, Quaternion& rot) const;

private:
	float currentTime = 0.0f;
	bool loop = false;
	Animation* m_currentAnimation = nullptr;

};