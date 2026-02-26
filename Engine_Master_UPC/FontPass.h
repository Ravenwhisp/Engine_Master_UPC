#pragma once
#include "IRenderPass.h"

class FontPass : public IRenderPass {
	void apply(ID3D12GraphicsCommandList4* commandList) override;
};