#pragma once
#include "EditorWindow.h"
#include "AssetReference.h"
#include "VideoAsset.h"

class ModuleVideo;

class WindowVideoDebug : public EditorWindow
{
private:
	ModuleVideo* m_moduleVideo = nullptr;
	VideoRef m_videoAsset;

public:
	WindowVideoDebug();

	const char* getWindowName() const override
	{
		return "Video Debug";
	}

	void drawInternal() override;
};