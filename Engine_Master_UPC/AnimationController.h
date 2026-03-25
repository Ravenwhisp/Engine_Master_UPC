#pragma once
#include <memory>
#include <string>
#include "SimpleMath.h"

class AnimationAsset;

struct AnimationSample
{
    Vector3 position = Vector3::Zero;
    Quaternion rotation = Quaternion::Identity;
    Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

    bool hasPosition = false;
    bool hasRotation = false;
    bool hasScale = false;
};

class AnimationController
{
public:
    void SetAnimation(const std::shared_ptr<AnimationAsset>& animation);
    const std::shared_ptr<AnimationAsset>& GetAnimation() const { return m_animation; }

    void Play(bool loop = true);
    void Stop();
    void Pause() { m_playing = false; }

    void SetLoop(bool loop) { m_loop = loop; }
    bool IsLooping() const { return m_loop; }
    bool IsPlaying() const { return m_playing; }

    void SetTime(float seconds);
    float GetTime() const { return m_currentTime; }
    float GetDuration() const;

    void Update(float deltaTimeSeconds);

    bool GetTransform(const std::string& channelName, AnimationSample& outSample) const;

private:
    std::shared_ptr<AnimationAsset> m_animation;
    float m_currentTime = 0.0f;
    bool m_loop = true;
    bool m_playing = false;
};