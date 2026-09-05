#include "Globals.h"
#include "Quadtree.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Settings.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

Quadtree::Quadtree() = default;
Quadtree::~Quadtree() = default;

void Quadtree::init(Scene* scene, const ddVec3& baseColor, const ddVec3& culledColor)
{
    m_scene = scene;
    std::copy(std::begin(baseColor), std::end(baseColor), m_debugBaseColor);
    std::copy(std::begin(culledColor), std::end(culledColor), m_debugCulledColor);
}

void Quadtree::build(const std::vector<Layer> layers)
{
    m_layers = layers;

    const std::vector<GameObject*>& objects = m_scene->getAllGameObjects();
    float minX = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    size_t eligible = 0;

    for (const auto& go : objects)
    {
        // Inactive objects are included too: gatherObjects filters by active
        // per query, and skipping them here would leave them untracked when
        // they are activated later.
        const bool layerMatch = layers.empty() || std::find(layers.begin(), layers.end(), go->GetLayer()) != layers.end();
        if (!layerMatch)
        {
            continue;
        }

        auto* mesh = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (!mesh || !mesh->hasMesh())
        {
            continue;
        }

        ++eligible;

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
        DEBUG_WARN("[Quadtree] Build found NO MeshRenderers with meshes (total GOs: %zu). "
                   "Queries will return nothing until an object is inserted.", objects.size());
        isBuilded = true;
        return;
    }

    BoundingRect rect(
        minX - app->getSettings()->frustumCulling.quadtreeXExtraSize, 
        minZ - app->getSettings()->frustumCulling.quadtreeZExtraSize,
        maxX - minX + app->getSettings()->frustumCulling.quadtreeXExtraSize * 2,
        maxZ - minZ + app->getSettings()->frustumCulling.quadtreeZExtraSize * 2
    );

    m_root = std::make_unique<QuadNode>(rect, 0, *this, nullptr);

    for (const auto& go : objects)
    {
        const bool layerMatch =
            layers.empty() ||
            std::find(layers.begin(), layers.end(), go->GetLayer()) != layers.end();

        if (!layerMatch)
        {
            continue;
        }

        auto* mesh = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
        if (!mesh || !mesh->hasMesh())
        {
            continue;
        }

        insert(*go);
    }

    DEBUG_LOG("[Quadtree] Built with %zu objects (%zu eligible of %zu total GOs)",
              m_objectLocationMap.size(), eligible, objects.size());

    if (m_objectLocationMap.size() != eligible)
    {
        DEBUG_WARN("[Quadtree] %zu eligible objects were not inserted during build.",
                   eligible - m_objectLocationMap.size());
    }

    isBuilded = true;
    m_warnedNoRoot = false;
}

void Quadtree::update()
{
    resolveDirtyNodes();
}

void Quadtree::clear()
{
    m_dirtyNodes.clear();
    m_objectLocationMap.clear();
    m_root.reset();

    isBuilded = false;
    m_warnedNoRoot = false;
}

bool Quadtree::insert(GameObject& object)
{
    auto* model = object.GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    if (!model)
    {
        // Not an error: non-visual objects (spawners, triggers, lights...) are
        // simply not tracked by the quadtree.
        return false;
    }

    if (!model->hasMesh())
    {
        static std::unordered_set<GameObject*> s_noMeshWarned;
        if (s_noMeshWarned.insert(&object).second)
        {
            DEBUG_WARN("[Quadtree] '%s' cannot be tracked: its MeshRenderer has no mesh loaded (references not fixed?).",
                       object.GetName().c_str());
        }
        return false;
    }

    if (!m_root)
    {
        if (!m_warnedNoRoot)
        {
            DEBUG_WARN("[Quadtree] Insert of '%s' ignored: quadtree is not built yet.", object.GetName().c_str());
            m_warnedNoRoot = true;
        }
        return false;
    }

    const Engine::BoundingBox& box = model->getBoundingBox();

    if (!m_rebuilding && !m_root->getBounds().containsFullySafe(box))
    {
        // The object lies outside the area the tree was built for (e.g. a
        // prefab spawned at runtime). Grow the tree instead of silently
        // dropping the object: it would never be rendered or found by
        // quadtree-based gameplay queries.
        DEBUG_WARN("[Quadtree] Object '%s' lies outside the quadtree bounds, rebuilding.", object.GetName().c_str());
        rebuild();
        if (!m_root)
            return false;
    }

    // After a rebuild the object was already inserted by build(); avoid a
    // duplicate entry.
    if (m_objectLocationMap.count(&object) == 0)
    {
        m_root->insert(object);
    }

    return m_objectLocationMap.count(&object) > 0;
}

void Quadtree::rebuild()
{
    if (m_rebuilding)
        return;

    m_rebuilding = true;
    clear();
    build(m_layers);
    m_rebuilding = false;
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
    {
        if (!m_warnedNoRoot)
        {
            DEBUG_WARN("[Quadtree] query() called but the quadtree is not built; returning nothing.");
            m_warnedNoRoot = true;
        }
        return result;
    }

    m_root->gatherObjects(frustum, result);

    return result;
}

std::vector<GameObject*> Quadtree::queryInArea(const Vector2& center, const float radius) const
{
    std::vector<GameObject*> result;

    if (!m_root)
    {
        if (!m_warnedNoRoot)
        {
            DEBUG_WARN("[Quadtree] queryInArea() called but the quadtree is not built; returning nothing "
                       "(gameplay area queries will find no objects).");
            m_warnedNoRoot = true;
        }
        return result;
    }

    const BoundingRect area(center.x - radius, center.y - radius, radius * 2, radius * 2);

    m_root->gatherObjectsInArea(area, result);

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

void Quadtree::debugDraw()
{
    for (const BoundingRect& rect : getQuadrants())
    {
        Vector3 extents(rect.width * 0.5f, 0.0f, rect.height * 0.5f);
        Vector3 center(rect.x + rect.width * 0.5f, 0.1f, rect.y + rect.height * 0.5f);

        const ddVec3& color = rect.m_debugIsCulled ? m_debugBaseColor : m_debugCulledColor;
        dd::box(ddConvert(center), color, extents.x * 2.f, extents.y * 2.f, extents.z * 2.f);

        center.y = 0.f;
        extents.y = 10000.f;
        Vector3 min = center - extents;
        Vector3 max = center + extents;

        Vector3 pts[8] = {
            {center.x - extents.x, center.y - extents.y, center.z - extents.z},
            {center.x - extents.x, center.y - extents.y, center.z + extents.z},
            {center.x - extents.x, center.y + extents.y, center.z - extents.z},
            {center.x - extents.x, center.y + extents.y, center.z + extents.z},
            {center.x + extents.x, center.y - extents.y, center.z - extents.z},
            {center.x + extents.x, center.y - extents.y, center.z + extents.z},
            {center.x + extents.x, center.y + extents.y, center.z - extents.z},
            {center.x + extents.x, center.y + extents.y, center.z + extents.z},
        };
    }
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