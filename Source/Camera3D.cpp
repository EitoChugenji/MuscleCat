#include "Camera3D.h"
#include "Config.h"
#include "Common.h"
#include <cmath>
#include <algorithm>

Camera3D::Camera3D()
    : m_targetPos(VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f))
    , m_currentPos(VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE))
    , m_angleH(0.0f)
    , m_angleV(22.0f * MathHelper::DEG_TO_RAD)
    , m_distance(Config::CAMERA_DISTANCE)
    , m_height(Config::CAMERA_HEIGHT)
    , m_isFirstFrame(true) {
}

void Camera3D::Init(const VECTOR& initialTargetPos) {
    m_targetPos = VGet(initialTargetPos.x, initialTargetPos.y + Config::CAMERA_TARGET_OFFSET_Y, initialTargetPos.z);
    m_angleH = 0.0f;
    m_angleV = 24.0f * MathHelper::DEG_TO_RAD;
    m_distance = Config::CAMERA_DISTANCE;
    m_height = Config::CAMERA_HEIGHT;
    m_isFirstFrame = true;

    float desiredX = m_targetPos.x - std::sin(m_angleH) * m_distance * std::cos(m_angleV);
    float desiredY = m_targetPos.y + std::sin(m_angleV) * m_distance;
    float desiredZ = m_targetPos.z - std::cos(m_angleH) * m_distance * std::cos(m_angleV);
    m_currentPos = VGet(desiredX, desiredY, desiredZ);

    SetMousePoint(Config::SCREEN_WIDTH / 2, Config::SCREEN_HEIGHT / 2);
}

void Camera3D::Update(const VECTOR& targetPlayerPos, float playerFacingAngle, bool enableMouseLook) {
    // ------------------------------------------------------------------------
    // マウス操作による直感的な3D視点移動
    // ------------------------------------------------------------------------
    if (enableMouseLook) {
        int centerX = Config::SCREEN_WIDTH / 2;
        int centerY = Config::SCREEN_HEIGHT / 2;
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);

        if (!m_isFirstFrame) {
            int dx = mouseX - centerX;
            int dy = mouseY - centerY;
            float sensitivity = 0.0032f;

            m_angleH += static_cast<float>(dx) * sensitivity;
            m_angleV += static_cast<float>(dy) * sensitivity; // 上下反転
        } else {
            m_isFirstFrame = false;
        }

        // マウスを画面中央に常時リセット（画面外への飛び出し防止）
        SetMousePoint(centerX, centerY);
    } else {
        m_isFirstFrame = true;
    }

    // ------------------------------------------------------------------------
    // キーボード操作（Q/E または 矢印キーでの補助旋回）
    // ------------------------------------------------------------------------
    float keyRotSpeed = 0.04f;
    if (CheckHitKey(KEY_INPUT_Q) || CheckHitKey(KEY_INPUT_LEFT)) {
        m_angleH -= keyRotSpeed;
    }
    if (CheckHitKey(KEY_INPUT_E) || CheckHitKey(KEY_INPUT_RIGHT)) {
        m_angleH += keyRotSpeed;
    }
    if (CheckHitKey(KEY_INPUT_UP)) {
        m_angleV += keyRotSpeed * 0.7f;
    }
    if (CheckHitKey(KEY_INPUT_DOWN)) {
        m_angleV -= keyRotSpeed * 0.7f;
    }

    // 垂直角度の制限（地面へのめり込み・真上真下の反転防止）
    m_angleV = MathHelper::Clamp(m_angleV, 5.0f * MathHelper::DEG_TO_RAD, 68.0f * MathHelper::DEG_TO_RAD);

    // ------------------------------------------------------------------------
    // 目標注視点とカメラ位置の滑らかな追従補間
    // ------------------------------------------------------------------------
    VECTOR desiredTarget = VGet(targetPlayerPos.x, targetPlayerPos.y + Config::CAMERA_TARGET_OFFSET_Y, targetPlayerPos.z);
    m_targetPos.x = MathHelper::Lerp(m_targetPos.x, desiredTarget.x, 0.22f);
    m_targetPos.y = MathHelper::Lerp(m_targetPos.y, desiredTarget.y, 0.22f);
    m_targetPos.z = MathHelper::Lerp(m_targetPos.z, desiredTarget.z, 0.22f);

    // ------------------------------------------------------------------------
    // ★ 壁際での視界遮蔽防止（スマートレイキャスト＆自動ズームイン）
    // ------------------------------------------------------------------------
    float camDirX = -std::sin(m_angleH) * std::cos(m_angleV);
    float camDirY =  std::sin(m_angleV);
    float camDirZ = -std::cos(m_angleH) * std::cos(m_angleV);

    float currentDist = m_distance;

    // 外壁との安全マージン
    float wallMargin = 20.0f;
    float minX = -Config::STAGE_HALF_WIDTH + wallMargin;
    float maxX =  Config::STAGE_HALF_WIDTH - wallMargin;
    float minZ = -Config::STAGE_HALF_DEPTH + wallMargin;
    float maxZ =  Config::STAGE_HALF_DEPTH - wallMargin;

    // X軸方向の壁との交差判定（壁に近づいたらカメラをズームイン）
    if (std::abs(camDirX) > 0.0001f) {
        if (camDirX > 0.0f && m_targetPos.x < maxX) {
            float distToWall = (maxX - m_targetPos.x) / camDirX;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        } else if (camDirX < 0.0f && m_targetPos.x > minX) {
            float distToWall = (minX - m_targetPos.x) / camDirX;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
    }

    // Z軸方向の壁との交差判定
    if (std::abs(camDirZ) > 0.0001f) {
        if (camDirZ > 0.0f && m_targetPos.z < maxZ) {
            float distToWall = (maxZ - m_targetPos.z) / camDirZ;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        } else if (camDirZ < 0.0f && m_targetPos.z > minZ) {
            float distToWall = (minZ - m_targetPos.z) / camDirZ;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
    }

    // 最短距離リミット（近すぎ防止）
    if (currentDist < 35.0f) currentDist = 35.0f;

    float desiredCamX = m_targetPos.x + camDirX * currentDist;
    float desiredCamY = m_targetPos.y + camDirY * currentDist;
    float desiredCamZ = m_targetPos.z + camDirZ * currentDist;

    // 壁境界内クランプ
    desiredCamX = MathHelper::Clamp(desiredCamX, minX, maxX);
    desiredCamZ = MathHelper::Clamp(desiredCamZ, minZ, maxZ);
    if (desiredCamY < 14.0f) desiredCamY = 14.0f;

    // カメラ位置のスムーズ補間
    m_currentPos.x = MathHelper::Lerp(m_currentPos.x, desiredCamX, 0.28f);
    m_currentPos.y = MathHelper::Lerp(m_currentPos.y, desiredCamY, 0.28f);
    m_currentPos.z = MathHelper::Lerp(m_currentPos.z, desiredCamZ, 0.28f);
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
