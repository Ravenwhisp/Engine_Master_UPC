#include "Globals.h"
#include "ExitApplication.h"

#include "Application.h"
#include "GameObject.h"
#include "UIButton.h"
#include "SceneReferenceResolver.h"

ExitApplication::ExitApplication(UID id, GameObject* gameObject)
    : Component(id, ComponentType::EXIT_APPLICATION, gameObject)
{
}

ExitApplication::~ExitApplication()
{
}

std::unique_ptr<Component> ExitApplication::clone(GameObject* newOwner) const
{
    std::unique_ptr<ExitApplication> newComponent = std::make_unique<ExitApplication>(m_uuid, newOwner);

    newComponent->setActive(this->isActive());

    return newComponent;
}

bool ExitApplication::init()
{
    return true;
}

void ExitApplication::drawUi()
{
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 180, 0, 255));
    ImGui::Text("[!!!] Deprecated Component");
    ImGui::PopStyleColor();
}

void ExitApplication::onExitApplication()
{
    app->requestApplicationExit();
}

rapidjson::Value ExitApplication::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::EXIT_APPLICATION), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());
    return componentInfo;
}

bool ExitApplication::deserializeJSON(const rapidjson::Value& componentInfo)
{
    return true;
}
