// ============================================================================
// @file   Camera3D.h
// @brief  プレイヤー追従型3Dサードパーソンカメラ（TPS視点・壁めり込み防止・シェイク）
// ============================================================================
#pragma once
#include "DxLib.h"

// ============================================================================
// Camera3D クラス
// マウス移動および矢印キーによる自由な360度軌道回転、筋トレQTE中の正面アングルへの
// 自動回り込み、ステージ壁面とのレイキャスト遮蔽回避、捕獲時のカメラシェイクを管理する
// ============================================================================
class Camera3D
{
private:
    VECTOR m_cameraPos;              // 現在のカメラ視点ワールド座標
    VECTOR m_targetPos;              // 現在のカメラ注視点ワールド座標（プレイヤー胸部付近）
    float  m_angleH         = 0.0f;  // 水平回転角（ヨー角、ラジアン）
    float  m_angleV         = 0.35f; // 垂直仰角（ピッチ角、ラジアン）
    float  m_distance       = 190.0f;// 注視点からカメラまでの基準距離
    float  m_heightOffset   = 28.0f; // プレイヤー足元から注視点までの高さオフセット

    // 筋トレQTE時に猫の背後（0.0f）から正面（1.0f）へ滑らかに回り込むブレンド率
    float  m_frontViewBlend = 0.0f;

    // カメラシェイク（画面揺れ演出）用パラメータ
    float  m_shakeIntensity = 0.0f;  // シェイクの振幅（ワールド単位）
    int    m_shakeTimer     = 0;     // シェイク残りフレーム数

    // マウス視点操作の感度倍率（0.2x 〜 3.0x、デフォルト 1.0x）
    float  m_mouseSensitivity = 1.0f;

    // 視点リセットキー（Fキー）の前フレーム押下状態（エッジ検出用）
    bool   m_prevResetKey   = false;

public:
    Camera3D();
    ~Camera3D() = default;

    // プレイヤー初期位置と向きに合わせてカメラを初期化する
    void Init(const VECTOR& targetPos, float targetRotY);

    // ------------------------------------------------------------------------
    // 毎フレームのカメラ更新処理
    // @param targetPos       追従対象（プレイヤー）の現在座標
    // @param targetRotY      追従対象のY軸回転角（正面回り込み・Fキーリセット用）
    // @param enableMouseLook マウス移動による視点回転を有効にするか
    // @param isFrontView     trueのとき猫の正面アングルへ滑らかに回り込む（筋トレQTE中）
    // @param stageModelHandle 壁遮蔽レイキャスト用のステージモデルハンドル
    // @param minBounds       ステージ境界ボックス最小座標
    // @param maxBounds       ステージ境界ボックス最大座標
    // ------------------------------------------------------------------------
    void Update(
        const VECTOR& targetPos,
        float targetRotY,
        bool enableMouseLook = true,
        bool isFrontView = false,
        int stageModelHandle = -1,
        const VECTOR* minBounds = nullptr,
        const VECTOR* maxBounds = nullptr
    );

    // 計算されたカメラ座標・注視点（＋シェイクオフセット）をDxLibの3Dカメラへ反映する
    void Apply() const;

    // カメラシェイクを発動する（既に強いシェイクが再生中なら大きい方を優先）
    void TriggerShake(float intensity, int frames);

    // ------------------------------------------------------------------------
    // カメラの向きに基づく水平面（XZ平面）の単位方向ベクトル取得（WASD移動基準用）
    // ------------------------------------------------------------------------
    VECTOR GetForwardXZ() const; // カメラ前方の水平単位ベクトル
    VECTOR GetRightXZ() const;   // カメラ右方の水平単位ベクトル

    float GetAngleH() const { return m_angleH; }

    // リザルト開始時のズームイン補間元として現在のカメラ座標・注視点を取得する
    VECTOR GetCameraPos() const { return m_cameraPos; }
    VECTOR GetTargetPos() const { return m_targetPos; }

    // ------------------------------------------------------------------------
    // マウス感度設定・取得（ポーズ画面のスライダーやプレイ中の [- / +] キーと連動）
    // ------------------------------------------------------------------------
    float GetMouseSensitivity() const { return m_mouseSensitivity; }
    void  SetMouseSensitivity(float sens);
    void  AdjustMouseSensitivity(float delta);

    // ポーズ解除時などにマウスカーソルを画面中央へ戻し、視点の急激な跳ねを防ぐ
    void  ResetMouseToCenter() const;
};
