#include "Globals.h"
#include "ModuleHaptics.h"
#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

/*static*/ float ModuleHaptics::clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float ModuleHaptics::evaluateCurve(const ActiveEffect& ae) const
{
    const float duration = ae.effect.durationSeconds;
    if (duration <= 0.0f)
        return 0.0f;

    const float t = clamp01(ae.elapsed / duration);

    switch (ae.effect.curve)
    {
    case HapticCurve::Linear:
        return 1.0f - t;

    case HapticCurve::Exponential:
        return std::exp(-5.0f * t);

    case HapticCurve::Sustain:
        return 1.0f;

    case HapticCurve::Punch:
    {
        const float inv = 1.0f - t;
        return inv * inv * inv;
    }

    default:
        return 1.0f - t;
    }
}

ModuleHaptics::ModuleHaptics() = default;
ModuleHaptics::~ModuleHaptics() = default;

bool ModuleHaptics::init()
{
    for (PlayerState& ps : m_players)
        ps.activeEffects.reserve(16);

    return true;
}

void ModuleHaptics::update()
{
    const bool shouldRumble = (app->getCurrentEngineState() == ENGINE_STATE::PLAYING
        && !app->isPaused());

    const float dt = app->getModuleTime()->unscaledDeltaTime();

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        tickPlayer(i, dt);

        if (!shouldRumble)
        {
            PlayerState& ps = m_players[i];
            ps.leftMotor = ps.rightMotor = ps.leftTrigger = ps.rightTrigger = 0.0f;
        }

        applyToHardware(i);
    }
}

bool ModuleHaptics::cleanUp()
{
    silenceAll();
    for (PlayerState& ps : m_players)
        ps.activeEffects.clear();
    return true;
}

uint32_t ModuleHaptics::submitEffect(const HapticEffect& effect, int player)
{
    if (player < 0 || player >= MAX_PLAYERS)
    {
        DEBUG_WARN("[ModuleHaptics] submitEffect: player index %d out of range.", player);
        return 0;
    }

    constexpr int MAX_CRITICAL_EFFECTS = 4;
    constexpr int MAX_TOTAL_EFFECTS = 16;

    PlayerState& ps = m_players[player];

    if (static_cast<int>(ps.activeEffects.size()) >= MAX_TOTAL_EFFECTS)
    {
        auto lowestIt = std::min_element(
            ps.activeEffects.begin(), ps.activeEffects.end(),
            [](const ActiveEffect& a, const ActiveEffect& b)
            {
                return static_cast<uint8_t>(a.effect.priority)
                    < static_cast<uint8_t>(b.effect.priority);
            });

        if (lowestIt != ps.activeEffects.end()
            && static_cast<uint8_t>(lowestIt->effect.priority)
            < static_cast<uint8_t>(effect.priority))
        {
            lowestIt->alive = false;
        }
        else
        {
            DEBUG_WARN("[ModuleHaptics] submitEffect: effect pool full for player %d, "
                "effect dropped (priority too low).", player);
            return 0;
        }
    }

    const uint32_t handle = m_nextHandle++;
    if (m_nextHandle == 0) m_nextHandle = 1; 

    ActiveEffect ae;
    ae.effect = effect;
    ae.elapsed = 0.0f;
    ae.delay = effect.delaySeconds;
    ae.handle = handle;
    ae.alive = true;

    ps.activeEffects.push_back(ae);
    return handle;
}

void ModuleHaptics::cancelEffect(uint32_t handle, int player)
{
    if (handle == 0 || player < 0 || player >= MAX_PLAYERS)
        return;

    PlayerState& ps = m_players[player];
    for (ActiveEffect& ae : ps.activeEffects)
    {
        if (ae.handle == handle)
        {
            ae.alive = false;
            return;
        }
    }
}

void ModuleHaptics::cancelAll(int player)
{
    if (player < 0 || player >= MAX_PLAYERS)
        return;

    PlayerState& ps = m_players[player];
    for (ActiveEffect& ae : ps.activeEffects)
        ae.alive = false;
}

void ModuleHaptics::silenceAll()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        cancelAll(i);

        PlayerState& ps = m_players[i];
        ps.leftMotor = ps.rightMotor = ps.leftTrigger = ps.rightTrigger = 0.0f;
        applyToHardware(i);
    }
}

bool ModuleHaptics::isPlaying(int player) const
{
    if (player < 0 || player >= MAX_PLAYERS)
        return false;

    const PlayerState& ps = m_players[player];
    for (const ActiveEffect& ae : ps.activeEffects)
    {
        if (ae.alive)
            return true;
    }
    return false;
}

void ModuleHaptics::tickPlayer(int playerIndex, float dt)
{
    PlayerState& ps = m_players[playerIndex];

    for (ActiveEffect& ae : ps.activeEffects)
    {
        if (!ae.alive)
            continue;

        if (ae.delay > 0.0f)
        {
            ae.delay -= dt;
            continue; 
        }

        ae.elapsed += dt;

        if (ae.elapsed >= ae.effect.durationSeconds)
            ae.alive = false;
    }

    for (int i = static_cast<int>(ps.activeEffects.size()) - 1; i >= 0; --i)
    {
        if (!ps.activeEffects[i].alive)
        {
            ps.activeEffects[i] = ps.activeEffects.back();
            ps.activeEffects.pop_back();
        }
    }

    float leftMotor = 0.0f;
    float rightMotor = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;

    for (const ActiveEffect& ae : ps.activeEffects)
    {
        if (!ae.alive || ae.delay > 0.0f)
            continue;

        const float envelope = evaluateCurve(ae);

        leftMotor += ae.effect.leftMotor * envelope;
        rightMotor += ae.effect.rightMotor * envelope;
        leftTrigger += ae.effect.leftTrigger * envelope;
        rightTrigger += ae.effect.rightTrigger * envelope;
    }

    ps.leftMotor = clamp01(leftMotor);
    ps.rightMotor = clamp01(rightMotor);
    ps.leftTrigger = clamp01(leftTrigger);
    ps.rightTrigger = clamp01(rightTrigger);
}

void ModuleHaptics::applyToHardware(int playerIndex)
{
    // Get the SDL gamepad from ModuleInput instead of XInput
    ModuleInput* input = app->getModuleInput();
    if (!input)
        return;

    // SDL_Gamepad* is stored in ModuleInput — we need to access it
    // Get the gamepad for this player index
    SDL_Gamepad* gamepad = input->getSDLGamepad(playerIndex);
    if (!gamepad)
        return;

    const PlayerState& ps = m_players[playerIndex];

    // SDL_RumbleGamepad takes uint16 values (0-65535)
    const uint16_t left = static_cast<uint16_t>(ps.leftMotor * 65535.0f);
    const uint16_t right = static_cast<uint16_t>(ps.rightMotor * 65535.0f);

    // Duration 0 = use our own timing system, just keep sending each frame
    SDL_RumbleGamepad(gamepad, left, right, 32);  // 32ms — refreshed every frame

    // Trigger rumble (SDL3 supports this for DualSense natively)
    const uint16_t leftTrigger = static_cast<uint16_t>(ps.leftTrigger * 65535.0f);
    const uint16_t rightTrigger = static_cast<uint16_t>(ps.rightTrigger * 65535.0f);
    SDL_RumbleGamepadTriggers(gamepad, leftTrigger, rightTrigger, 32);
}