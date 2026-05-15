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
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;

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

};

