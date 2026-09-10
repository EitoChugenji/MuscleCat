#pragma once
#include "ObjectManager3D.h"
#include "Camera3D.h"

// ============================================================================
// ゲーム状態定義
// ============================================================================
enum class GameState {
    Title,
    Playing,
    GameClear
};

// ============================================================================
// GameManager クラス
// ============================================================================
class GameManager {
private:
    ObjectManager3D m_objManager;
    Camera3D        m_camera;
    GameState       m_state = GameState::Title;

    int   m_startCount = 0;
    int   m_clearCount = 0;
    float m_clearTimeSeconds = 0.0f;
    float m_titleCameraAngle = 0.0f;

    // パフォーマンス計測用
    int   m_frameCount = 0;
    int   m_fpsTimer = 0;
    float m_currentFps = 60.0f;
    float m_frameProcessTimeMs = 0.0f;

public:
    GameManager() = default;
    ~GameManager() = default;

    void Init();
    void StartGame();
    void Update();
    void Draw();

    void SetFrameProcessTime(float ms) { m_frameProcessTimeMs = ms; }
};
