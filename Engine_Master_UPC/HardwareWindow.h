#pragma once
#include "EditorWindow.h"
#include <dxgi1_6.h>

class HardwareWindow: public EditorWindow
{
public:
	const char* GetWindowName() const override { return "Hardware Info"; }
	void Render() override;

private:
	void CPU();
	void SystemRAM();
	void VRAM();
	void GPU();
	void GPUCaps();
};

