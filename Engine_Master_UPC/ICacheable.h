#pragma once
#include "Globals.h"
#include "MD5.h"

class ICacheable
{
public:
    explicit ICacheable(MD5Hash uid) : m_uid(uid) {}
    virtual ~ICacheable() = default;
    MD5Hash getId() const { return m_uid; }
private:
    MD5Hash m_uid;
};