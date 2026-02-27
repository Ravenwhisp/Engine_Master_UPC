#pragma once
#include "EditorWindow.h"

class PlayToolbar;

class GameWindow : public EditorWindow
{
private:
	PlayToolbar* m_playToolbar;

public:
	GameWindow();
	~GameWindow();

	const char* getWindowName() const override { return "Game"; }
	void		render() override;

	bool        resize(ImVec2 contentRegion);
};
