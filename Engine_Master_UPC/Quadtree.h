#pragma once
#include <memory>
#include <vector>
#include <unordered_map>

#include "Frustum.h"
#include "QuadNode.h"
#include "IDebugDrawable.h"

class Scene;
class GameObject;

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

public:
    Quadtree();
    ~Quadtree();

    void init(Scene* scene);
    void update();
    void clear();

    void build();

    std::vector<GameObject*> query() const;
    std::vector<BoundingRect> getQuadrants() const;

    void debugDraw() override;

    QuadNode& getRoot() { return *m_root; }

    void registerDirtyNode(QuadNode* node);
    void resolveDirtyNodes();

    bool getIsBuilded() { return isBuilded; }
private:
    void insert(GameObject& object);
    void remove(GameObject& object);
    void move(GameObject& object);
};