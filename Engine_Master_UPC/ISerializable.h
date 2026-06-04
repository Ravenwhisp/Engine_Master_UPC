#pragma once

class IArchive;

class ISerializable
{
public:
    virtual ~ISerializable() = default;
    virtual void serialize(IArchive& archive) = 0;
};
