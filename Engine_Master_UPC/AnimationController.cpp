#include "Globals.h"
#include "AnimationController.h"
#include "AnimationAsset.h"

#include <algorithm>
#include <cmath>

namespace
{
    float NormalizeTime(float time, float duration)
    {
        if (duration <= 0.0f)
            return 0.0f;

        time = std::fmod(time, duration);
        if (time < 0.0f)
            time += duration;

        return time;
    }

    template<typename TKey, typename TValue, typename TLerpFn>
    bool SampleTrack(const std::vector<TKey>& keys, float time, TValue& outValue, TLerpFn lerpFn)
    {
        if (keys.empty())
            return false;

        if (keys.size() == 1)
        {
            outValue = keys[0].value;
            return true;
        }

        auto it = std::upper_bound(
            keys.begin(),
            keys.end(),
            time,
            [](float t, const TKey& key)
            {
                return t < key.time;
            });

        if (it == keys.begin())
        {
            outValue = keys.front().value;
            return true;
        }

        if (it == keys.end())
        {
            outValue = keys.back().value;
            return true;
        }

        const size_t nextIdx = static_cast<size_t>(it - keys.begin());
        const size_t prevIdx = nextIdx - 1;

        const float t0 = keys[prevIdx].time;
        const float t1 = keys[nextIdx].time;

        if (t1 <= t0)
        {
            outValue = keys[prevIdx].value;
            return true;
        }

        const float lambda = (time - t0) / (t1 - t0);
        outValue = lerpFn(keys[prevIdx].value, keys[nextIdx].value, lambda);
        return true;
    }
}

void AnimationController::SetAnimation(const std::shared_ptr<AnimationAsset>& animation)
{
    m_animation = animation;
    m_currentTime = 0.0f;
    m_playing = false;
}

void AnimationController::Play(bool loop)
{
    m_loop = loop;

    if (!m_animation)
    {
        m_playing = false;
        return;
    }

    m_playing = true;
}

void AnimationController::Stop()
{
    m_playing = false;
    m_currentTime = 0.0f;
}

void AnimationController::SetTime(float seconds)
{
    if (!m_animation)
    {
        m_currentTime = 0.0f;
        return;
    }

    const float duration = GetDuration();
    if (duration <= 0.0f)
    {
        m_currentTime = 0.0f;
        return;
    }

    if (m_loop)
    {
        m_currentTime = NormalizeTime(seconds, duration);
    }
    else
    {
        m_currentTime = std::clamp(seconds, 0.0f, duration);
    }
}

float AnimationController::GetDuration() const
{
    return m_animation ? m_animation->getDurationSeconds() : 0.0f;
}

void AnimationController::Update(float deltaTimeSeconds)
{
    if (!m_playing || !m_animation)
        return;

    const float duration = GetDuration();
    if (duration <= 0.0f)
    {
        m_currentTime = 0.0f;
        m_playing = false;
        return;
    }

    m_currentTime += deltaTimeSeconds;

    if (m_loop)
    {
        m_currentTime = NormalizeTime(m_currentTime, duration);
    }
    else
    {
        if (m_currentTime >= duration)
        {
            m_currentTime = duration;
            m_playing = false;
        }
        else if (m_currentTime < 0.0f)
        {
            m_currentTime = 0.0f;
            m_playing = false;
        }
    }
}

bool AnimationController::GetTransform(const std::string& channelName, AnimationSample& outSample) const
{
    outSample = AnimationSample{};

    if (!m_animation)
        return false;

    const auto& channels = m_animation->getChannels();
    const auto itChannel = channels.find(channelName);
    if (itChannel == channels.end())
        return false;

    const AnimChannel& channel = itChannel->second;
    bool foundAny = false;

    if (SampleTrack<AnimKeyVec3, Vector3>(
        channel.posKeys,
        m_currentTime,
        outSample.position,
        [](const Vector3& a, const Vector3& b, float t)
        {
            return Vector3::Lerp(a, b, t);
        }))
    {
        outSample.hasPosition = true;
        foundAny = true;
    }

    if (SampleTrack<AnimKeyQuat, Quaternion>(
        channel.rotKeys,
        m_currentTime,
        outSample.rotation,
        [](const Quaternion& a, const Quaternion& b, float t)
        {
            return Quaternion::Slerp(a, b, t);
        }))
    {
        outSample.hasRotation = true;
        foundAny = true;
    }

    if (SampleTrack<AnimKeyVec3, Vector3>(
        channel.scaleKeys,
        m_currentTime,
        outSample.scale,
        [](const Vector3& a, const Vector3& b, float t)
        {
            return Vector3::Lerp(a, b, t);
        }))
    {
        outSample.hasScale = true;
        foundAny = true;
    }

    return foundAny;
}