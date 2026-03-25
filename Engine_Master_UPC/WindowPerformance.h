#pragma once
#include "EditorWindow.h"
#include "cvector.h"

class WindowPerformance: public EditorWindow
{
public:
	WindowPerformance();
	const char* getWindowName() const override { return "Performance"; }
	void		drawInternal() override;

private:
	cvector<float> m_fps{ 100 };
	cvector<float> m_milliseconds{ 100 };
};

