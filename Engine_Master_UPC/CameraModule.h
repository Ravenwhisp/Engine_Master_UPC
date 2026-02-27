#pragma once
#include "Module.h"

class InputModule;
class D3D12Module;
class EditorModule;

class Settings;

class CameraModule : public Module {
private:
    InputModule* m_inputModule;
    D3D12Module* m_D3D12Module;
    EditorModule* m_editorModule;

    Settings* m_settings;

    // Camera state
    Vector3 m_position{ 0.0f, 10.0f, 10.0f };
    Vector3 m_target{ 0.0f, 0.0f, 0.0f };
    Vector3 m_up = Vector3::Up;

    Matrix m_view;
    Matrix m_projection;

    // FPS angles (persistentes)
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    // Orbit distance
    float m_distanceToTarget = 10.0f;

    // --------- Tunables (UI) ----------
    // General
    float m_fovY = XM_PIDIV4;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;

    // Mouse state per-mode
    bool m_panFirst = true;
    int  m_panLastX = 0, m_panLastY = 0;

    bool m_orbitFirst = true;
    int  m_orbitLastX = 0, m_orbitLastY = 0;

    bool m_zoomFirst = true;
    int  m_zoomLastX = 0, m_zoomLastY = 0;

    bool m_flyFirst = true;
    int  m_flyLastX = 0, m_flyLastY = 0;

    int m_lastWheel = 0;
    int m_lastNavMode = -999;

public:
    bool init() override;
    void update() override;

    // --- Getters ---
    const inline Matrix& getView() const { return m_view; }
    const inline Matrix& getProjection() const { return m_projection; }
    const inline Vector3& getPosition() const { return m_position; }
    const inline Quaternion getRotation() const { return Quaternion::CreateFromYawPitchRoll(-m_yaw, m_pitch, 0.0f); }

private:
    // Modes
    void panMode();
    void orbitMode();
    void zoomMode();
    void flythroughMode(float dt);

    // Helpers
    void handleMouseWheel();
    void handleAutoFocus();
    void rebuildViewProj();

    void syncYawPitchFromLookAt();
    void syncDistanceFromLookAt();
    Vector3 getForwardFromYawPitch() const;
    Vector3 getRightFromForward(const Vector3& fwd) const;
    void focusOnTarget();
};
