// ============================================================================
// @file   GameManager.h
// @brief  ゲーム全体のシーン遷移（タイトル・プレイ中・リザルト・ポーズ）およびUI管理
// ============================================================================
#pragma once
#include "ObjectManager3D.h"
#include "Camera3D.h"

// ============================================================================
// ゲームシーン状態列挙型
// ============================================================================
enum class GameState
{
    Title,      // タイトル画面（3Dトレーニング演出・ステージ選択・ゲーム説明）
    Playing,    // ゲームプレイ画面（ネズミ捕獲アクション・120秒制限時間バトル）
    GameClear   // リザルト画面（猫へのズームイン演出・S/A/B/Cランクスタンプ・報酬スコア）
};

// ============================================================================
// GameManager クラス
// シーン遷移制御、ステージ初期化、3Dカメラ演出、各種2D UI（タイトル・プレイHUD・
// リザルト・遊び方モーダル・ポーズ画面・F1デバッグオーバーレイ）の更新と描画を統括する
// ============================================================================
class GameManager
{
private:
    // ------------------------------------------------------------------------
    // コアサブシステム・シーン状態
    // ------------------------------------------------------------------------
    ObjectManager3D m_objManager;                  // 3Dオブジェクト・ステージ・衝突の一括管理
    Camera3D        m_camera;                      // プレイヤー追従TPSカメラ
    GameState       m_state = GameState::Title;    // 現在のゲームシーン状態

    // ------------------------------------------------------------------------
    // プレイ時間・制限時間・リザルト演出用メンバ
    // ------------------------------------------------------------------------
    int    m_startCount           = 0;                       // ゲーム開始時のミリ秒カウンタ (GetNowCount)
    int    m_clearCount           = 0;                       // クリア/タイムアップ成立時のミリ秒カウンタ
    float  m_clearTimeSeconds     = 0.0f;                    // クリアまでにかかった経過秒数
    float  m_remainingTimeSeconds = 120.0f;                  // クリア時点での残り時間（秒）
    bool   m_isTimeUp             = false;                   // 時間切れ（TIME UP）で終了したかどうか
    int    m_clearAnimTimer       = 0;                       // リザルト開始からの経過フレーム数
    int    m_clearSelectedBtn     = 0;                       // リザルト画面の選択ボタン (0: RETRY, 1: MAIN MENU)
    bool   m_prevKeyM             = false;                   // [M]キーの前フレーム押下状態
    VECTOR m_clearCamStartPos     = VGet(0.0f, 0.0f, 0.0f);  // クリア瞬間のカメラ座標（ズーム補間開始点）
    VECTOR m_clearCamStartTarget  = VGet(0.0f, 0.0f, 0.0f);  // クリア瞬間のカメラ注視点（ズーム補間開始点）
    static constexpr int CLEAR_ZOOM_DURATION = 75;           // クリア時に筋トレする猫へズームインする演出フレーム数（約1.25秒）

    // ------------------------------------------------------------------------
    // パフォーマンス計測（FPS・フレーム処理時間）用メンバ
    // ------------------------------------------------------------------------
    int   m_frameCount         = 0;     // 0.5秒間のフレーム数積算カウンタ
    int   m_fpsTimer           = 0;     // 前回FPSを算出した時刻（ミリ秒）
    float m_currentFps         = 60.0f; // 現在の実測FPS
    float m_frameProcessTimeMs = 0.0f;  // 1フレームのCPU処理時間（ミリ秒）

    // マウス左クリックの前フレーム押下状態（エッジ検出用）
    bool  m_prevMouseLeft      = false;

    // ------------------------------------------------------------------------
    // F1 デバッグチート・10m目測キューブ・2Dミニマップ関連メンバ
    // ------------------------------------------------------------------------
    bool  m_cheatEnabled       = false; // F1デバッグモード有効フラグ
    int   m_debugCubeMode      = 0;     // [7]キー切替: 0=猫中心10m■, 1=猫前方10m■, 2=原点10m■, 3=OFF
    bool  m_debugWideCamera    = true;  // [8]キー切替: F1時の10m全体目測用・広角俯瞰カメラ有効フラグ
    bool  m_prevKeyF1          = false;
    bool  m_prevKey1           = false;
    bool  m_prevKey2           = false;
    bool  m_prevKey3           = false;
    bool  m_prevKey4           = false;
    bool  m_prevKey5           = false;
    bool  m_prevKey6           = false;
    bool  m_prevKey7           = false;
    bool  m_prevKey8           = false;
    int   m_prevUpdateTime     = 0;

    // ------------------------------------------------------------------------
    // タイトル画面・メニュー操作・トランジション演出用メンバ
    // ------------------------------------------------------------------------
    ModelConfig::MapType m_selectedMap          = ModelConfig::MapType::SlopeHills; // 現在選択中のステージ
    float                m_titleCameraAngle     = 0.0f;  // タイトル画面の3D旋回カメラ角度（ラジアン）
    bool                 m_showHowToPlay        = false; // ゲーム説明モーダル表示中フラグ
    int                  m_startTransitionTimer = 0;     // ゲーム開始時のホワイトアウト演出タイマー
    int                  m_titleAnimTimer       = 0;     // タイトル画面のアニメーション経過フレーム
    int                  m_selectedMenuItem     = 0;     // 選択中のメニュー項目 (0: START, 1: STAGE, 2: ゲーム説明)
    bool                 m_prevKeyH             = false;
    bool                 m_prevKeyTab           = false;
    bool                 m_prevKeyEsc           = false;
    bool                 m_prevKeyUp            = false;
    bool                 m_prevKeyDown          = false;
    bool                 m_prevKeyLeft          = false;
    bool                 m_prevKeyRight         = false;
    bool                 m_prevKeyEnter         = false;
    bool                 m_prevKeySpace         = false;
    bool                 m_prevKeyR             = false;
    bool                 m_prevKeyT             = false;

    // ------------------------------------------------------------------------
    // ポーズ（一時停止）画面・終了管理メンバ
    // ------------------------------------------------------------------------
    bool  m_isPaused               = false; // ポーズ中フラグ
    int   m_pauseStartTime         = 0;     // ポーズ開始時刻（ポーズ中の経過時間を除外するため記録）
    int   m_pauseMenuItem          = 0;     // ポーズメニュー選択項目 (0: リトライ, 1: タイトル, 2: マウス感度)
    bool  m_shouldQuit             = false; // アプリケーション終了要求フラグ

    // ------------------------------------------------------------------------
    // マウス感度クイック変更通知用メンバ
    // ------------------------------------------------------------------------
    int   m_sensitivityNoticeTimer = 0;     // プレイ中の感度変更ポップアップ残りフレーム
    bool  m_prevKeyMinus           = false;
    bool  m_prevKeyPlus            = false;

    // ------------------------------------------------------------------------
    // 内部ヘルパー・画面別描画メソッド
    // ------------------------------------------------------------------------
    void SetupRoomObstacles();           // ステージ内の家具・障害物配置
    void DrawTitleScreen();              // タイトル画面2D UI（ロゴ「マッスルねこ」・吹き出し・メニュー）描画
    void DrawTitle3DScene();             // タイトル画面3D演出（ステージリング・ライト柱・金粉）描画
    void DrawGameClearScreen();          // リザルト画面（アメコミ風ランクスタンプ・スコア・報酬）描画
    void DrawHowToPlayModal();           // ゲーム説明モーダルダイアログ描画
    void UpdatePause(bool triggerEsc);   // ポーズ画面中の入力・感度調整更新
    void DrawPauseModal();               // ポーズモーダルダイアログ描画
    void DrawDebug3DOverlay();           // F1デバッグ時の3Dオーバーレイ（10m目測■・1.8m物差し・壁可視化）描画
    void DrawDebug2DOverlay();           // F1デバッグ時の2Dパネル＆ミニマップレーダー描画

public:
    GameManager() = default;
    ~GameManager() = default;

    // 初期化・シーン遷移・毎フレーム更新・描画
    void Init();
    void SetupTitle();
    void StartGame();
    void SwitchSelectedMap(ModelConfig::MapType newMap);
    void Update();
    void Draw();

    // ------------------------------------------------------------------------
    // 各種ゲッター＆セッター
    // ------------------------------------------------------------------------
    void SetCheatEnabled(bool enabled)   { m_cheatEnabled = enabled; }
    bool IsCheatEnabled() const          { return m_cheatEnabled; }
    void SetDebugWideCamera(bool wide)   { m_debugWideCamera = wide; }
    bool IsDebugWideCamera() const       { return m_debugWideCamera; }
    int  GetDebugCubeMode() const        { return m_debugCubeMode; }
    bool ShouldQuit() const              { return m_shouldQuit; }
    void SetShowHowToPlay(bool show)     { m_showHowToPlay = show; }
    void SetFrameProcessTime(float ms)   { m_frameProcessTimeMs = ms; }
    void SetFrameProcessTimeMs(float ms) { m_frameProcessTimeMs = ms; }

    ObjectManager3D& GetObjManager() { return m_objManager; }
    Camera3D&        GetCamera()     { return m_camera; }

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

