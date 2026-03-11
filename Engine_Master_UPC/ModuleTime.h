#pragma once
#include "Module.h"

class Timer;

class ModuleTime : public Module
{
public:
	ModuleTime(int targetFps = 60);
	~ModuleTime() override;

	bool init() override;
	void update() override;

	void waitForNextFrame();
	
	float unscaledDeltaTime() const { return m_lastRealDelta; }
	float deltaTime() const { return m_scale * m_lastRealDelta; }
	float time();
	float timeScale() const  { return m_scale; }
	void setTimeScale(float scale) { m_scale = std::max(0.0f, scale); }
	uint32_t frameCount() const { return m_frames; }
	float realtimeSinceStartup();

private:
	std::unique_ptr<Timer> m_realTimer;
	std::unique_ptr<Timer> m_gameTimer;

	float m_scale = 1.0f;
	uint64_t m_lastRealMs = 0;
	float m_lastRealDelta = 0.0f;
	uint32_t m_frames = 0;

	int m_targetFps = 60;
};

