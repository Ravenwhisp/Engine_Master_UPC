#include "Globals.h"
#include "EmitterArea.h"
#include "JsonArchive.h"

#include "Application.h"
#include "Transform.h"
#include "EmitterInstance.h"
#include "ModuleParticleSystem.h"
#include "ParticleSystemComponent.h"
#include "GameObject.h"

#include <cmath>

static const float PI = 3.1415926535897931f; // This is redundant; we should have it centralised somewhere

void EmitterArea::update(EmitterInstance* particleData)
{

	switch (m_shapeType) 
	{

	case AreaType::CIRCLE:

		setNewParticlesPlacementCircle(particleData);
		break;

	case AreaType::CONE:

		setNewParticlesPlacementCone(particleData);
		break;

	case AreaType::SPHERE:

		setNewParticlesPlacementSphere(particleData);
		break;
	
	case AreaType::HEMISPHERE:

		setNewParticlesPlacementHemisphere(particleData);
	}
}

bool EmitterArea::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Area"))
	{
		{
			int shapeType = static_cast<int>(m_shapeType);
			if (ImGui::Combo("Shape##Area", &shapeType, "Circle\0Cone\0Sphere\0Hemisphere\0", static_cast<int>(AreaType::TOTAL_TYPES)))
			{
				m_shapeType = static_cast<AreaType>(shapeType);
				parameterChanged |= true;
			}
		}

		parameterChanged |= ImGui::DragFloat("Radius##Area", &m_radius, 0.1f, 0.0f);

		parameterChanged |= ImGui::DragFloat("Radius thickness##Area", &m_radiusThickness, 0.1f, 0.0f, 1.0f);


		if (m_shapeType == AreaType::CONE) 
		{
			parameterChanged |= ImGui::DragFloat("Radius scale##Area", &m_radiusScale, 0.1f, 1.0f, 100.f);
		}
	}

	return parameterChanged;
}

void EmitterArea::debugDraw(Transform* parent)
{
	bool depthEnabled = false;
	auto asFloat3 = [](const Vector3& v) { return &v.x; };

	Vector3 position = parent->getGlobalMatrix().Translation();

	switch (m_shapeType)
	{

	case AreaType::CIRCLE:
	
	{
		Vector3 circleNormal = parent->getUp();

		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor*m_thicknessAreaColor), m_radius*(1.f - m_radiusThickness), 20, 0, depthEnabled);  // thickness radius
		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor), m_radius, 20, 0, depthEnabled);
		
	}
		break;

	case AreaType::CONE:

		drawCone(parent, m_areaColor * m_thicknessAreaColor, m_radius*(1.f - m_radiusThickness), depthEnabled); // Thickness area
		drawCone(parent, m_areaColor, m_radius, depthEnabled);
		
		break;

	case AreaType::SPHERE:

		dd::sphere(asFloat3(position), asFloat3(m_areaColor*m_thicknessAreaColor), m_radius*(1.f - m_radiusThickness), 0, depthEnabled); // thickness radius
		dd::sphere(asFloat3(position), asFloat3(m_areaColor), m_radius, 0, depthEnabled);
		
		break;

	case AreaType::HEMISPHERE:

	{
		Vector3 circleNormal = parent->getUp();
		Vector3 thicknessColor = m_areaColor * m_thicknessAreaColor;

		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(thicknessColor), m_radius *(1.f - m_radiusThickness), 20, 0, depthEnabled);  // thickness radius circle
		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor), m_radius, 20, 0, depthEnabled);

		Vector3 hemisphereHeight = circleNormal * m_radius;
		dd::arrow(asFloat3(position), asFloat3(position + hemisphereHeight * (1.f - m_radiusThickness)), asFloat3(thicknessColor), 0.25f, 0, depthEnabled); // thickness hemisphere height
		dd::arrow(asFloat3(position), asFloat3(position + hemisphereHeight), asFloat3(m_areaColor), 0.25f, 0, depthEnabled); // hemisphere height

	}

		break;
	}
}

void EmitterArea::serialize(IArchive& archive)
{
    ParticleModule::serialize(archive);

    archive.serializeStringEnum(m_shapeType, "ShapeType", AreaTypeToString, StringToAreaType);

    archive.serialize(m_radius, "Radius");
    archive.serialize(m_radiusThickness, "RadiusThickness");
    archive.serialize(m_radiusScale, "RadiusScale");
}

void EmitterArea::setNewParticlesPlacementCircle(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();
	Vector3 objectPosition = objectTransform->getGlobalMatrix().Translation();

	if (m_radius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getCircleDirection(objectPosition, *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectPosition;
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		float radiusLimit = (1.f - m_radiusThickness) * m_radius; // (marks the internal area where we will not spawn particles)
		float radiusDifference = m_radius - radiusLimit;

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the circle area (for now we will assume that this position will mark the movement direction as well, from the center)

			Vector3 direction = getPointOnCircleEdge(m_radius, objectTransform->getRight(), objectTransform->getForward());
			Vector3 spawnPosition = objectPosition + direction; // we still need to substract the offset
			direction.Normalize();

			spawnPosition -= direction * uniform_rand() * radiusDifference; // the offset

			particlePool[particleIndex].position = spawnPosition;
			particlePool[particleIndex].movementDirection = direction;		
		}
	}
}

void EmitterArea::setNewParticlesPlacementSphere(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();
	Vector3 objectPosition = objectTransform->getGlobalMatrix().Translation();

	if (m_radius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getSphereDirection(objectPosition, *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectPosition;
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		float radiusLimit = (1.f - m_radiusThickness) * m_radius; // (marks the internal area where we will not spawn particles)
		float radiusDifference = m_radius - radiusLimit;

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the sphere area (for now we will assume that this position will mark the movement direction as well, from the center)

			Vector3 direction = getPointOnSphereEdge(m_radius);
			Vector3 spawnPosition = objectPosition + direction; // we still need to substract the offset
			direction.Normalize();

			spawnPosition -= direction * uniform_rand() * radiusDifference; // the offset

			particlePool[particleIndex].position = spawnPosition;
			particlePool[particleIndex].movementDirection = direction;
		}
	}
}

void EmitterArea::setNewParticlesPlacementHemisphere(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();
	Vector3 objectPosition = objectTransform->getGlobalMatrix().Translation();

	if (m_radius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getHemisphereDirection(objectPosition, *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectPosition;
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		float radiusLimit = (1.f - m_radiusThickness) * m_radius; // (marks the internal area where we will not spawn particles)
		float radiusDifference = m_radius - radiusLimit;

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the hemisphere area (for now we will assume that this position will mark the movement direction as well, from the center)

			Vector3 direction = getPointOnHemisphereEdge(m_radius, objectTransform->getRight(), objectTransform->getUp(), objectTransform->getForward());
			Vector3 spawnPosition = objectPosition + direction; // we still need to substract the offset
			direction.Normalize();

			spawnPosition -= direction * uniform_rand() * radiusDifference; // the offset

			particlePool[particleIndex].position = spawnPosition;
			particlePool[particleIndex].movementDirection = direction;
		}
	}
}

void EmitterArea::setNewParticlesPlacementCone(EmitterInstance* particleData)
{
	auto& particlePool = app->getModuleParticleSystem()->getPool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();
	Vector3 objectPosition = objectTransform->getGlobalMatrix().Translation();

	float radiusLimit = (1.f - m_radiusThickness) * m_radius; // (marks the internal area where we will not spawn particles)
	float radiusDifference = m_radius - radiusLimit;

	for (auto& particleIndex : newParticles)
	{
		// We need to obtain a position to appear within the base circle area

		Vector3 direction = getPointOnCircleEdge(m_radius, objectTransform->getRight(), objectTransform->getForward());
		Vector3 spawnPosition = objectPosition + direction; // we still need to substract the offset
		direction.Normalize();

		spawnPosition -= direction * uniform_rand() * radiusDifference; // the offset

		// Ok, this is what we do in the other cases; but with the cone, the movement direction (for now) is marked by the radius scale; we will define the "upper circle" of the cone (that is the base circle scaled), and assume it is on an arbitrary height to get the direction
		if (spawnPosition == objectPosition)
		{
			particlePool[particleIndex].movementDirection = objectTransform->getUp();
		}
		else
		{
			direction = (spawnPosition - objectPosition) * m_radiusScale + objectPosition + objectTransform->getUp() - spawnPosition;
			direction.Normalize();

			particlePool[particleIndex].movementDirection = direction;
		}

		particlePool[particleIndex].position = spawnPosition;
	}
}

Vector3 EmitterArea::getCircleDirection(Vector3 center, const Transform& objectTransform)
{
	float rightOffset;
	float forwardOffset;

	do 
	{
		rightOffset = static_cast<float>(((rand() & 511) << 1) - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>(((rand() & 511) << 1) - 511);

	} while (rightOffset == 0.f and forwardOffset == 0.f);

	Vector3 pointToDirection = center + rightOffset * objectTransform.getRight() + forwardOffset * objectTransform.getForward();

	Vector3 direction = (pointToDirection - center);
	direction.Normalize();
	return direction;
}

Vector3 EmitterArea::getSphereDirection(Vector3 center, const Transform& objectTransform)
{
	float rightOffset;
	float forwardOffset;
	float upOffset;

	do
	{
		rightOffset = static_cast<float>(((rand() & 511) << 1) - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>(((rand() & 511) << 1) - 511);
		upOffset = static_cast<float>(((rand() & 511) << 1) - 511);

	} while (rightOffset == 0.f and forwardOffset == 0.f and upOffset == 0.f);

	Vector3 pointToDirection = center + rightOffset * objectTransform.getRight() + forwardOffset * objectTransform.getForward() + upOffset * objectTransform.getUp();

	Vector3 direction = (pointToDirection - center);
	direction.Normalize();
	return direction;
}

Vector3 EmitterArea::getHemisphereDirection(Vector3 center, const Transform& objectTransform)
{
	float rightOffset;
	float forwardOffset;
	float upOffset;

	do
	{
		rightOffset = static_cast<float>(((rand() & 511) << 1) - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>(((rand() & 511) << 1) - 511);
		upOffset = static_cast<float>(rand() & (512 - 1)); // This HAS to be different, because up will always need to be positive or 0

	} while (rightOffset == 0.f and forwardOffset == 0.f and upOffset == 0.f);

	Vector3 pointToDirection = center + rightOffset * objectTransform.getRight() + forwardOffset * objectTransform.getForward() + upOffset * objectTransform.getUp();

	Vector3 direction = (pointToDirection - center);
	direction.Normalize();
	return direction;
}

Vector3 EmitterArea::getPointOnCircleEdge(float radius, const Vector3& rightAxis, const Vector3& upAxis)
{
	// Random angle in radians
	float angle = uniform_rand() * PI * 2.f;

	// Calculate coordinates
	return radius * (std::cos(angle) * rightAxis + std::sin(angle) * upAxis);
}

Vector3 EmitterArea::getPointOnSphereEdge(float radius)
{
	// Random angles in radians (from spherical coordinates)
	float phi = uniform_rand() * PI * 2.f;
	float theta = uniform_rand() * PI;

	// Calculate coordinates
	float sinPhiPerRadius = radius * std::sin(phi);

	float x = sinPhiPerRadius * std::cos(theta);
	float y = sinPhiPerRadius * std::sin(theta);
	float z = radius * std::cos(phi);
	return Vector3(x, y, z);
}

Vector3 EmitterArea::getPointOnHemisphereEdge(float radius, const Vector3& rightAxis, const Vector3& upAxis, const Vector3& forwardAxis)
{
	// Random angles in radians (from spherical coordinates)
	float phi = uniform_rand() * PI;
	float theta = uniform_rand() * PI;

	// Calculate coordinates
	float sinPhiPerRadius = radius * std::sin(phi);

	Vector3 x = (sinPhiPerRadius * std::cos(theta)) * rightAxis;
	Vector3 y = (sinPhiPerRadius * std::sin(theta)) * upAxis;
	Vector3 z = (radius * std::cos(phi)) * forwardAxis;
	return x + y + z;
}

void EmitterArea::drawCone(Transform* parent, const Vector3& color, float radius, bool depthEnabled) const
{
	auto asFloat3 = [](const Vector3& v) { return &v.x; };

	Vector3 right = parent->getRight();
	Vector3 up = parent->getUp(); // will be the circles' normal

	Vector3 bottomCircleCenter = parent->getGlobalMatrix().Translation();
	Vector3 topCircleCenter = bottomCircleCenter + up * m_topConeCircleHeight;

	Vector3 bottomLeftPoint = bottomCircleCenter - right * radius;
	Vector3 bottomRightPoint = bottomCircleCenter + right * radius;

	float topCircleRadius = radius * m_radiusScale;
	Vector3 topLeftPoint = topCircleCenter - right * topCircleRadius;
	Vector3 topRightPoint = topCircleCenter + right * topCircleRadius;

	dd::circle(asFloat3(bottomCircleCenter), asFloat3(up), asFloat3(color), m_radius, 20, 0, depthEnabled);
	dd::circle(asFloat3(topCircleCenter), asFloat3(up), asFloat3(color), topCircleRadius, 20, 0, depthEnabled);

	dd::line(asFloat3(bottomLeftPoint), asFloat3(topLeftPoint), asFloat3(color), 0, depthEnabled);
	dd::line(asFloat3(bottomRightPoint), asFloat3(topRightPoint), asFloat3(color), 0, depthEnabled);
}
