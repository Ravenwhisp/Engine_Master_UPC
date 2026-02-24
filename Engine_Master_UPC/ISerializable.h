#pragma once
#include "ISerializer.h"
#include "IDeserializer.h"

class ISerializable {
public:
    virtual ~ISerializable() = default;

    virtual void serialize(ISerializer& serializer) const = 0;

    virtual void deserialize(IDeserializer& deserializer) = 0;

};