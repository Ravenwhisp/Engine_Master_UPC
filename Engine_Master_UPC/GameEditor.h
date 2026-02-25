#pragma once
#include "Globals.h"

#include "EditorWindow.h"

class EditorToolbar;

class GameEditor : public EditorWindow
{
private:
	EditorToolbar* m_editorToolbar;

public:
	GameEditor();
	~GameEditor();

	const char* getWindowName() const override { return "Game"; }
	void		render() override;

	bool        resize(ImVec2 contentRegion);
};
