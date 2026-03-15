#include "Globals.h"
#include "EngineAPI.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include "Keyboard.h"
#include "ScriptFactory.h"

void registerScript(const char* scriptName, ScriptCreator creator)
{
    ScriptFactory::registerScript(scriptName, creator);
}

namespace Input 
{
    bool isKeyDown(int key)
    {
        if (!app)
        {
            return false;
        }

        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        return input->isKeyDown(static_cast<Keyboard::Keys>(key));
    }
}

namespace Time
{
    float getDeltaTime()
    {
        if (!app)
        {
            return 0.0f;
        }

        if (!app->getModuleTime())
        {
            return 0.0f;
        }

        return app->getModuleTime()->deltaTime();
    }
}