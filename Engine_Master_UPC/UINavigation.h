#pragma once

#include "UID.h"

class GameObject;

class UINavigation
{
public:
    void update();

    void setSelected(GameObject* go);
    GameObject* getSelected() const;
    void clearSelection();

private:
    void processNavigation();

    bool isSelectable(GameObject* go) const;
    GameObject* findFirstSelectableButton() const;

private:
    UID m_selectedUid = INVALID_UID;
};
