#pragma once

class GameObject;

class UINavigation
{
public:
    void update();

    void setSelected(GameObject* go);
    GameObject* getSelected()  const { return m_selected; }
    void clearSelection();

private:
    void processNavigation();

    bool isSelectable(GameObject* go) const;
    GameObject* findFirstSelectableButton() const;

private:
    GameObject* m_selected = nullptr;
};