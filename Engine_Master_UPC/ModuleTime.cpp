#include "Globals.h"
#include "ModuleTime.h"

#include <thread>

#include "Timer.h"

ModuleTime::ModuleTime(int targetFps) : m_targetFps(targetFps) {}

ModuleTime::~ModuleTime() = default;

bool ModuleTime::init()
{
	m_realTimer = std::make_unique<Timer>();
	m_gameTimer = std::make_unique<Timer>();

	m_realTimer->start();
	m_gameTimer->start();
	m_lastRealMs = m_realTimer->read();
	return true;
}

void ModuleTime::update()
{
	uint64_t nowRealMs = m_realTimer->read();
	uint64_t deltaMs = nowRealMs - m_lastRealMs;

	m_lastRealDelta = deltaMs * 0.001f; // Convert ms to seconds
	m_lastRealMs = nowRealMs;

	m_frames++;
}

void ModuleTime::waitForNextFrame()
{
	if (m_targetFps <= 0) return;

	float targetFrameTime = 1000.0f / static_cast<float>(m_targetFps);
	uint64_t frameStartTime = m_realTimer->read();

	while (true)
	{
		uint64_t now = m_realTimer->read();
		float elapsed = static_cast<float>(now - frameStartTime);

		if (elapsed >= targetFrameTime)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

float ModuleTime::time() { return (m_realTimer->read() * 0.001f) * m_scale; }

float ModuleTime::realtimeSinceStartup() { return m_realTimer->read() * 0.001f; }
