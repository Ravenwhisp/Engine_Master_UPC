#pragma once
#include "Module.h"
#include "Timer.h"


class TimeModule : public Module
{
public:
	TimeModule(int targetFps = 60): _targetFps(targetFps) {}
	~TimeModule() override = default;

	bool init() override
	{
		realTimer.Start();
		gameTimer.Start();
		lastRealMs = realTimer.Read();
		return true;
	}

	void update() override
	{
		uint64_t nowRealMs = realTimer.Read();
		uint64_t deltaMs = nowRealMs - lastRealMs;

		lastRealDelta = deltaMs * 0.001f; // Convert ms to seconds
		lastRealMs = nowRealMs;

		frames++;
	}

	void WaitForNextFrame()
	{
		if (_targetFps <= 0) return;

		float targetFrameTime = 1000.0f / static_cast<float>(_targetFps); // in ms
		uint64_t frameStartTime = realTimer.Read();

		while (true)
		{
			uint64_t now = realTimer.Read();
			float elapsed = static_cast<float>(now - frameStartTime);
			if (elapsed >= targetFrameTime)
				break;
		}
	}
	
	float unscaledDeltaTime() const { return lastRealDelta; }
	float deltaTime() const { return scale * lastRealDelta; }
	float time() { return (realTimer.Read() * 0.001f) * scale; }
	float timeScale() const  { return scale; }
	void setTimeScale(float s) { scale = std::max(0.0f, s); }
	uint32_t frameCount() const { return frames; }
	float realtimeSinceStartup()  { return realTimer.Read() * 0.001f; }

private:
	Timer realTimer;
	Timer gameTimer;

	float scale = 1.0f;
	uint64_t lastRealMs = 0;
	float lastRealDelta = 0.0f;
	uint32_t frames = 0;

	int _targetFps = 60;
};

