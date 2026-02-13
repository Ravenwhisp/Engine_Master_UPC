#pragma once
#include "EditorWindow.h"
#include "cvector.h"


class PerformanceWindow: public EditorWindow
{
public:
	PerformanceWindow();
	const char* getWindowName() const override { return "Performance"; }
	void		update() override;
	void		render() override;

private:
	cvector<float> m_fps{ 100 };
	cvector<float> m_milliseconds{ 100 };
};

