#pragma once
#include "SimpleMath.h"

struct SceneDataCB
{
    friend class cereal::access;

	DirectX::SimpleMath::Vector3 viewPos;
	float pad0 = 0.0f;

#pragma once Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(viewPos, pad0);
    }
#pragma endregion
};