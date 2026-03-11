#pragma once
#include "EditorWindow.h"

class PlayToolbar;
class ModuleInput;

class GameWindow : public EditorWindow
{
private:
	PlayToolbar* m_playToolbar;

	float m_viewportX = 0.0f;
	float m_viewportY = 0.0f;

public:
	GameWindow();
	~GameWindow();

	const char* getWindowName() const override { return "Game"; }
	void		render() override;

	bool        resize(ImVec2 contentRegion);

	float		getViewportX()      const { return m_viewportX; }
	float		getViewportY()      const { return m_viewportY; }
};
