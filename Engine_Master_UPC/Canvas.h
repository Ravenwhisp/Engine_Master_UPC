#pragma once
#include "Component.h"
#include "CanvasRenderMode.h"

class Canvas : public Component
{
public:
    Canvas(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    CanvasRenderMode renderMode = CanvasRenderMode::SCREEN_SPACE;
    bool zTest = false;

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Component>(this), renderMode, zTest);
	}
};

CEREAL_REGISTER_TYPE(Canvas);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, Canvas)