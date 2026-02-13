#pragma once
#include "EditorWindow.h"
#include <dxgi1_6.h>

class HardwareWindow: public EditorWindow
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

