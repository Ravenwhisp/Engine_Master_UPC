#pragma once
#include "EditorWindow.h"
#include "cvector.h"


class PerformanceWindow: public EditorWindow
{
public:
	PerformanceWindow();
	const char* GetWindowName() const override { return "Performance"; }
	void Update() override;
	void Render() override;

private:
	cvector<float> fps_log{ 100 };
	cvector<float> ms_log{ 100 };
};

