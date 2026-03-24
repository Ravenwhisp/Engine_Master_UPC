#pragma once
#include "EditorWindow.h"

class WindowHardware: public EditorWindow
{
public:
	const char* getWindowName() const override { return "Hardware Info"; }
	void render() override;

private:
	void cpu();
	void systemVRAM();
	void vram();
	void gpu();
	void gpuFeatures();
};

