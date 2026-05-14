#include "Globals.h"
#include "EmitterArea.h"

#include "Transform.h"
#include "EmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "GameObject.h"

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
		break;
	}
}

bool EmitterArea::drawUi()
{
	bool parameterChanged = false;

	if (ImGui::CollapsingHeader("Area"))
	{
		{
			int shapeType = static_cast<int>(m_shapeType);
			if (ImGui::Combo("Shape", &shapeType, "Circle\0Cone\0Sphere\0Hemisphere\0", static_cast<int>(AreaType::TOTAL_TYPES)))
			{
				m_shapeType = static_cast<AreaType>(shapeType);
				parameterChanged |= true;
			}
		}

		parameterChanged |= ImGui::DragFloat("Radius", &m_radius, 0.1f, 0.0f);

		parameterChanged |= ImGui::DragFloat("Radius thickness", &m_radiusThickness, 0.1f, 0.0f, 1.0f);


		if (m_shapeType == AreaType::CONE) 
		{
			parameterChanged |= ImGui::DragFloat("Radius scale", &m_radiusScale, 0.1f, 1.0f, 100.f);
		}
	}

	return parameterChanged;
}

rapidjson::Value EmitterArea::getJSON(rapidjson::Document& domTree)
{
	return rapidjson::Value();
}

bool EmitterArea::deserializeJSON(const rapidjson::Value& componentValue)
{
	return false;
}

void EmitterArea::setNewParticlesPlacementCircle(EmitterInstance* particleData)
{
	Particle* particlePool = particleData->getParticlePool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();

	float realRadius = m_radiusThickness * m_radius;

	if (realRadius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getCircleDirection(objectTransform->getPosition(), *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectTransform->getPosition();
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the circle (for now we will assume that this position will mark the movement direction as well, from the center)

			float rightOffset;
			float forwardOffset;
			Vector3 spawnPosition;
			do
			{
				rightOffset = uniform_rand() * (realRadius * 2) - realRadius;
				forwardOffset = uniform_rand() * (realRadius * 2) - realRadius;

				spawnPosition = objectTransform->getPosition() + rightOffset * objectTransform->getRight() + forwardOffset * objectTransform->getForward();

			} while (Vector3::DistanceSquared(spawnPosition, objectTransform->getPosition()) > realRadius * realRadius); // Could be optimized by using Vector2 on x, z?


			if (rightOffset == 0.f && forwardOffset == 0.f) // similar case to realRadius == 0.f
			{
				Vector3 direction = getCircleDirection(objectTransform->getPosition(), *objectTransform);

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;

			}
			else {

				Vector3 direction = (spawnPosition - objectTransform->getPosition());
				direction.Normalize();

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;
			}
		}
	}
}

void EmitterArea::setNewParticlesPlacementSphere(EmitterInstance* particleData)
{
	Particle* particlePool = particleData->getParticlePool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();

	float realRadius = m_radiusThickness * m_radius;

	if (realRadius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getSphereDirection(objectTransform->getPosition(), *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectTransform->getPosition();
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the sphere (for now we will assume that this position will mark the movement direction as well, from the center)

			float rightOffset;
			float forwardOffset;
			float upOffset;
			Vector3 spawnPosition;
			do
			{
				rightOffset = uniform_rand() * (realRadius * 2) - realRadius;
				forwardOffset = uniform_rand() * (realRadius * 2) - realRadius;
				upOffset = uniform_rand() * (realRadius * 2) - realRadius;

				spawnPosition = objectTransform->getPosition() + rightOffset * objectTransform->getRight() + forwardOffset * objectTransform->getForward() + upOffset * objectTransform->getUp();

			} while (Vector3::DistanceSquared(spawnPosition, objectTransform->getPosition()) > realRadius * realRadius);


			if (rightOffset == 0.f && forwardOffset == 0.f && upOffset == 0.f) // similar case to realRadius == 0.f
			{
				Vector3 direction = getSphereDirection(objectTransform->getPosition(), *objectTransform);

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;

			}
			else {

				Vector3 direction = (spawnPosition - objectTransform->getPosition());
				direction.Normalize();

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;
			}
		}
	}
}

void EmitterArea::setNewParticlesPlacementHemisphere(EmitterInstance* particleData)
{
	Particle* particlePool = particleData->getParticlePool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();

	float realRadius = m_radiusThickness * m_radius;

	if (realRadius == 0.f) // new particles will always appear from center (we only need a direction) 
	{
		for (auto& particleIndex : newParticles)
		{
			Vector3 direction = getHemisphereDirection(objectTransform->getPosition(), *objectTransform); // in the future, we may add offsets to the position

			particlePool[particleIndex].position = objectTransform->getPosition();
			particlePool[particleIndex].movementDirection = direction;
		}

	}
	else {

		for (auto& particleIndex : newParticles)
		{
			// We need to obtain a position to appear within the hemisphere (for now we will assume that this position will mark the movement direction as well, from the center)

			float rightOffset;
			float forwardOffset;
			float upOffset;
			Vector3 spawnPosition;
			do
			{
				rightOffset = uniform_rand() * (realRadius * 2) - realRadius;
				forwardOffset = uniform_rand() * (realRadius * 2) - realRadius;
				upOffset = uniform_rand() * realRadius; // different from the others, because it has to be positive or 0

				spawnPosition = objectTransform->getPosition() + rightOffset * objectTransform->getRight() + forwardOffset * objectTransform->getForward() + upOffset * objectTransform->getUp();

			} while (Vector3::DistanceSquared(spawnPosition, objectTransform->getPosition()) > realRadius * realRadius);


			if (rightOffset == 0.f && forwardOffset == 0.f && upOffset == 0.f) // similar case to realRadius == 0.f
			{
				Vector3 direction = getHemisphereDirection(objectTransform->getPosition(), *objectTransform);

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;

			}
			else {

				Vector3 direction = (spawnPosition - objectTransform->getPosition());
				direction.Normalize();

				particlePool[particleIndex].position = spawnPosition;
				particlePool[particleIndex].movementDirection = direction;
			}
		}
	}
}

void EmitterArea::setNewParticlesPlacementCone(EmitterInstance* particleData)
{
	Particle* particlePool = particleData->getParticlePool();
	std::vector<unsigned int>& newParticles = particleData->getNewParticles();

	Transform* objectTransform = particleData->getParticleSystemComponent()->getOwner()->GetTransform();

	float realRadius = m_radiusThickness * m_radius;

	for (auto& particleIndex : newParticles)
	{
		// We need to obtain a position to appear within the circle
		float rightOffset;
		float forwardOffset;
		Vector3 spawnPosition;
		do
		{
			rightOffset = uniform_rand() * (realRadius * 2) - realRadius;
			forwardOffset = uniform_rand() * (realRadius * 2) - realRadius;

			spawnPosition = objectTransform->getPosition() + rightOffset * objectTransform->getRight() + forwardOffset * objectTransform->getForward();

		} while (Vector3::DistanceSquared(spawnPosition, objectTransform->getPosition()) > realRadius * realRadius); // Could be optimized by using Vector2 on x, z?

		// For now, we will use a scale on the offsets, to define the "upper circle" of the cone, and assume it is on an arbitrary height to get a direction

		Vector3 scaledPosition = objectTransform->getPosition() + m_radiusScale * rightOffset * objectTransform->getRight() + m_radiusScale * forwardOffset * objectTransform->getForward() + objectTransform->getUp();
		Vector3 direction = (scaledPosition - spawnPosition);
		direction.Normalize();

		particlePool[particleIndex].position = spawnPosition;
		particlePool[particleIndex].movementDirection = direction;
	}
}

Vector3 EmitterArea::getCircleDirection(Vector3 center, const Transform& objectTransform)
{
	float rightOffset;
	float forwardOffset;

	do 
	{
		rightOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511);

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
		rightOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511);
		upOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511);

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
		rightOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511); // 'and' is because x % 2^n == x & (2^n - 1) for positive numbers; << 1 is for double (we are trying to generate a value in the range [-511, 511], because 512 is even and we can use the 'and' trick for its mod)
		forwardOffset = static_cast<float>((rand() & (512 - 1)) << 1 - 511);
		upOffset = static_cast<float>(rand() & (512 - 1)); // This HAS to be different, because up will always need to be positive or 0

	} while (rightOffset == 0.f and forwardOffset == 0.f and upOffset == 0.f);

	Vector3 pointToDirection = center + rightOffset * objectTransform.getRight() + forwardOffset * objectTransform.getForward() + upOffset * objectTransform.getUp();

	Vector3 direction = (pointToDirection - center);
	direction.Normalize();
	return direction;
}
