#pragma once

#include "ParticleModule.h"

class Transform;

// Update when needed
enum class AreaType {
	CIRCLE,
	CONE,
	SPHERE,
	HEMISPHERE,
	TOTAL_TYPES
};


class EmitterArea : public ParticleModule
{
public:

	EmitterArea() : ParticleModule(ParticleModuleType::AREA) {}
	std::unique_ptr<ParticleModule> clone() const override {
		return std::make_unique<EmitterArea>(*this); // calls copy constructor
	}

	void update(EmitterInstance* particleData) override;

	bool drawUi() override;
	void debugDraw(Transform* parent) override;
	void serialize(IArchive& archive) override;

private:

	AreaType m_shapeType = AreaType::CIRCLE;

	// circle, sphere, hemisphere params.
	float m_radius = 5.f;
	float m_radiusThickness = 1.f; // part of the radius where we spawn particles ([0, 1])

	float m_radiusScale = 3.f; // temporary parameter for cone area

	void setNewParticlesPlacementCircle(EmitterInstance* particleData);
	void setNewParticlesPlacementSphere(EmitterInstance* particleData);
	void setNewParticlesPlacementHemisphere(EmitterInstance* particleData);
	void setNewParticlesPlacementCone(EmitterInstance* particleData);

	Vector3 getCircleDirection(Vector3 center, const Transform& objectTransform);
	Vector3 getSphereDirection(Vector3 center, const Transform& objectTransform);
	Vector3 getHemisphereDirection(Vector3 center, const Transform& objectTransform);

	// DEBUG DRAW //
	const Vector3 m_areaColor = Vector3(0.65f, 0.90f, 0.98f);
	const float m_thicknessAreaColor = 0.65f;
	
	const float m_topConeCircleHeight = 2.5f; // if not enough, number can be increased

	void drawCone(Transform* parent, const Vector3& color, float radius, bool depthEnabled) const;
};

