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

    // 入力履歴
    bool  m_prevMouseLeft = false;

    // デバッグチート関連
    bool  m_cheatEnabled = false;
    bool  m_prevKeyF1 = false;
    bool  m_prevKey1  = false;
    bool  m_prevKey2  = false;
    bool  m_prevKey3  = false;
    bool  m_prevKey4  = false;
    bool  m_prevKey5  = false;
    int   m_prevUpdateTime = 0;

    void SetupTitle();
    void SetupRoomObstacles();

public:
    GameManager() = default;
    ~GameManager() = default;

    void Init();
    void StartGame();
    void Update();
    void Draw();

    void SetFrameProcessTime(float ms) { m_frameProcessTimeMs = ms; }
};
