#include "Globals.h"
#include "DebugScene.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"
#include "LightDebugDraw.h"
#include "NavigationAgentComponent.h"
#include "RenderContext.h"

void DebugScene::draw(const RenderContext& ctx)
{
    for (GameObject* root : ctx.scene->getScene()->getRootObjects())
    {
        drawGameObject(root);
    }
}

void DebugScene::drawGameObject(const GameObject* go) const
{
    if (!go || !go->GetActive()) return;

    if (auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT))
    {
        if (light->isDebugDrawEnabled())
        {
            light->isDebugDrawDepthEnabled() ? LightDebugDraw::drawLightWithDepth(*go) : LightDebugDraw::drawLightWithoutDepth(*go);
        }
    }

    if (auto* agent = go->GetComponentAs<NavigationAgentComponent>(ComponentType::NAVIGATION_AGENT))
    {
        if (agent->isActive())
        {
            agent->drawDebugPath();
        }
    }

    for (GameObject* child : go->GetTransform()->getAllChildren())
    {
        drawGameObject(child);
    }

}