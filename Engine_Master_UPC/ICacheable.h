#pragma once
#include "Globals.h"
#include "UID.h"

class ICacheable
{
public:
    explicit ICacheable(UID uid) : m_uid(uid) {}
    virtual ~ICacheable() = default;
    UID getId() const { return m_uid; }
private:
    UID m_uid;
};