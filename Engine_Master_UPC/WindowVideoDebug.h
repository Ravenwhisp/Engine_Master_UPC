#pragma once
#include "EditorWindow.h"

class ModuleVideo;

class WindowVideoDebug : public EditorWindow
{
private:
	ModuleVideo* m_moduleVideo = nullptr;

public:
	WindowVideoDebug();

	const char* getWindowName() const override
	{
		return "Video Debug";
	}

	void drawInternal() override;
};