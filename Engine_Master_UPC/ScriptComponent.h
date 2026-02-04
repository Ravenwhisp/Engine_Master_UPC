#pragma once
#include "Component.h"

class ScriptComponent: public Component
{
public:
	virtual void Awake(){}
	virtual void Start(){}
	virtual void FixedUpdate(float deltaTime){}
	virtual void Update(float deltaTime){}
	virtual void LateUpdate(float deltaTime){}
	virtual void Reset(){}
};

