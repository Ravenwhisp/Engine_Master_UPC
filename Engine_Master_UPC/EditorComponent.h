#pragma once
#include "GameObject.h"

class GameObject;

template<class T> class EditorComponent
{
public:
	virtual const char* getName() const = 0;
	virtual void		render(){}

	//Not sure if this is the right way since we have to notify all the editor components that the GameObject changed
	void				setGameObject(GameObject* gameObject) { m_gameObject = gameObject; }
	T*					getComponent() { return m_gameObject->GetComponent<T>(); }

	bool	isOpen() const { return m_isOpen; }
	void	setOpen(bool open) { m_isOpen = open; }
	bool*	getOpenPtr() { return &m_isOpen; }

protected:
	//This should go in a utility class
	void VecToFloat(Vector3& vec, float arr[3]) {
		arr[0] = vec.x;
		arr[1] = vec.y;
		arr[2] = vec.z;
	}
private:
	bool m_isOpen = true;

	GameObject* m_gameObject;
};

