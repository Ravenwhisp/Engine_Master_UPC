#include "Globals.h"
#include "Quadtree.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"

#include <algorithm>
#include <limits>

Quadtree::Quadtree() = default;
Quadtree::~Quadtree() = default;

void Quadtree::init(Scene* scene)
{
    m_scene = scene;
}

void Quadtree::build()
{
    const std::vector<GameObject*>& objects = m_scene->getAllGameObjects();
    float minX = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& go : objects)
    {
        if (!go->GetActive())
            continue;

        auto* mesh = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (!mesh)
            continue;

        Engine::BoundingBox boundingBox = mesh->getBoundingBox();

        Vector3 wmin(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3 wmax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        const Vector3* pts = boundingBox.getPoints();

        for (int i = 0; i < 8; ++i)
        {
            wmin.x = std::min(wmin.x, pts[i].x);
            wmin.z = std::min(wmin.z, pts[i].z);

            wmax.x = std::max(wmax.x, pts[i].x);
            wmax.z = std::max(wmax.z, pts[i].z);
        }

        minX = std::min(minX, wmin.x);
        minZ = std::min(minZ, wmin.z);
        maxX = std::max(maxX, wmax.x);
        maxZ = std::max(maxZ, wmax.z);
    }

    if (minX == std::numeric_limits<float>::max())
    {
        return;
    }

    BoundingRect rect(minX, minZ, maxX - minX, maxZ - minZ);

    m_root = std::make_unique<QuadNode>(rect, 0, *this, nullptr);

    for (const auto& go : objects)
    {
        if (go->GetActive())
        {
            insert(*go);
        }
    }
}

void Quadtree::update()
{
    const std::vector<GameObject*>& objects = m_scene->getAllGameObjects();
    if (!m_root)
    {
        return;
    }

    for (const auto& go : objects)
    {
        if (!go->GetActive())
            continue;

        if (go->GetTransform()->isDirty())
        {
            move(*go);
        }
    }

    resolveDirtyNodes();
}

void Quadtree::clear()
{
    m_dirtyNodes.clear();
    m_objectLocationMap.clear();
}

void Quadtree::insert(GameObject& object)
{
    if (!m_root)
        return;

    m_root->insert(object);
}

void Quadtree::remove(GameObject& object)
{
    auto it = m_objectLocationMap.find(&object);

    if (it == m_objectLocationMap.end())
        return;

    QuadNode* node = it->second;

    node->remove(object);
}

void Quadtree::move(GameObject& object)
{
    auto it = m_objectLocationMap.find(&object);

    if (it == m_objectLocationMap.end())
    {
        insert(object);
        return;
    }

    it->second->refit(object);
}

std::vector<GameObject*> Quadtree::query() const
{
    auto frustum = m_scene->getDefaultCamera()->getFrustum();
    std::vector<GameObject*> result;

    if (!m_root)
        return result;

    m_root->gatherObjects(frustum, result);

    return result;
}

std::vector<BoundingRect> Quadtree::getQuadrants() const
{
    std::vector<BoundingRect> result;

    if (!m_root)
        return result;

    m_root->gatherRectangles(result);

    return result;
}

void Quadtree::registerDirtyNode(QuadNode* node)
{
    m_dirtyNodes.push_back(node);
}

void Quadtree::resolveDirtyNodes()
{
    std::sort(
        m_dirtyNodes.begin(),
        m_dirtyNodes.end(),
        [](QuadNode* a, QuadNode* b)
        {
            return a->getDepth() > b->getDepth();
        });

    for (QuadNode* node : m_dirtyNodes)
    {
        if (node->canMerge())
        {
            node->merge();
        }

        node->clearDirty();
    }

    m_dirtyNodes.clear();
}