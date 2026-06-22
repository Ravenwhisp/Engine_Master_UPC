#pragma once
#include "EditorWindow.h"

class WindowOutlineSettings : public EditorWindow
{
public:
	WindowOutlineSettings();

	const char* getWindowName() const override { return "Outline Settings"; }
	void drawInternal() override;
};
