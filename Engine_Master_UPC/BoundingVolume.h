#pragma once
namespace Engine
{
    struct Frustum;
}

class BoundingVolume
{
public:
    virtual ~BoundingVolume() = default;

    virtual bool test(const Engine::Frustum& frustum) const = 0;
    virtual void render() = 0;
};