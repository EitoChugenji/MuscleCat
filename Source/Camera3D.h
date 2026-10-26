#pragma once
#include "DxLib.h"

// Camera3D クラス
// 三人称視点 (TPS) カメラ制御
class Camera3D
{
private:
    VECTOR m_targetPos;     // 注視点（補間後）
    VECTOR m_currentPos;    // カメラ位置（補間後）
    float  m_angleH;        // 水平アングル (ラジアン)
    float  m_angleV;        // 垂直アングル (ラジアン)
    float  m_distance;      // ターゲットからの距離
    float  m_height;        // ターゲットからの高さ

    int    m_prevMouseX = 0;
    int    m_prevMouseY = 0;
    bool   m_isFirstFrame = true;
    bool   m_resetMouseNextFrame = true;
    VECTOR m_prevPlayerPos = VGet(0.0f, 0.0f, 0.0f);
    int    m_manualControlTimer = 0;
    float  m_mouseSensitivityScale = 1.0f; // マウス視点感度倍率 (0.2x ~ 3.0x, デフォルト 1.0x)

    // カメラシェイク（画面揺れ演出）
    float  m_shakeIntensity = 0.0f;
    int    m_shakeFrames = 0;
    int    m_shakeMaxFrames = 0;
    VECTOR m_shakeOffset = VGet(0.0f, 0.0f, 0.0f);

public:
    Camera3D();
    ~Camera3D() = default;

    void Init(const VECTOR& initialTargetPos, float initialAngleH = 0.0f);
    void ResetMouseToCenter();
    void Update(
        const VECTOR& targetPlayerPos,
        float playerFacingAngle,
        bool enableMouseLook = true,
        bool isFrontView = false,
        int stageModelHandle = -1,
        const VECTOR* minBounds = nullptr,
        const VECTOR* maxBounds = nullptr
    );
    void Apply() const;

    // カメラシェイク発動
    void TriggerShake(float intensity, int frames);

    float GetAngleH() const { return m_angleH; }
    VECTOR GetCameraPos() const { return m_currentPos; }
    VECTOR GetTargetPos() const { return m_targetPos; }
    VECTOR GetForwardXZ() const;
    VECTOR GetRightXZ() const;

    // マウス視点感度設定
    float GetMouseSensitivity() const { return m_mouseSensitivityScale; }
    void  SetMouseSensitivity(float scale);
    void  AdjustMouseSensitivity(float delta);
};