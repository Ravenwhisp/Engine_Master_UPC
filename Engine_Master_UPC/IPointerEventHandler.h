#pragma once

#include "PointerEventData.h"

class IPointerEventHandler
{
public:
    virtual void onPointerEnter(PointerEventData&) {}
    virtual void onPointerExit(PointerEventData&) {}
    virtual void onPointerDown(PointerEventData&) {}
    virtual void onPointerDrag(PointerEventData&) {}
    virtual void onPointerUp(PointerEventData&) {}
    virtual void onPointerClick(PointerEventData&) {}
    virtual ~IPointerEventHandler() = default;
};

