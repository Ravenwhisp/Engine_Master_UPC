#pragma once

#include "ParticleModule.h"

class Transform;

// Update when needed
enum class AreaType {
	CIRCLE,
	CONE,
	SPHERE,
	HEMISPHERE
};


class EmitterArea : public ParticleModule
{
public:

	EmitterArea() : ParticleModule(ParticleModuleType::AREA) {}

	void update(EmitterInstance* particleData) override;

private:

	AreaType m_shapeType = AreaType::CIRCLE;

	// circle, sphere, hemisphere params.
	float m_radius = 5.f;
	float m_radiusThickness = 1.f; // part of the radius where we spawn particles ([0, 1])


	void setNewParticlesPlacementCircle(EmitterInstance* particleData);
	void setNewParticlesPlacementSphere(EmitterInstance* particleData);
	void setNewParticlesPlacementHemisphere(EmitterInstance* particleData);

	Vector3 getCircleDirection(Vector3 center, const Transform& objectTransform);
	Vector3 getSphereDirection(Vector3 center, const Transform& objectTransform);
	Vector3 getHemisphereDirection(Vector3 center, const Transform& objectTransform);

};

