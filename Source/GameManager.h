#pragma once
#include "ObjectManager3D.h"
#include "Camera3D.h"

// ゲーム状態定義
enum class GameState
{
    Title,
    Playing,
    GameClear
};

// GameManager クラス
class GameManager
{
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

    // デバッグチート・F1詳細情報＆10m目測キューブ関連
    bool  m_cheatEnabled = false;
    int   m_debugCubeMode = 0; // 0: 猫追従10m■+全体グリッド, 1: 猫前方10m■+全体グリッド, 2: 原点10m■+全体グリッド, 3: 3D枠OFF
    bool  m_debugWideCamera = true; // F1時の10m全体目測用・広角俯瞰カメラ切替（初期ON、[8]で通常視点と切替）
    bool  m_prevKeyF1 = false;
    bool  m_prevKey1  = false;
    bool  m_prevKey2  = false;
    bool  m_prevKey3  = false;
    bool  m_prevKey4  = false;
    bool  m_prevKey5  = false;
    bool  m_prevKey6  = false;
    bool  m_prevKey7  = false;
    bool  m_prevKey8  = false;
    int   m_prevUpdateTime = 0;

    // タイトル画面用演出・メニュー状態
    ModelConfig::MapType m_selectedMap = ModelConfig::MapType::SlopeHills;
    bool  m_showHowToPlay = false;
    int   m_startTransitionTimer = 0;
    int   m_titleAnimTimer = 0;
    int   m_selectedMenuItem = 0; // 0: START, 1: STAGE, 2: HOW TO PLAY
    bool  m_prevKeyH = false;
    bool  m_prevKeyTab = false;
    bool  m_prevKeyEsc = false;
    bool  m_prevKeyUp = false;
    bool  m_prevKeyDown = false;
    bool  m_prevKeyLeft = false;
    bool  m_prevKeyRight = false;
    bool  m_prevKeyEnter = false;
    bool  m_prevKeySpace = false;
    bool  m_prevKeyR = false;
    bool  m_prevKeyT = false;

    // ポーズ状態
    bool  m_isPaused = false;
    int   m_pauseStartTime = 0;
    int   m_pauseMenuItem = 0; // 0: リトライ, 1: タイトルに戻る, 2: マウス感度
    bool  m_shouldQuit = false;

    // マウス感度調整
    int   m_sensitivityNoticeTimer = 0; // プレイ中の感度変更通知タイマー
    bool  m_prevKeyMinus = false;
    bool  m_prevKeyPlus = false;

    void SetupRoomObstacles();
    void DrawTitleScreen();
    void DrawHowToPlayModal();
    void UpdatePause(bool triggerEsc);
    void DrawPauseModal();
    void DrawDebug3DOverlay();
    void DrawDebug2DOverlay();

public:
    GameManager() = default;
    ~GameManager() = default;

    void Init();
    void SetupTitle();
    void StartGame();
    void SwitchSelectedMap(ModelConfig::MapType newMap);
    void Update();
    void Draw();

    void SetCheatEnabled(bool enabled)
    {
        m_cheatEnabled = enabled;
    }

    bool IsCheatEnabled() const
    {
        return m_cheatEnabled;
    }

    void SetDebugWideCamera(bool wide)
    {
        m_debugWideCamera = wide;
    }

    bool IsDebugWideCamera() const
    {
        return m_debugWideCamera;
    }

    int GetDebugCubeMode() const
    {
        return m_debugCubeMode;
    }

    bool ShouldQuit() const
    {
        return m_shouldQuit;
    }

    void SetShowHowToPlay(bool show)
    {
        m_showHowToPlay = show;
    }

    void SetFrameProcessTime(float ms)
    {
        m_frameProcessTimeMs = ms;
    }

    ObjectManager3D& GetObjManager()
    {
        return m_objManager;
    }

    Camera3D& GetCamera()
    {
        return m_camera;
    }

    void SetPaused(bool paused, int menuItem = 2)
    {
        m_isPaused = paused;
        m_pauseMenuItem = menuItem;
        if (paused)
        {
            m_pauseStartTime = GetNowCount();
        }
    }
};