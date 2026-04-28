#pragma once
#include "SimpleMath.h"

struct SceneLightingSettings
{
	friend class cereal::access;

	DirectX::SimpleMath::Vector3 ambientColor;
	float ambientIntensity;

#pragma once Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(ambientColor, ambientIntensity);
    }
#pragma endregion
};