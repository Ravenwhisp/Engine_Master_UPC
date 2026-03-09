#pragma once

class Settings;

class ViewGameDebug
{
private:
    Settings* m_settings;

public:
    ViewGameDebug();

    void render();
};