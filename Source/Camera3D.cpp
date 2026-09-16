#include "Camera3D.h"
#include "Config.h"
#include "Common.h"
#include <cmath>
#include <algorithm>

Camera3D::Camera3D()
    : m_targetPos(VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f))
    , m_currentPos(VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE))
    , m_angleH(0.0f)
    , m_angleV(48.0f * MathHelper::DEG_TO_RAD)
    , m_distance(Config::CAMERA_DISTANCE)
    , m_height(Config::CAMERA_HEIGHT)
    , m_isFirstFrame(true) {
}

void Camera3D::Init(const VECTOR& initialTargetPos) {
    // 注視点はステージ中央
    m_targetPos = VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f);
    m_angleH = 0.0f;
    m_angleV = 48.0f * MathHelper::DEG_TO_RAD;
    m_distance = Config::CAMERA_DISTANCE;
    m_height = Config::CAMERA_HEIGHT;
    m_isFirstFrame = true;

    // ステージ手前斜め上から中央を見下ろす位置
    m_currentPos = VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE);
}

void Camera3D::Update(const VECTOR& targetPlayerPos, float playerFacingAngle, bool enableMouseLook) {
    // ステージ全体が画面に収まる俯瞰視点
    // 注視点はステージ中央（原点）
    m_targetPos = VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f);

    // カメラ位置：ステージ手前上空から見下ろし（壁コリジョンによるズームインなし）
    VECTOR desiredCamPos = VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE);
    m_currentPos.x = MathHelper::Lerp(m_currentPos.x, desiredCamPos.x, 0.2f);
    m_currentPos.y = MathHelper::Lerp(m_currentPos.y, desiredCamPos.y, 0.2f);
    m_currentPos.z = MathHelper::Lerp(m_currentPos.z, desiredCamPos.z, 0.2f);
}

void Camera3D::Apply() const {
    SetCameraNearFar(5.0f, 3500.0f);
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
