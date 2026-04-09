#include "Globals.h"
#include "HierarchyUtils.h"


#include "Application.h"
#include "ModuleEditor.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include <PrefabEditSession.h>

namespace HierarchyUtils
{
    GameObject* findByUID(Scene* scene, UID id)
    {
        if (!scene || id == 0) return nullptr;
        for (GameObject* go : scene->getAllGameObjects())
        {
            if (go && go->GetID() == id)
            {
                return go;
            }
        }

        return nullptr;
    }

    void reparent(Scene* scene, GameObject* child, GameObject* newParent)
    {
        if (!child) return;

        Transform* childTransform = child->GetTransform();
        Transform* newParentTransform = newParent ? newParent->GetTransform() : nullptr;

        // Guard: reparenting onto a descendant would create a cycle
        if (newParentTransform && newParentTransform->isDescendantOf(childTransform))
        {
            return;
        }

        const Matrix worldMatrix = childTransform->getGlobalMatrix();

        // Detach from old parent
        Transform* oldRoot = childTransform->getRoot();
        GameObject* oldParent = oldRoot ? oldRoot->getOwner() : nullptr;

        if (oldParent)
        {
            oldParent->GetTransform()->removeChild(child->GetID());

        }
        else
        {
            scene->removeFromRootList(child);
        }

        // Attach to new parent
        childTransform->setRoot(newParentTransform);

        if (newParent)
        {
            newParentTransform->addChild(child);

        }
        else
        {
            scene->addToRootList(child);
        }

        // Preserve world-space position/rotation/scale
        childTransform->setFromGlobalMatrix(worldMatrix);
    }

    Scene* resolveTargetScene()
    {
        PrefabEditSession* session = app->getModuleEditor()->getPrefabSession();
        if (session && session->m_active && session->m_isolatedScene)
        {
            return session->m_isolatedScene;
        }

        return app->getModuleScene()->getScene();
    }
}
