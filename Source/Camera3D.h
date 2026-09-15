#pragma once
#include "DxLib.h"

// ============================================================================
// Camera3D クラス
// 三人称視点 (TPS) カメラ制御
// ============================================================================
class Camera3D {
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

public:
    Camera3D();
    ~Camera3D() = default;

    void Init(const VECTOR& initialTargetPos);
    void Update(const VECTOR& targetPlayerPos, float playerFacingAngle, bool enableMouseLook = true);
    void Apply() const;

    float GetAngleH() const { return m_angleH; }
    VECTOR GetCameraPos() const { return m_currentPos; }
    VECTOR GetForwardXZ() const;
    VECTOR GetRightXZ() const;
};
