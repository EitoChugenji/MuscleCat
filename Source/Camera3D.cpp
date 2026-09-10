#include "Camera3D.h"
#include "Config.h"
#include "Common.h"
#include <cmath>

Camera3D::Camera3D()
    : m_targetPos(VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f))
    , m_currentPos(VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE))
    , m_angleH(0.0f)
    , m_angleV(22.0f * MathHelper::DEG_TO_RAD)
    , m_distance(Config::CAMERA_DISTANCE)
    , m_height(Config::CAMERA_HEIGHT) {
}

void Camera3D::Init(const VECTOR& initialTargetPos) {
    m_targetPos = VGet(initialTargetPos.x, initialTargetPos.y + Config::CAMERA_TARGET_OFFSET_Y, initialTargetPos.z);
    m_angleH = 0.0f;
    m_angleV = 22.0f * MathHelper::DEG_TO_RAD;
    m_distance = Config::CAMERA_DISTANCE;
    m_height = Config::CAMERA_HEIGHT;

    float desiredX = m_targetPos.x - std::sin(m_angleH) * m_distance * std::cos(m_angleV);
    float desiredY = m_targetPos.y + std::sin(m_angleV) * m_distance;
    float desiredZ = m_targetPos.z - std::cos(m_angleH) * m_distance * std::cos(m_angleV);
    m_currentPos = VGet(desiredX, desiredY, desiredZ);

    GetMousePoint(&m_prevMouseX, &m_prevMouseY);
}

void Camera3D::Update(const VECTOR& targetPlayerPos, float playerFacingAngle) {
    // ------------------------------------------------------------------------
    // キー入力またはマウス操作によるカメラ回転
    // ------------------------------------------------------------------------
    // Q / E キー または 矢印キーでの水平旋回
    float rotSpeed = 0.04f;
    if (CheckHitKey(KEY_INPUT_Q) || CheckHitKey(KEY_INPUT_LEFT)) {
        m_angleH -= rotSpeed;
    }
    if (CheckHitKey(KEY_INPUT_E) || CheckHitKey(KEY_INPUT_RIGHT)) {
        m_angleH += rotSpeed;
    }
    if (CheckHitKey(KEY_INPUT_UP)) {
        m_angleV += rotSpeed * 0.7f;
    }
    if (CheckHitKey(KEY_INPUT_DOWN)) {
        m_angleV -= rotSpeed * 0.7f;
    }

    // マウス右ボタンドラッグでのカメラ操作
    int mouseX, mouseY;
    GetMousePoint(&mouseX, &mouseY);
    if ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0) {
        int dx = mouseX - m_prevMouseX;
        int dy = mouseY - m_prevMouseY;
        m_angleH += static_cast<float>(dx) * 0.008f;
        m_angleV -= static_cast<float>(dy) * 0.008f;
    }
    m_prevMouseX = mouseX;
    m_prevMouseY = mouseY;

    // 垂直角度の制限（地面へのめり込み・真上真下の反転防止）
    m_angleV = MathHelper::Clamp(m_angleV, 5.0f * MathHelper::DEG_TO_RAD, 65.0f * MathHelper::DEG_TO_RAD);

    // ------------------------------------------------------------------------
    // 目標注視点とカメラ位置の滑らかな追従補間
    // ------------------------------------------------------------------------
    VECTOR desiredTarget = VGet(targetPlayerPos.x, targetPlayerPos.y + Config::CAMERA_TARGET_OFFSET_Y, targetPlayerPos.z);
    m_targetPos.x = MathHelper::Lerp(m_targetPos.x, desiredTarget.x, 0.15f);
    m_targetPos.y = MathHelper::Lerp(m_targetPos.y, desiredTarget.y, 0.15f);
    m_targetPos.z = MathHelper::Lerp(m_targetPos.z, desiredTarget.z, 0.15f);

    float desiredCamX = m_targetPos.x - std::sin(m_angleH) * (m_distance * std::cos(m_angleV));
    float desiredCamY = m_targetPos.y + (m_distance * std::sin(m_angleV));
    float desiredCamZ = m_targetPos.z - std::cos(m_angleH) * (m_distance * std::cos(m_angleV));

    m_currentPos.x = MathHelper::Lerp(m_currentPos.x, desiredCamX, 0.18f);
    m_currentPos.y = MathHelper::Lerp(m_currentPos.y, desiredCamY, 0.18f);
    m_currentPos.z = MathHelper::Lerp(m_currentPos.z, desiredCamZ, 0.18f);
}

void Camera3D::Apply() const {
    SetCameraNearFar(5.0f, 2000.0f);
    SetCameraPositionAndTarget_UpVecY(m_currentPos, m_targetPos);
}

VECTOR Camera3D::GetForwardXZ() const {
    float sinH = std::sin(m_angleH);
    float cosH = std::cos(m_angleH);
    return VGet(sinH, 0.0f, cosH);
}

VECTOR Camera3D::GetRightXZ() const {
    float sinH = std::sin(m_angleH);
    float cosH = std::cos(m_angleH);
    return VGet(cosH, 0.0f, -sinH);
}
