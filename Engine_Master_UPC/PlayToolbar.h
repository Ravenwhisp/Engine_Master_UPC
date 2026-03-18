#pragma once

class ModuleEditor;

class PlayToolbar
{
public:
	PlayToolbar();
	~PlayToolbar();

	void DrawCentered(float menuWidth);

private:
	ModuleEditor* m_moduleEditor;

	void ManagePositionButton(int selectedIndex);
	void CreateButton(int selectedIndex, const char* text, int index);

	float m_buttonWidth = 100.0f;
	float m_buttonHeight = 28.0f;
};
