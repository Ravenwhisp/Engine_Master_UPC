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

void EmitterArea::debugDraw(Transform* parent)
{
	bool depthEnabled = false;
	auto asFloat3 = [](const Vector3& v) { return &v.x; };

	Vector3 position = parent->getPosition();

	switch (m_shapeType)
	{

	case AreaType::CIRCLE:
	
	{
		Vector3 circleNormal = parent->getUp();

		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor*m_thicknessAreaColor), m_radius*m_radiusThickness, 20, 0, depthEnabled);  // thickness radius
		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor), m_radius, 20, 0, depthEnabled);
		
	}
		break;

	case AreaType::CONE:

		break;

	case AreaType::SPHERE:

		dd::sphere(asFloat3(position), asFloat3(m_areaColor*m_thicknessAreaColor), m_radius*m_radiusThickness, 0, depthEnabled); // thickness radius
		dd::sphere(asFloat3(position), asFloat3(m_areaColor), m_radius, 0, depthEnabled);
		
		break;

	case AreaType::HEMISPHERE:

	{
		Vector3 circleNormal = parent->getUp();
		Vector3 thicknessColor = m_areaColor * m_thicknessAreaColor;

		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(thicknessColor), m_radius * m_radiusThickness, 20, 0, depthEnabled);  // thickness radius circle
		dd::circle(asFloat3(position), asFloat3(circleNormal), asFloat3(m_areaColor), m_radius, 20, 0, depthEnabled);

		Vector3 hemisphereHeight = circleNormal * m_radius;
		dd::arrow(asFloat3(position), asFloat3(position + hemisphereHeight * m_radiusThickness), asFloat3(thicknessColor), 0.25f, 0, depthEnabled); // thickness hemisphere height
		dd::arrow(asFloat3(position), asFloat3(position + hemisphereHeight), asFloat3(m_areaColor), 0.25f, 0, depthEnabled); // hemisphere height

	}

		break;
	}
}

rapidjson::Value EmitterArea::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value moduleInfo(rapidjson::kObjectType);

	moduleInfo.AddMember("ModuleType", unsigned int(ParticleModuleType::AREA), domTree.GetAllocator());

	moduleInfo.AddMember("ShapeType", unsigned int(m_shapeType), domTree.GetAllocator());

	moduleInfo.AddMember("Radius", m_radius, domTree.GetAllocator());
	moduleInfo.AddMember("RadiusThickness", m_radiusThickness, domTree.GetAllocator());

	if (m_shapeType == AreaType::CONE) moduleInfo.AddMember("RadiusScale", m_radiusScale, domTree.GetAllocator());

	return moduleInfo;
}

bool EmitterArea::deserializeJSON(const rapidjson::Value& moduleInfo)
{
	if (moduleInfo.HasMember("ShapeType"))
	{
		unsigned int shapeUInt = moduleInfo["ShapeType"].GetUint();
		AreaType shapeType = static_cast<AreaType>(shapeUInt);

		switch (shapeType) {
		
		case AreaType::CIRCLE:

			m_shapeType = AreaType::CIRCLE;

			if (moduleInfo.HasMember("Radius")) 
			{
				m_radius = moduleInfo["Radius"].GetFloat();
			}
			if (moduleInfo.HasMember("RadiusThickness"))
			{
				m_radiusThickness = moduleInfo["RadiusThickness"].GetFloat();
			}

			break;

		case AreaType::CONE:

			m_shapeType = AreaType::CONE;

			if (moduleInfo.HasMember("Radius"))
			{
				m_radius = moduleInfo["Radius"].GetFloat();
			}
			if (moduleInfo.HasMember("RadiusThickness"))
			{
				m_radiusThickness = moduleInfo["RadiusThickness"].GetFloat();
			}
			if (moduleInfo.HasMember("RadiusScale")) // only this one has it
			{
				m_radiusScale = moduleInfo["RadiusScale"].GetFloat();
			}

			break;
		
		case AreaType::HEMISPHERE:

			m_shapeType = AreaType::HEMISPHERE;

			if (moduleInfo.HasMember("Radius"))
			{
				m_radius = moduleInfo["Radius"].GetFloat();
			}
			if (moduleInfo.HasMember("RadiusThickness"))
			{
				m_radiusThickness = moduleInfo["RadiusThickness"].GetFloat();
			}

			break;

		case AreaType::SPHERE:

			m_shapeType = AreaType::SPHERE;

			if (moduleInfo.HasMember("Radius"))
			{
				m_radius = moduleInfo["Radius"].GetFloat();
			}
			if (moduleInfo.HasMember("RadiusThickness"))
			{
				m_radiusThickness = moduleInfo["RadiusThickness"].GetFloat();
			}

		}
	}

	return true;
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
