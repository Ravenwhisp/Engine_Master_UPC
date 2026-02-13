#pragma once
#include "Module.h"
#include "Timer.h"


class TimeModule : public Module
{
public:
	TimeModule(int targetFps = 60): m_targetFps(targetFps) {}
	~TimeModule() override = default;

	bool init() override;
	void update() override;

	void waitForNextFrame();
	
	float unscaledDeltaTime() const { return m_lastRealDelta; }
	float deltaTime() const { return m_scale * m_lastRealDelta; }
	float time() { return (m_realTimer.read() * 0.001f) * m_scale; }
	float timeScale() const  { return m_scale; }
	void setTimeScale(float scale) { m_scale = std::max(0.0f, scale); }
	uint32_t frameCount() const { return m_frames; }
	float realtimeSinceStartup()  { return m_realTimer.read() * 0.001f; }

private:
	Timer m_realTimer;
	Timer m_gameTimer;

	float m_scale = 1.0f;
	uint64_t m_lastRealMs = 0;
	float m_lastRealDelta = 0.0f;
	uint32_t m_frames = 0;

	int m_targetFps = 60;
};

