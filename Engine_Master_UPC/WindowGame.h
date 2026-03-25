#pragma once
#include "EditorWindow.h"

class PlayToolbar;
class ModuleInput;
class RenderSurface;

class WindowGame : public EditorWindow
{
private:
	std::unique_ptr<RenderSurface> m_surface;

	PlayToolbar* m_playToolbar;

	float m_viewportX = 0.0f;
	float m_viewportY = 0.0f;

public:
	WindowGame();
	~WindowGame();

	const char* getWindowName() const override { return "Game"; }
	void		drawInternal() override;

	bool        resize(ImVec2 contentRegion);

	float		getViewportX()      const { return m_viewportX; }
	float		getViewportY()      const { return m_viewportY; }
};
