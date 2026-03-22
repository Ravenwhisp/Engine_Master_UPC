#pragma once
#include "Globals.h"

struct Channel
{
	std::unique_ptr<Vector3[]>		positions;
	std::unique_ptr<float[]>		posTimeStamps;
	std::unique_ptr<Quaternion[]>	rotations;
	std::unique_ptr<float[]>		rotTimeStamps;
	/*
	Scales optional
	std::unique_ptr<Vector3[]> scales;
	std::unique_ptr<float[]> scaleTimeStamps;
	*/
	uint32_t numPositions = 0;
	uint32_t numRotations = 0;
	//uint32_t numScales = 0;
};
