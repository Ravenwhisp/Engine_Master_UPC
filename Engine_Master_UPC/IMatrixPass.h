#pragma once
#include "Globals.h"

class IMatrixPass {
	virtual void setWorld(const Matrix& world) = 0;
	virtual void setView(const Matrix& view) = 0;
	virtual void setProjectionconst(Matrix& projection) = 0;
	virtual void setMVP(const Matrix& world, const Matrix& view, const Matrix& projection) = 0;
};