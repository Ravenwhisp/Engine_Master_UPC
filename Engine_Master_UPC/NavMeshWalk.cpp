#include "Globals.h"
#include "NavMeshWalk.h"

#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "TimeModule.h"
#include "InputModule.h"
#include "NavigationModule.h"

#include <DetourNavMeshQuery.h>
#include <imgui.h>
#include <cmath>

static const float PI = 3.1415926535897931f;

NavMeshWalk::NavMeshWalk(UID id, GameObject* gameobject)
    : Component(id, ComponentType::NAVMESH_WALK, gameobject)
{
}

std::unique_ptr<Component> NavMeshWalk::clone(GameObject* newOwner) const
{
    auto c = std::make_unique<NavMeshWalk>(m_uuid, newOwner);

    // Copy state
    c->setActive(this->isActive());
    c->m_moveSpeed = m_moveSpeed;
    c->m_shiftMultiplier = m_shiftMultiplier;
    c->m_turnSpeedDegPerSec = m_turnSpeedDegPerSec;

    c->m_keyUp = m_keyUp;
    c->m_keyDown = m_keyDown;
    c->m_keyLeft = m_keyLeft;
    c->m_keyRight = m_keyRight;

    c->m_constrainToNavMesh = m_constrainToNavMesh;
    c->m_navExtents[0] = m_navExtents[0];
    c->m_navExtents[1] = m_navExtents[1];
    c->m_navExtents[2] = m_navExtents[2];

    // Reset runtime cache
    c->m_yawInitialized = false;
    c->m_currentYawDeg = 0.0f;

    return c;
}

float NavMeshWalk::getDeltaSecondsFromTimer() const
{
    return app->getTimeModule()->deltaTime();
}

void NavMeshWalk::update()
{
    Transform* transform = m_owner->GetTransform();
    InputModule* inputModule = app->getInputModule();

    if (!inputModule->isLeftMouseDown())
        return;

    Vector3 direction = readMoveDirection(inputModule);
    if (direction == Vector3::Zero)
        return;

    direction.Normalize();

    const float dt = getDeltaSecondsFromTimer();
    bool shiftHeld = checkShiftHeld(inputModule);

    applyFacingFromDirection(transform, direction, dt);
    applyTranslation(transform, direction, dt, shiftHeld);
}

Vector3 NavMeshWalk::readMoveDirection(InputModule* inputModule) const
{
    Vector3 direction(0, 0, 0);

    if (inputModule->isKeyDown(m_keyUp))    direction.z -= 1.0f;
    if (inputModule->isKeyDown(m_keyDown))  direction.z += 1.0f;
    if (inputModule->isKeyDown(m_keyLeft))  direction.x -= 1.0f;
    if (inputModule->isKeyDown(m_keyRight)) direction.x += 1.0f;

    return direction;
}

bool NavMeshWalk::checkShiftHeld(InputModule* inputModule) const
{
    return inputModule->isKeyDown(Keyboard::Keys::LeftShift) ||
        inputModule->isKeyDown(Keyboard::Keys::RightShift);
}

void NavMeshWalk::applyFacingFromDirection(Transform* transform, const Vector3& direction, float dt)
{
    const float yawRad = std::atan2(-direction.x, -direction.z);
    const float targetYawDeg = yawRad * (180.0f / PI) + 90.0f;

    if (!m_yawInitialized)
    {
        m_currentYawDeg = transform->getEulerDegrees().y;
        m_yawInitialized = true;
    }

    const float maxStep = m_turnSpeedDegPerSec * dt;
    m_currentYawDeg = moveTowardsAngleDegrees(m_currentYawDeg, targetYawDeg, maxStep);

    transform->setRotationEuler(Vector3(0.0f, m_currentYawDeg, 0.0f));
}

void NavMeshWalk::applyTranslation(Transform* transform, const Vector3& direction, float dt, bool shiftHeld) const
{
    float speed = m_moveSpeed;
    if (shiftHeld) speed *= m_shiftMultiplier;

    float step = speed * dt;

    Vector3 currentPos = transform->getPosition();
    Vector3 desiredPos = currentPos + direction * step;

    if (!m_constrainToNavMesh)
    {
        transform->setPosition(desiredPos);
        return;
    }

    NavigationModule* nav = app->getNavigationModule();
    dtNavMeshQuery* q = nav ? nav->getNavQuery() : nullptr;

    if (!q)
    {
        transform->setPosition(desiredPos);
        return;
    }

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float start[3] = { currentPos.x, currentPos.y, currentPos.z };
    float end[3] = { desiredPos.x, desiredPos.y, desiredPos.z };

    dtPolyRef startRef = 0;
    float startNearest[3];

    if (dtStatusFailed(q->findNearestPoly(start, m_navExtents, &filter, &startRef, startNearest)) || !startRef)
    {
        return;
    }

    dtPolyRef visited[64];
    int nvisited = 0;
    float result[3];

    const dtStatus st = q->moveAlongSurface(startRef, startNearest, end, &filter, result, visited, &nvisited, 64);
    if (dtStatusFailed(st))
        return;

    dtPolyRef lastRef = (nvisited > 0) ? visited[nvisited - 1] : startRef;
    float h = result[1];
    q->getPolyHeight(lastRef, result, &h);

    transform->setPosition(Vector3(result[0], h, result[2]));
}

float NavMeshWalk::wrapAngleDegrees(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float NavMeshWalk::moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta)
{
    float delta = wrapAngleDegrees(targetYawAngle - currentYawAngle);

    if (delta > maxDelta)  delta = maxDelta;
    if (delta < -maxDelta) delta = -maxDelta;

    return currentYawAngle + delta;
}

void NavMeshWalk::drawUi()
{
    ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1), "NOTE: NavMeshWalk only updates in PLAY mode.");
    ImGui::Separator();

    ImGui::Text("Hold Left Mouse Button + WASD to move (NavMesh constrained)");
    ImGui::Text("Press shift to go faster");
    ImGui::Separator();

    ImGui::Checkbox("Constrain to NavMesh", &m_constrainToNavMesh);
    ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.05f, 0.0f, 50.0f);
    ImGui::DragFloat("Shift Multiplier", &m_shiftMultiplier, 0.05f, 1.0f, 10.0f);
    ImGui::DragFloat3("Nav Extents", m_navExtents, 0.1f, 0.1f, 100.0f);
}

rapidjson::Value NavMeshWalk::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::NAVMESH_WALK), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("MoveSpeed", m_moveSpeed, domTree.GetAllocator());
    componentInfo.AddMember("ShiftMultiplier", m_shiftMultiplier, domTree.GetAllocator());
    componentInfo.AddMember("ConstrainToNavMesh", m_constrainToNavMesh, domTree.GetAllocator());

    rapidjson::Value ext(rapidjson::kArrayType);
    ext.PushBack(m_navExtents[0], domTree.GetAllocator());
    ext.PushBack(m_navExtents[1], domTree.GetAllocator());
    ext.PushBack(m_navExtents[2], domTree.GetAllocator());
    componentInfo.AddMember("NavExtents", ext, domTree.GetAllocator());

    return componentInfo;
}

bool NavMeshWalk::deserializeJSON(const rapidjson::Value& c)
{
    if (c.HasMember("MoveSpeed")) m_moveSpeed = c["MoveSpeed"].GetFloat();
    if (c.HasMember("ShiftMultiplier")) m_shiftMultiplier = c["ShiftMultiplier"].GetFloat();
    if (c.HasMember("ConstrainToNavMesh")) m_constrainToNavMesh = c["ConstrainToNavMesh"].GetBool();

    if (c.HasMember("NavExtents"))
    {
        const auto& ext = c["NavExtents"].GetArray();
        if (ext.Size() == 3)
        {
            m_navExtents[0] = ext[0].GetFloat();
            m_navExtents[1] = ext[1].GetFloat();
            m_navExtents[2] = ext[2].GetFloat();
        }
    }

    m_yawInitialized = false;
    m_currentYawDeg = 0.0f;

    return true;
}