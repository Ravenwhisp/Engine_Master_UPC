#include "pch.h"
#include "TestSceneChangeButton.h"

static const ScriptMethodInfo testSceneChangeButtonMethods[] =
{
    {
        "ChangeScene",
        nullptr,
        ScriptMethodParamType::String,
        "sceneName",
        [](Script* s, const void* param)
        {
            static_cast<TestSceneChangeButton*>(s)->ChangeScene(
                *static_cast<const std::string*>(param));
        }
    }
};

TestSceneChangeButton::TestSceneChangeButton(GameObject* owner)
    : Script(owner)
{
}

ScriptMethodList TestSceneChangeButton::getExposedMethods() const
{
    return
    {
        testSceneChangeButtonMethods,
        sizeof(testSceneChangeButtonMethods) / sizeof(ScriptMethodInfo)
    };
}

void TestSceneChangeButton::ChangeScene(const std::string& sceneName)
{
    Debug::log("TestSceneChangeButton: requesting scene change to %s", sceneName.c_str());
    SceneAPI::requestSceneChange(sceneName.c_str());
}

IMPLEMENT_SCRIPT(TestSceneChangeButton)