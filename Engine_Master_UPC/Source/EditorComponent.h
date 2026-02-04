#pragma once
#include "GameObject.h"

class GameObject;

template<class T> class EditorComponent
{
public:
	virtual const char* GetName() const = 0;
	virtual void Render(){}

	//Not sure if this is the right way since we have to notify all the editor components that the GameObject changed
	void SetGameObject(GameObject* gameObject) { this->gameObject = gameObject; }
	T* GetComponent() { return gameObject->GetComponent<T>(); }

	bool IsOpen() const { return m_IsOpen; }
	void SetOpen(bool open) { m_IsOpen = open; }
	bool* GetOpenPtr() { return &m_IsOpen; }

protected:
	//This should go in a utility class
	void VecToFloat(Vector3& vec, float arr[3]) {
		arr[0] = vec.x;
		arr[1] = vec.y;
		arr[2] = vec.z;
	}
private:
	bool m_IsOpen = true;

	GameObject* gameObject;
};

