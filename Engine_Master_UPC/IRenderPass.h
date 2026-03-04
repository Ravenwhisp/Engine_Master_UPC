#pragma once
#include "Globals.h"

class IRenderPass 
{
public:
    virtual void apply(ID3D12GraphicsCommandList4* commandList) = 0;
};