#include "Globals.h"
#include "TimeModule.h"

bool TimeModule::init()
{
	m_realTimer.start();
	m_gameTimer.start();
	m_lastRealMs = m_realTimer.read();
	return true;
}

void TimeModule::update()
{
	uint64_t nowRealMs = m_realTimer.read();
	uint64_t deltaMs = nowRealMs - m_lastRealMs;

	m_lastRealDelta = deltaMs * 0.001f; // Convert ms to seconds
	m_lastRealMs = nowRealMs;

	m_frames++;
}

void TimeModule::waitForNextFrame()
{
	if (m_targetFps <= 0) return;

	float targetFrameTime = 1000.0f / static_cast<float>(m_targetFps);
	uint64_t frameStartTime = m_realTimer.read();

	while (true)
	{
		uint64_t now = m_realTimer.read();
		float elapsed = static_cast<float>(now - frameStartTime);
		if (elapsed >= targetFrameTime) break;
	}
}
