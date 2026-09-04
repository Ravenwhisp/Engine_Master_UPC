#include "pch.h"
#include "AelorinDetectionAggro.h"

IMPLEMENT_SCRIPT_FIELDS(AelorinDetectionAggro,
	SERIALIZED_FLOAT(m_detectionRadius, "Detection Radius", 0.0f, 50.0f, 0.1f),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled"),
	SERIALIZED_COMPONENT_REF(m_lyrielTransform, "Lyriel Transform", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_deathTransform, "Death Transform", ComponentType::TRANSFORM)
)

AelorinDetectionAggro::AelorinDetectionAggro(GameObject* owner) : Script(owner) {}

void AelorinDetectionAggro::Start()
{
	findPlayerTransforms();
}

void AelorinDetectionAggro::Update()
{
}

void AelorinDetectionAggro::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

	const Vector3 white = { 1.0f, 1.0f, 1.0f };
	const Vector3 red = { 1.0f, 0.0f, 0.0f };

	Vector3 debugPosition = getOwnerPosition() + Vector3(0.0f, 0.2f, 0.0f);

	DebugDrawAPI::drawCircle(debugPosition, Vector3(0.0f, 1.0f, 0.0f), white, m_detectionRadius, 24.0f, 0, true);

	if (isLyrielInDetectionRange())
	{
		DebugDrawAPI::drawLine(debugPosition, getLyrielPosition(), red, 0, true);
	}

	if (isDeathInDetectionRange())
	{
		DebugDrawAPI::drawLine(debugPosition, getDeathPosition(), red, 0, true);
	}
}

void AelorinDetectionAggro::findPlayerTransforms()
{
	m_lyrielCachedTransform = m_lyrielTransform.getReferencedComponent();
	m_deathCachedTransform = m_deathTransform.getReferencedComponent();

	if (m_lyrielCachedTransform && m_deathCachedTransform)
		return;

	const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
	for (GameObject* player : players)
	{
		const char* name = GameObjectAPI::getName(player);
		if (!name)
			continue;

		if (!m_lyrielCachedTransform && strcmp(name, "Lyriel") == 0)
			m_lyrielCachedTransform = GameObjectAPI::getTransform(player);

		if (!m_deathCachedTransform && strcmp(name, "Death") == 0)
			m_deathCachedTransform = GameObjectAPI::getTransform(player);

		if (m_lyrielCachedTransform && m_deathCachedTransform)
			break;
	}
}

Transform* AelorinDetectionAggro::getOwnerTransform() const
{
	return GameObjectAPI::getTransform(getOwner());
}

Vector3 AelorinDetectionAggro::getOwnerPosition() const
{
	Transform* ownerTransform = getOwnerTransform();
	if (!ownerTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getGlobalPosition(ownerTransform);
}

Transform* AelorinDetectionAggro::getLyrielTransform() const
{
	Transform* ref = m_lyrielTransform.getReferencedComponent();
	return ref ? ref : m_lyrielCachedTransform;
}

Transform* AelorinDetectionAggro::getDeathTransform() const
{
	Transform* ref = m_deathTransform.getReferencedComponent();
	return ref ? ref : m_deathCachedTransform;
}

Vector3 AelorinDetectionAggro::getLyrielPosition() const
{
	Transform* lyrielTransform = getLyrielTransform();
	if (!lyrielTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}
	
	return TransformAPI::getGlobalPosition(lyrielTransform);
}

Vector3 AelorinDetectionAggro::getDeathPosition() const
{
	Transform* deathTransform = getDeathTransform();
	if (!deathTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getGlobalPosition(deathTransform);
}

float AelorinDetectionAggro::getDistanceToLyriel() const
{
	Vector3 difference = getLyrielPosition() - getOwnerPosition();
	return difference.Length();
}

float AelorinDetectionAggro::getDistanceToDeath() const
{
	Vector3 difference = getDeathPosition() - getOwnerPosition();
	return difference.Length();
}

bool AelorinDetectionAggro::isLyrielInDetectionRange() const
{
	if (!getLyrielTransform())
	{
		return false;
	}

	return getDistanceToLyriel() <= m_detectionRadius;
}

bool AelorinDetectionAggro::isDeathInDetectionRange() const
{
	if (!getDeathTransform())
	{
		return false;
	}

	return getDistanceToDeath() <= m_detectionRadius;
}

bool AelorinDetectionAggro::startEncounter() const
{
	if (isLyrielInDetectionRange() || isDeathInDetectionRange())
		return true;
	
	return false;
}

IMPLEMENT_SCRIPT(AelorinDetectionAggro)