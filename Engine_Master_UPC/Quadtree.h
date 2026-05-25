#pragma once
#include <memory>
#include <vector>
#include <unordered_map>

#include "Frustum.h"
#include "QuadNode.h"
#include "IDebugDrawable.h"
#include "Layer.h"

class Scene;
class GameObject;

typedef float ddVec3[3];

class Quadtree: public IDebugDrawable
{
public:
    static const int MAX_OBJECTS = 1;
    static const int MAX_DEPTH = 5;

private:
    friend class QuadNode;

    Scene* m_scene = nullptr;
    bool isBuilded = false;

    std::unique_ptr<QuadNode> m_root;
    std::vector<QuadNode*> m_dirtyNodes;
    std::unordered_map<GameObject*, QuadNode*> m_objectLocationMap;

    ddVec3 m_debugBaseColor = { 1.0f, 0.0f, 0.0f };
    ddVec3 m_debugCulledColor = { 0.0f, 1.0f, 0.0f };

public:
    Quadtree();
    ~Quadtree();

    void init(Scene* scene, const ddVec3& baseColor, const ddVec3& culledColor);
    void update();
    void clear();

    void build(const std::vector<Layer> layers = {});

    std::vector<GameObject*> query() const;
	std::vector<GameObject*> queryInArea(const Vector2& center, const float radius) const;
    std::vector<BoundingRect> getQuadrants() const;

    void debugDraw() override;

    QuadNode& getRoot() { return *m_root; }

    void registerDirtyNode(QuadNode* node);
    void resolveDirtyNodes();

    bool getIsBuilded() { return isBuilded; }

    void move(GameObject& object);
    void remove(GameObject& object);

private:
    void insert(GameObject& object);
};