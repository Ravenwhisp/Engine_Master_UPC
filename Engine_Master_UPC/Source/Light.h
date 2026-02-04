#pragma once
#include "Component.h"


class Light: public Component
{
public:
	enum Mode {
		REALTIME,
		MIXED,
		BAKED
	};

	enum Type {
		DIRECTIONAL,
		POINT,
		SPOT, 
		AREA
	};

	Light(Type type = Type::DIRECTIONAL) : type(type){}
	constexpr Vector3& GetColour() { return colour; }
	constexpr Vector3& GetAmbientColour() { return ambientColour; }

	void SetColour(Vector3& colour) { this->colour = colour; }
	void SetAmbientColour(Vector3& colour) { this->ambientColour = colour; }
private:
	Type type;
	Vector3 ambientColour = Vector3::One * (0.1f);
	Vector3 colour = Vector3(1.0f, 1.0f, 1.0f);
};

