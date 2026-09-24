#include "pch.h"
#include "ParticleManager.h"

#include "ParticleLifecycle.h"

#include <algorithm>
#include <cmath>

ParticleManager* ParticleManager::s_instance = nullptr;
std::vector<GameObject*> ParticleManager::s_pendingRegistrations;

namespace
{
    // visitParticleSystems takes a plain function pointer, so the result is
    // shared through this flag. Registration is not reentrant.
    bool s_foundParticleSystem = false;
    void markParticleSystemFound(ParticleSystemComponent*) { s_foundParticleSystem = true; }

    struct CameraView
    {
        Vector3 position;
        Vector3 forward;
        Vector3 right;
        Vector3 up;
        float   nearPlane         = 0.0f;
        float   farPlane          = 0.0f;
        float   tanHalfHorizontal = 0.0f;
        float   tanHalfVertical   = 0.0f;
    };

    float degreesToRadians(float degrees)
    {
        return degrees * (MathAPI::PI / 180.0f);
    }

    // Point-in-frustum test built from the camera basis vectors. The engine
    // camera FOV is horizontal; the vertical extent derives from the aspect.
    bool isPointVisible(const CameraView& view, const Vector3& point, float marginDegrees)
    {
        const Vector3 toPoint = point - view.position;
        const float   depth   = toPoint.Dot(view.forward);

        if (depth < view.nearPlane || depth > view.farPlane)
        {
            return false;
        }

        // Angular margin converted to linear size at the point's depth.
        const float margin     = depth * tanf(degreesToRadians(marginDegrees));
        const float halfWidth  = depth * view.tanHalfHorizontal + margin;
        const float halfHeight = depth * view.tanHalfVertical + margin;

        const float x = toPoint.Dot(view.right);
        const float y = toPoint.Dot(view.up);

        return x >= -halfWidth && x <= halfWidth && y >= -halfHeight && y <= halfHeight;
    }

    void drawFrustumLines(const CameraView& view, float marginDegrees, const Vector3& color)
    {
        const float marginTan = tanf(degreesToRadians(marginDegrees));
        const float tanH      = view.tanHalfHorizontal + marginTan;
        const float tanV      = view.tanHalfVertical + marginTan;

        // Cap the drawn depth so the gizmo stays readable with large far planes.
        const float farDepth = view.farPlane < 100.0f ? view.farPlane : 100.0f;

        const Vector3 nearCenter = view.position + view.forward * view.nearPlane;
        const Vector3 farCenter  = view.position + view.forward * farDepth;

        const float nearH = view.nearPlane * tanH;
        const float nearV = view.nearPlane * tanV;
        const float farH  = farDepth * tanH;
        const float farV  = farDepth * tanV;

        Vector3 nearCorners[4];
        Vector3 farCorners[4];

        nearCorners[0] = nearCenter - view.right * nearH + view.up * nearV;
        nearCorners[1] = nearCenter + view.right * nearH + view.up * nearV;
        nearCorners[2] = nearCenter + view.right * nearH - view.up * nearV;
        nearCorners[3] = nearCenter - view.right * nearH - view.up * nearV;

        farCorners[0] = farCenter - view.right * farH + view.up * farV;
        farCorners[1] = farCenter + view.right * farH + view.up * farV;
        farCorners[2] = farCenter + view.right * farH - view.up * farV;
        farCorners[3] = farCenter - view.right * farH - view.up * farV;

        for (int i = 0; i < 4; ++i)
        {
            const int next = (i + 1) % 4;
            DebugDrawAPI::drawLine(nearCorners[i], nearCorners[next], color, 0, false);
            DebugDrawAPI::drawLine(farCorners[i], farCorners[next], color, 0, false);
            DebugDrawAPI::drawLine(nearCorners[i], farCorners[i], color, 0, false);
        }
    }
}

IMPLEMENT_SCRIPT_FIELDS(ParticleManager,
    SERIALIZED_FLOAT(m_checkIntervalSeconds, "Check Interval (s)", 0.1f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_enterMarginDegrees, "Enter Margin (deg)", 0.0f, 45.0f, 0.5f),
    SERIALIZED_FLOAT(m_exitMarginDegrees, "Exit Margin (deg)", 0.0f, 45.0f, 0.5f),
    SERIALIZED_FLOAT(m_aspectRatio, "Aspect Ratio", 0.5f, 4.0f, 0.01f),
    SERIALIZED_BOOL(m_manageAllParticles, "Manage All Particles")
)

ParticleManager::ParticleManager(GameObject* owner)
    : Script(owner)
{
}

void ParticleManager::Start()
{
    // Hysteresis requires the exit margin to be the larger one.
    if (m_exitMarginDegrees < m_enterMarginDegrees)
    {
        m_exitMarginDegrees = m_enterMarginDegrees;
    }

    if (m_aspectRatio <= 0.0f)
    {
        m_aspectRatio = 1.0f;
    }

    m_timer = 0.0f;
    s_instance = this;
    adoptPendingRegistrations();

    if (m_manageAllParticles)
    {
        scanAndRegisterAll();
    }

    Debug::log("[ParticleManager] Managing %zu VFX roots (manageAll=%s), camera culling enter=%.1f deg, exit=%.1f deg, interval=%.2fs.",
        m_managedParticles.size(), m_manageAllParticles ? "true" : "false",
        m_enterMarginDegrees, m_exitMarginDegrees, m_checkIntervalSeconds);
}

void ParticleManager::OnGameStop()
{
    // Leave every managed object as it was before the manager touched it.
    for (ManagedParticle& entry : m_managedParticles)
    {
        if (entry.deactivatedByManager && entry.gameObject != nullptr && SceneAPI::containsGameObject(entry.gameObject))
        {
            GameObjectAPI::setActive(entry.gameObject, true);
        }
    }

    m_managedParticles.clear();
    s_pendingRegistrations.clear();

    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

void ParticleManager::Update()
{
    m_timer += Time::getDeltaTime();
    if (m_timer >= m_checkIntervalSeconds)
    {
        m_timer -= m_checkIntervalSeconds;
        pruneInvalidEntries();
        updateActivity();
    }
}

void ParticleManager::registerVfxRoot(GameObject* root)
{
    if (root == nullptr)
    {
        return;
    }

    if (s_instance != nullptr)
    {
        s_instance->registerRoot(root);
        return;
    }

    // No manager in the scene yet: keep the root until one starts.
    if (std::find(s_pendingRegistrations.begin(), s_pendingRegistrations.end(), root) == s_pendingRegistrations.end())
    {
        s_pendingRegistrations.push_back(root);
    }
}

void ParticleManager::unregisterVfxRoot(GameObject* root)
{
    if (root == nullptr)
    {
        return;
    }

    auto pendingIt = std::find(s_pendingRegistrations.begin(), s_pendingRegistrations.end(), root);
    if (pendingIt != s_pendingRegistrations.end())
    {
        s_pendingRegistrations.erase(pendingIt);
    }

    if (s_instance == nullptr)
    {
        return;
    }

    std::vector<ManagedParticle>& entries = s_instance->m_managedParticles;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (entries[i].gameObject == root)
        {
            if (entries[i].deactivatedByManager)
            {
                GameObjectAPI::setActive(root, true);
                ParticleLifecycle::restart(root);
            }

            entries.erase(entries.begin() + i);
            return;
        }
    }
}

bool ParticleManager::registerRoot(GameObject* root)
{
    if (!isValidVfxRoot(root))
    {
        return false;
    }

    for (const ManagedParticle& entry : m_managedParticles)
    {
        if (entry.gameObject == root)
        {
            return true;
        }
    }

    ManagedParticle entry;
    entry.gameObject = root;
    m_managedParticles.push_back(entry);
    return true;
}

void ParticleManager::adoptPendingRegistrations()
{
    for (GameObject* root : s_pendingRegistrations)
    {
        registerRoot(root);
    }

    s_pendingRegistrations.clear();
}

void ParticleManager::scanAndRegisterAll()
{
    const std::vector<GameObject*> all = SceneAPI::findAllGameObjectsByComponent(
        ComponentType::PARTICLE_SYSTEM, false);

    for (GameObject* obj : all)
    {
        if (obj == nullptr || obj == getOwner())
        {
            continue;
        }

        // Skip objects already covered by an explicitly registered root.
        if (isUnderManagedRoot(obj))
        {
            continue;
        }

        registerRoot(obj);
    }
}

bool ParticleManager::isUnderManagedRoot(GameObject* obj) const
{
    Transform* transform = GameObjectAPI::getTransform(obj);

    while (transform != nullptr)
    {
        GameObject* current = ComponentAPI::getOwner(transform);
        if (current == nullptr)
        {
            return false;
        }

        for (const ManagedParticle& entry : m_managedParticles)
        {
            if (entry.gameObject == current)
            {
                return true;
            }
        }

        transform = TransformAPI::getParent(transform);
    }

    return false;
}

void ParticleManager::pruneInvalidEntries()
{
    for (size_t i = m_managedParticles.size(); i-- > 0;)
    {
        GameObject* obj = m_managedParticles[i].gameObject;
        if (obj == nullptr || !SceneAPI::containsGameObject(obj))
        {
            m_managedParticles.erase(m_managedParticles.begin() + static_cast<std::ptrdiff_t>(i));
        }
    }
}

bool ParticleManager::isValidVfxRoot(GameObject* root) const
{
    if (root == nullptr || root == getOwner())
    {
        return false;
    }

    if (!SceneAPI::containsGameObject(root))
    {
        return false;
    }

    s_foundParticleSystem = false;
    ParticleLifecycle::visitParticleSystems(root, &markParticleSystemFound);

    if (!s_foundParticleSystem)
    {
        Debug::warn("[ParticleManager] Rejected VFX root '%s': no particle system found in its hierarchy.",
            GameObjectAPI::getName(root));
        return false;
    }

    return true;
}

void ParticleManager::updateActivity()
{
    GameObject* cameraObject = SceneAPI::getDefaultCameraGameObject();
    if (cameraObject == nullptr)
    {
        return;
    }

    Transform*       cameraTransform = GameObjectAPI::getTransform(cameraObject);
    CameraComponent* camera          = CameraAPI::getCameraComponent(cameraObject);
    if (cameraTransform == nullptr || camera == nullptr)
    {
        return;
    }

    CameraView view;
    view.position = TransformAPI::getGlobalPosition(cameraTransform);
    view.forward  = TransformAPI::getForward(cameraTransform);
    view.right    = TransformAPI::getRight(cameraTransform);
    view.up       = TransformAPI::getUp(cameraTransform);
    view.nearPlane = CameraAPI::getNearPlane(camera);
    view.farPlane  = CameraAPI::getFarPlane(camera);

    view.tanHalfHorizontal = tanf(degreesToRadians(CameraAPI::getFov(camera)) * 0.5f);
    view.tanHalfVertical   = view.tanHalfHorizontal / m_aspectRatio;

    for (ManagedParticle& entry : m_managedParticles)
    {
        GameObject* obj = entry.gameObject;

        Transform* transform = GameObjectAPI::getTransform(obj);
        if (transform == nullptr)
        {
            continue;
        }

        const Vector3 position = TransformAPI::getGlobalPosition(transform);
        const bool    isActive = GameObjectAPI::isActiveSelf(obj);

        if (isActive && !isPointVisible(view, position, m_exitMarginDegrees))
        {
            ParticleLifecycle::stop(obj);
            GameObjectAPI::setActive(obj, false);
            entry.deactivatedByManager = true;
        }
        else if (!isActive && entry.deactivatedByManager && isPointVisible(view, position, m_enterMarginDegrees))
        {
            GameObjectAPI::setActive(obj, true);
            ParticleLifecycle::restart(obj);
            entry.deactivatedByManager = false;
        }
        else if (isActive)
        {
            // Reactivated by gameplay or back in view: the manager no longer
            // owns its inactive state.
            entry.deactivatedByManager = false;
        }
    }
}

void ParticleManager::drawGizmo()
{
    GameObject* cameraObject = SceneAPI::getDefaultCameraGameObject();
    if (cameraObject == nullptr)
    {
        return;
    }

    Transform*       cameraTransform = GameObjectAPI::getTransform(cameraObject);
    CameraComponent* camera          = CameraAPI::getCameraComponent(cameraObject);
    if (cameraTransform == nullptr || camera == nullptr)
    {
        return;
    }

    CameraView view;
    view.position = TransformAPI::getGlobalPosition(cameraTransform);
    view.forward  = TransformAPI::getForward(cameraTransform);
    view.right    = TransformAPI::getRight(cameraTransform);
    view.up       = TransformAPI::getUp(cameraTransform);
    view.nearPlane = CameraAPI::getNearPlane(camera);
    view.farPlane  = CameraAPI::getFarPlane(camera);

    view.tanHalfHorizontal = tanf(degreesToRadians(CameraAPI::getFov(camera)) * 0.5f);
    view.tanHalfVertical   = view.tanHalfHorizontal / m_aspectRatio;

    drawFrustumLines(view, m_enterMarginDegrees, Vector3(0.0f, 1.0f, 0.5f));
    drawFrustumLines(view, m_exitMarginDegrees, Vector3(1.0f, 0.6f, 0.1f));
}

IMPLEMENT_SCRIPT(ParticleManager)
