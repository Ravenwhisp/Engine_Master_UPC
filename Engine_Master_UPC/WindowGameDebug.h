#pragma once

class Settings;

class WindowGameDebug
{
private:
    Settings* m_settings;

public:
    WindowGameDebug();

    void render();
};