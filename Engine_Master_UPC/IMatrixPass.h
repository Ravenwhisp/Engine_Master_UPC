#pragma once
#include <SimpleMath.h>

class IMatrixPass {
	virtual void setWorld(const DirectX::SimpleMath::Matrix& world) = 0;
	virtual void setView(const DirectX::SimpleMath::Matrix& view) = 0;
	virtual void setProjectionconst(DirectX::SimpleMath::Matrix& projection) = 0;
	virtual void setMVP(const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection) = 0;
};