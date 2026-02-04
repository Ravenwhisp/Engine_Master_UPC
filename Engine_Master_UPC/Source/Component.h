#pragma once

class Component {
public:
    virtual ~Component() = default;

    virtual void Start() {}
    virtual void Update(float deltaTime) {}
    virtual void OnDestroy() {}

protected:
    bool enabled = true;
};

