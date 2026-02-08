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
		AREA,

		COUNT
	};

	Light(int id, GameObject* gameObject, Type type = Type::DIRECTIONAL) : m_type(type), Component(id, ComponentType::LIGHT, gameObject) {}

	constexpr Vector3&	getColour() { return m_colour; }
	constexpr Vector3&	getAmbientColour() { return m_ambientColour; }

	void	setColour(Vector3& colour) { m_colour = colour; }
	void	setAmbientColour(Vector3& colour) { m_ambientColour = colour; }
private:
	Type	m_type;
	Vector3 m_ambientColour = Vector3::One * (0.1f);
	Vector3 m_colour = Vector3(1.0f, 1.0f, 1.0f);
};

