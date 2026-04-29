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

float ModuleHaptics::evaluateEnvelope(const HapticInstance& inst) const
{
    const HapticEffectDefinition& d = *inst.def;

    if (d.durationSeconds <= 0.0f)
        return 0.0f;

    const float elapsed = inst.elapsed;
    const float dur = d.durationSeconds;

    const float t = clamp01(elapsed / dur);

    float attackScale = 1.0f;
    if (d.attackSeconds > 0.0f && elapsed < d.attackSeconds)
        attackScale = elapsed / d.attackSeconds;

    float releaseScale = 1.0f;
    if (d.releaseSeconds > 0.0f)
    {
        const float releaseStart = dur - d.releaseSeconds;
        if (elapsed >= releaseStart)
            releaseScale = 1.0f - (elapsed - releaseStart) / d.releaseSeconds;
    }

    float curveScale = 1.0f;

    if (d.curve == HapticCurve::Custom && d.customCurve)
    {
        curveScale = clamp01(d.customCurve(t));
    }
    else
    {
        switch (d.curve)
        {
        case HapticCurve::Linear:
            curveScale = 1.0f - t;
            break;

        case HapticCurve::Exponential:
            // Fast initial drop,long tail. exp(-5) ? 0.007 at t=1.
            curveScale = std::exp(-5.0f * t);
            break;

        case HapticCurve::Sustain:
            // Perfectly flat duration controls the length.
            curveScale = 1.0f;
            break;

        case HapticCurve::Punch:
            // Cubic inverse instant peak, sharp fall.
        {
            const float inv = 1.0f - t;
            curveScale = inv * inv * inv;
        }
        break;

        default:
            curveScale = 1.0f - t;
            break;
        }
    }

    return clamp01(attackScale * releaseScale * curveScale);
}


ModuleHaptics::ModuleHaptics() = default;
ModuleHaptics::~ModuleHaptics() = default;

bool ModuleHaptics::init()
{
    HapticEffectLibrary::get().registerBuiltins();
    HapticEffectLibrary::get().loadFromJSON("Assets/Haptics/game_effects.json");

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_players[i].instances.reserve(MAX_INSTANCES);
    }

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
        ps.instances.clear();
    return true;
}

uint32_t ModuleHaptics::playEffect(const std::string& effectId, int player)
{
    const HapticEffectDefinition* def =
        HapticEffectLibrary::get().findEffect(effectId);

    if (!def)
    {
        DEBUG_WARN("[ModuleHaptics] playEffect: unknown effect id '%s'.", effectId.c_str());
        return 0;
    }

    return submitInternal(def, 1.0f, player);
}

uint32_t ModuleHaptics::playAtScale(const std::string& effectId, float scale, int player)
{
    const HapticEffectDefinition* def =
        HapticEffectLibrary::get().findEffect(effectId);

    if (!def)
    {
        DEBUG_WARN("[ModuleHaptics] playAtScale: unknown effect id '%s'.", effectId.c_str());
        return 0;
    }

    return submitInternal(def, clamp01(scale), player);
}

uint32_t ModuleHaptics::submitAnonymous(const HapticEffectDefinition& def, float scale, int player)
{
    if (player < 0 || player >= MAX_PLAYERS)
    {
        DEBUG_WARN("[ModuleHaptics] submitAnonymous: player %d out of range.", player);
        return 0;
    }

    PlayerState& ps = m_players[player];

    if (static_cast<int>(ps.instances.size()) >= MAX_INSTANCES)
    {
        auto lowest = std::min_element(
            ps.instances.begin(), ps.instances.end(),
            [](const HapticInstance& a, const HapticInstance& b)
            {
                return static_cast<uint8_t>(a.def->priority)
                    < static_cast<uint8_t>(b.def->priority);
            });

        if (lowest != ps.instances.end()
            && static_cast<uint8_t>(lowest->def->priority)
            < static_cast<uint8_t>(def.priority))
        {
            lowest->alive = false;
        }
        else
        {
            DEBUG_WARN("[ModuleHaptics] submitAnonymous: pool full, effect dropped.");
            return 0;
        }
    }

    const uint32_t handle = m_nextHandle++;
    if (m_nextHandle == 0) m_nextHandle = 1;

    HapticInstance inst;
    inst.anonDef = def;      
    inst.def = &inst.anonDef;
    inst.elapsed = 0.0f;
    inst.delay = def.delaySeconds;
    inst.scale = clamp01(scale);
    inst.handle = handle;
    inst.alive = true;

    ps.instances.push_back(std::move(inst));
    return handle;
}

uint32_t ModuleHaptics::submitInternal(const HapticEffectDefinition* def,
    float scale,
    int   player,
    const HapticEffectDefinition* /*unused*/)
{
    if (!def)
        return 0;

    if (player < 0 || player >= MAX_PLAYERS)
    {
        DEBUG_WARN("[ModuleHaptics] submitInternal: player %d out of range.", player);
        return 0;
    }

    PlayerState& ps = m_players[player];

    if (static_cast<int>(ps.instances.size()) >= MAX_INSTANCES)
    {
        auto lowest = std::min_element(
            ps.instances.begin(), ps.instances.end(),
            [](const HapticInstance& a, const HapticInstance& b)
            {
                return static_cast<uint8_t>(a.def->priority)
                    < static_cast<uint8_t>(b.def->priority);
            });

        if (lowest != ps.instances.end()
            && static_cast<uint8_t>(lowest->def->priority)
            < static_cast<uint8_t>(def->priority))
        {
            lowest->alive = false;
        }
        else
        {
            DEBUG_WARN("[ModuleHaptics] Effect pool full for player %d; '%s' dropped "
                "(priority too low).", player, def->id.c_str());
            return 0;
        }
    }

    const uint32_t handle = m_nextHandle++;
    if (m_nextHandle == 0) m_nextHandle = 1;

    HapticInstance inst;
    inst.def = def;
    inst.elapsed = 0.0f;
    inst.delay = def->delaySeconds;
    inst.scale = scale;
    inst.handle = handle;
    inst.alive = true;

    ps.instances.push_back(inst);
    return handle;
}

uint32_t ModuleHaptics::submitEffect(const HapticEffectDefinition& def, int player)
{
    return submitAnonymous(def, 1.0f, player);
}

void ModuleHaptics::cancelEffect(uint32_t handle, int player)
{
    if (handle == 0 || player < 0 || player >= MAX_PLAYERS)
        return;

    for (HapticInstance& inst : m_players[player].instances)
    {
        if (inst.handle == handle)
        {
            inst.alive = false;
            return;
        }
    }
}

void ModuleHaptics::cancelAll(int player)
{
    if (player < 0 || player >= MAX_PLAYERS)
        return;

    for (HapticInstance& inst : m_players[player].instances)
        inst.alive = false;
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

    for (const HapticInstance& inst : m_players[player].instances)
    {
        if (inst.alive)
            return true;
    }
    return false;
}

void ModuleHaptics::getMixedOutput(int player,
    float& outLeft, float& outRight,
    float& outLeftTrigger, float& outRightTrigger) const
{
    if (player < 0 || player >= MAX_PLAYERS)
    {
        outLeft = outRight = outLeftTrigger = outRightTrigger = 0.0f;
        return;
    }

    const PlayerState& ps = m_players[player];
    outLeft = ps.leftMotor;
    outRight = ps.rightMotor;
    outLeftTrigger = ps.leftTrigger;
    outRightTrigger = ps.rightTrigger;
}

void ModuleHaptics::tickPlayer(int playerIndex, float dt)
{
    PlayerState& ps = m_players[playerIndex];

    for (HapticInstance& inst : ps.instances)
    {
        if (!inst.alive)
            continue;

        if (inst.delay > 0.0f)
        {
            inst.delay -= dt;
            continue;
        }

        inst.elapsed += dt;

        if (inst.elapsed >= inst.def->durationSeconds)
            inst.alive = false;
    }

    for (int i = static_cast<int>(ps.instances.size()) - 1; i >= 0; --i)
    {
        if (!ps.instances[i].alive)
        {
            ps.instances[i] = std::move(ps.instances.back());
            ps.instances.pop_back();
        }
    }

    float leftMotor = 0.0f;
    float rightMotor = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;

    for (const HapticInstance& inst : ps.instances)
    {
        if (!inst.alive || inst.delay > 0.0f)
            continue;

        const float envelope = evaluateEnvelope(inst);
        const float combined = envelope * inst.scale;

        leftMotor += inst.def->peak.leftMotor * combined;
        rightMotor += inst.def->peak.rightMotor * combined;
        leftTrigger += inst.def->peak.leftTrigger * combined;
        rightTrigger += inst.def->peak.rightTrigger * combined;
    }

    ps.leftMotor = clamp01(leftMotor);
    ps.rightMotor = clamp01(rightMotor);
    ps.leftTrigger = clamp01(leftTrigger);
    ps.rightTrigger = clamp01(rightTrigger);
}

void ModuleHaptics::applyToHardware(int playerIndex) const
{
    ModuleInput* input = app->getModuleInput();
    if (!input)
        return;

    SDL_Gamepad* gamepad = input->getSDLGamepad(playerIndex);
    if (!gamepad)
        return;

    const PlayerState& ps = m_players[playerIndex];

    const auto toU16 = [](float v) -> uint16_t
        {
            return static_cast<uint16_t>(v * 65535.0f);
        };

    SDL_RumbleGamepad(gamepad, toU16(ps.leftMotor), toU16(ps.rightMotor), 32);
    SDL_RumbleGamepadTriggers(gamepad, toU16(ps.leftTrigger), toU16(ps.rightTrigger), 32);
}

void ModuleHaptics::logActiveEffects(int player) const
{
#ifndef GAME_RELEASE
    if (player < 0 || player >= MAX_PLAYERS)
        return;

    const PlayerState& ps = m_players[player];

    DEBUG_LOG("[ModuleHaptics] Player %d — %zu active instance(s):",
        player, ps.instances.size());

    for (const HapticInstance& inst : ps.instances)
    {
        if (!inst.alive) continue;
        DEBUG_LOG("  handle=%u  id='%s'  elapsed=%.3fs / %.3fs  scale=%.2f  priority=%d",
            inst.handle,
            inst.def->id.c_str(),
            inst.elapsed,
            inst.def->durationSeconds,
            inst.scale,
            static_cast<int>(inst.def->priority));
    }

    DEBUG_LOG("  mixed: L=%.2f  R=%.2f  LT=%.2f  RT=%.2f",
        ps.leftMotor, ps.rightMotor, ps.leftTrigger, ps.rightTrigger);
#endif
}

void ModuleHaptics::logConnectedControllers() const
{
#ifndef GAME_RELEASE
    ModuleInput* input = app->getModuleInput();
    if (!input) return;

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        const bool connected = input->isGamePadConnected(i);
        DEBUG_LOG("[ModuleHaptics] Player %d controller: %s",
            i, connected ? "CONNECTED" : "not connected");
    }
#endif
}