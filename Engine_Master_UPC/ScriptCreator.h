#pragma once

#include <memory>

class GameObject;
class Script;

using ScriptCreator = std::unique_ptr<Script>(*)(GameObject* owner);