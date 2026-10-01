#include "Camera3D.h"
#include "Config.h"
#include "Common.h"
#include <cmath>
#include <algorithm>

Camera3D::Camera3D()
    : m_targetPos(VGet(0.0f, Config::CAMERA_TARGET_OFFSET_Y, 0.0f))
    , m_currentPos(VGet(0.0f, Config::CAMERA_HEIGHT, -Config::CAMERA_DISTANCE))
    , m_angleH(0.0f)
    , m_angleV(38.0f * MathHelper::DEG_TO_RAD)
    , m_distance(Config::CAMERA_DISTANCE)
    , m_height(Config::CAMERA_HEIGHT)
    , m_prevMouseX(0)
    , m_prevMouseY(0)
    , m_isFirstFrame(true)
    , m_prevPlayerPos(VGet(0.0f, 0.0f, 0.0f))
    , m_manualControlTimer(0)
{
}

void Camera3D::Init(const VECTOR& initialTargetPos, float initialAngleH)
{
    m_distance = Config::CAMERA_DISTANCE;
    m_height   = Config::CAMERA_HEIGHT;
    m_angleH   = initialAngleH;
    m_angleV   = 38.0f * MathHelper::DEG_TO_RAD;
    m_isFirstFrame = true;
    m_prevPlayerPos = initialTargetPos;
    m_manualControlTimer = 0;

    m_targetPos = VGet(initialTargetPos.x, initialTargetPos.y + Config::CAMERA_TARGET_OFFSET_Y, initialTargetPos.z);

    float desiredCamX = m_targetPos.x - std::sin(m_angleH) * (m_distance * std::cos(m_angleV));
    float desiredCamY = m_targetPos.y + (m_distance * std::sin(m_angleV));
    float desiredCamZ = m_targetPos.z - std::cos(m_angleH) * (m_distance * std::cos(m_angleV));

    m_currentPos = VGet(desiredCamX, desiredCamY, desiredCamZ);

    GetMousePoint(&m_prevMouseX, &m_prevMouseY);
}

void Camera3D::Update(
    const VECTOR& targetPlayerPos,
    float playerFacingAngle,
    bool enableMouseLook,
    bool isFrontView,
    int stageModelHandle,
    const VECTOR* minBounds,
    const VECTOR* maxBounds
)
{
    // ------------------------------------------------------------------------
    // 1. キーボードによるカメラ操作（矢印キー、Q/Rキー、Fキーで視点リセット）
    // ------------------------------------------------------------------------
    const float rotSpeedH = 0.045f; // 水平旋回スピード（キビキビ動いて酔いにくい）
    const float rotSpeedV = 0.030f; // 垂直仰角スピード

    // [←] または [Q] : カメラ左旋回
    if (CheckHitKey(KEY_INPUT_LEFT) || CheckHitKey(KEY_INPUT_Q))
    {
        m_angleH -= rotSpeedH;
    }

    // [→] または [R] : カメラ右旋回
    if (CheckHitKey(KEY_INPUT_RIGHT) || CheckHitKey(KEY_INPUT_R))
    {
        m_angleH += rotSpeedH;
    }

    // [↑] : 見下ろし角度を上げる（上空からの俯瞰へ）
    if (CheckHitKey(KEY_INPUT_UP))
    {
        m_angleV += rotSpeedV;
    }

    // [↓] : 見下ろし角度を下げる（水平近くへ）
    if (CheckHitKey(KEY_INPUT_DOWN))
    {
        m_angleV -= rotSpeedV;
    }

    // [F] キー : 視点リセット（猫の背後へスッと戻す）
    if (CheckHitKey(KEY_INPUT_F))
    {
        m_angleH = MathHelper::LerpAngle(m_angleH, playerFacingAngle, 0.25f);
    }

    // マウス右ボタンドラッグ操作（マウス使用時の互換性）
    int mouseX = 0, mouseY = 0;
    GetMousePoint(&mouseX, &mouseY);

    if (enableMouseLook && ((GetMouseInput() & MOUSE_INPUT_RIGHT) != 0))
    {
        if (!m_isFirstFrame)
        {
            int dx = mouseX - m_prevMouseX;
            int dy = mouseY - m_prevMouseY;

            if (dx != 0 || dy != 0)
            {
                m_angleH += static_cast<float>(dx) * 0.006f;
                m_angleV += static_cast<float>(dy) * 0.006f;
            }
        }
    }

    m_prevMouseX = mouseX;
    m_prevMouseY = mouseY;

    // マウスホイールでのカメラ距離（ズーム）調整
    int wheelRot = GetMouseWheelRotVol();
    if (wheelRot != 0)
    {
        m_distance -= static_cast<float>(wheelRot) * 15.0f;
        m_distance = MathHelper::Clamp(m_distance, 110.0f, 340.0f);
    }

    // ------------------------------------------------------------------------
    // 2. 視点角度の制限（移動時の遅延自動旋回は酔い防止のため完全停止）
    // ------------------------------------------------------------------------
    // 垂直角度の制限（地面へのめり込み防止〜急角度見下ろしまで）
    m_angleV = MathHelper::Clamp(m_angleV, 12.0f * MathHelper::DEG_TO_RAD, 65.0f * MathHelper::DEG_TO_RAD);

    if (m_isFirstFrame)
    {
        m_angleH = playerFacingAngle;
    }

    float currentTargetDist = m_distance;

    if (isFrontView)
    {
        // 筋トレ（SPACE）中: 視点を急旋回させず安定を保ちつつ、少し寄ってスクワットを映す
        float targetAngleV = 24.0f * MathHelper::DEG_TO_RAD;
        m_angleV = MathHelper::Lerp(m_angleV, targetAngleV, 0.08f);
        currentTargetDist = MathHelper::Lerp(currentTargetDist, 160.0f, 0.08f);
    }

    // ------------------------------------------------------------------------
    // 3. 注視点の滑らかな追従（猫の体・頭付近）
    // ------------------------------------------------------------------------
    float targetOffsetY = isFrontView ? (Config::CAMERA_TARGET_OFFSET_Y + 2.0f) : Config::CAMERA_TARGET_OFFSET_Y;
    VECTOR desiredTarget = VGet(targetPlayerPos.x, targetPlayerPos.y + targetOffsetY, targetPlayerPos.z);
    if (m_isFirstFrame)
    {
        m_targetPos = desiredTarget;
    }
    else
    {
        // キビキビと追従させて遅延揺れ・フワフワ酔いを防止
        m_targetPos.x = MathHelper::Lerp(m_targetPos.x, desiredTarget.x, 0.35f);
        m_targetPos.y = MathHelper::Lerp(m_targetPos.y, desiredTarget.y, 0.35f);
        m_targetPos.z = MathHelper::Lerp(m_targetPos.z, desiredTarget.z, 0.35f);
    }

    // ------------------------------------------------------------------------
    // 4. カメラ位置の計算と外壁めり込み防止（スマートズーム）
    // ------------------------------------------------------------------------
    float camDirX = -std::sin(m_angleH) * std::cos(m_angleV);
    float camDirY =  std::sin(m_angleV);
    float camDirZ = -std::cos(m_angleH) * std::cos(m_angleV);

    float currentDist = currentTargetDist;

    // 4-A. ステージモデルのメッシュ壁レイキャスト判定（カメラと猫の間の遮蔽壁を検知）
    if (stageModelHandle != -1)
    {
        VECTOR startRay = m_targetPos;
        VECTOR endRay = VGet(
            m_targetPos.x + camDirX * (m_distance + 8.0f),
            m_targetPos.y + camDirY * (m_distance + 8.0f),
            m_targetPos.z + camDirZ * (m_distance + 8.0f)
        );

        MV1_COLL_RESULT_POLY hitLine = MV1CollCheck_Line(stageModelHandle, -1, startRay, endRay);
        if (hitLine.HitFlag == 1 && std::abs(hitLine.Normal.y) < 0.85f)
        {
            float hdx = hitLine.HitPosition.x - m_targetPos.x;
            float hdy = hitLine.HitPosition.y - m_targetPos.y;
            float hdz = hitLine.HitPosition.z - m_targetPos.z;
            float distToHit = std::sqrt(hdx * hdx + hdy * hdy + hdz * hdz);

            float safeDist = distToHit - 8.0f;
            if (safeDist < currentDist)
            {
                currentDist = safeDist;
            }
        }
    }

    // 4-B. 外壁境界ボックス判定（セーフティネット）
    float wallMargin = 15.0f;
    float minX = minBounds ? (minBounds->x + wallMargin) : (-Config::STAGE_HALF_WIDTH + wallMargin);
    float maxX = maxBounds ? (maxBounds->x - wallMargin) : ( Config::STAGE_HALF_WIDTH - wallMargin);
    float minZ = minBounds ? (minBounds->z + wallMargin) : (-Config::STAGE_HALF_DEPTH + wallMargin);
    float maxZ = maxBounds ? (maxBounds->z - wallMargin) : ( Config::STAGE_HALF_DEPTH - wallMargin);

    if (std::abs(camDirX) > 0.0001f)
    {
        if (camDirX > 0.0f && m_targetPos.x < maxX)
        {
            float distToWall = (maxX - m_targetPos.x) / camDirX;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
        else if (camDirX < 0.0f && m_targetPos.x > minX)
        {
            float distToWall = (minX - m_targetPos.x) / camDirX;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
    }

    if (std::abs(camDirZ) > 0.0001f)
    {
        if (camDirZ > 0.0f && m_targetPos.z < maxZ)
        {
            float distToWall = (maxZ - m_targetPos.z) / camDirZ;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
        else if (camDirZ < 0.0f && m_targetPos.z > minZ)
        {
            float distToWall = (minZ - m_targetPos.z) / camDirZ;
            if (distToWall > 0.0f && distToWall < currentDist) currentDist = distToWall;
        }
    }

    // 最短距離リミット（近すぎ防止）
    if (currentDist < 30.0f) currentDist = 30.0f;

    float desiredCamX = m_targetPos.x + camDirX * currentDist;
    float desiredCamY = m_targetPos.y + camDirY * currentDist;
    float desiredCamZ = m_targetPos.z + camDirZ * currentDist;

    // 壁境界内クランプ & 地面潜り込み防止
    desiredCamX = MathHelper::Clamp(desiredCamX, minX, maxX);
    desiredCamZ = MathHelper::Clamp(desiredCamZ, minZ, maxZ);
    if (desiredCamY < 14.0f) desiredCamY = 14.0f;

    if (m_isFirstFrame)
    {
        m_currentPos = VGet(desiredCamX, desiredCamY, desiredCamZ);
        m_isFirstFrame = false;
    }
    else
    {
        // カメラ追従の遅延を抑えて安定した視界を維持
        m_currentPos.x = MathHelper::Lerp(m_currentPos.x, desiredCamX, 0.35f);
        m_currentPos.y = MathHelper::Lerp(m_currentPos.y, desiredCamY, 0.35f);
        m_currentPos.z = MathHelper::Lerp(m_currentPos.z, desiredCamZ, 0.35f);
    }
}

void Camera3D::Apply() const
{
    SetCameraNearFar(5.0f, 3500.0f);
    SetCameraPositionAndTarget_UpVecY(m_currentPos, m_targetPos);
}

VECTOR Camera3D::GetForwardXZ() const
{
    float sinH = std::sin(m_angleH);
    float cosH = std::cos(m_angleH);
    return VGet(sinH, 0.0f, cosH);
}

VECTOR Camera3D::GetRightXZ() const
{
    float sinH = std::sin(m_angleH);
    float cosH = std::cos(m_angleH);
    return VGet(cosH, 0.0f, -sinH);
}