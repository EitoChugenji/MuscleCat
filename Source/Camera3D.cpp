// ============================================================================
// @file   Camera3D.cpp
// @brief  Camera3D クラスの実装（マウス視点操作、正面回り込み、壁遮蔽回避）
// ============================================================================
#include "Camera3D.h"
#include "Config.h"
#include "Common.h"
#include <cmath>

// ============================================================================
// コンストラクタ：初期カメラ位置・注視点の設定
// ============================================================================
Camera3D::Camera3D()
    : m_cameraPos(VGet(0.0f, 120.0f, -200.0f))
    , m_targetPos(VGet(0.0f, 20.0f, 0.0f))
    , m_angleH(0.0f)
    , m_angleV(0.35f)
{
}

// ============================================================================
// カメラの初期化処理
// プレイヤーの初期位置と背後の向きに合わせてカメラ座標を配置し、
// マウスカーソルを画面中央にセットする
// ============================================================================
void Camera3D::Init(const VECTOR& targetPos, float targetRotY)
{
    m_angleH = targetRotY;
    m_angleV = 0.28f; // 少し見下ろす快適なTPS視点角度
    m_frontViewBlend = 0.0f;
    m_targetPos = VGet(targetPos.x, targetPos.y + m_heightOffset, targetPos.z);

    // 初期の背後カメラ座標を算出
    float horizontalDist = m_distance * std::cos(m_angleV);
    float verticalDist   = m_distance * std::sin(m_angleV);
    m_cameraPos.x = m_targetPos.x - std::sin(m_angleH) * horizontalDist;
    m_cameraPos.z = m_targetPos.z - std::cos(m_angleH) * horizontalDist;
    m_cameraPos.y = m_targetPos.y + verticalDist;

    // クリップ距離（Near=5.0f, Far=90000.0f）を設定し、大型ステージの遠景切れを防ぐ
    SetCameraNearFar(5.0f, 90000.0f);

    // マウスカーソルを画面中心に初期化
    SetMousePoint(Config::SCREEN_WIDTH / 2, Config::SCREEN_HEIGHT / 2);
}

// ============================================================================
// マウス感度の設定（0.2倍 〜 3.0倍 の範囲にクランプ）
// ============================================================================
void Camera3D::SetMouseSensitivity(float sens)
{
    m_mouseSensitivity = MathHelper::Clamp(sens, 0.2f, 3.0f);
}

// ============================================================================
// マウス感度の増減調整（0.1刻みで丸めて誤差を防止）
// ============================================================================
void Camera3D::AdjustMouseSensitivity(float delta)
{
    float next = std::round((m_mouseSensitivity + delta) * 10.0f) / 10.0f;
    SetMouseSensitivity(next);
}

// ============================================================================
// マウスカーソルを画面中央へリセットする
// ============================================================================
void Camera3D::ResetMouseToCenter() const
{
    SetMousePoint(Config::SCREEN_WIDTH / 2, Config::SCREEN_HEIGHT / 2);
}

// ============================================================================
// 毎フレームのカメラ更新処理
// ============================================================================
void Camera3D::Update(
    const VECTOR& targetPos,
    float targetRotY,
    bool enableMouseLook,
    bool isFrontView,
    int stageModelHandle,
    const VECTOR* minBounds,
    const VECTOR* maxBounds
)
{
    // ------------------------------------------------------------------------
    // 1. 視点操作入力（マウス移動 + 矢印キー）
    // ------------------------------------------------------------------------
    const float keyRotSpeed  = 0.040f;
    const float baseMouseSens = 0.0028f;
    const float mouseRotSpeed = baseMouseSens * m_mouseSensitivity;

    // ウィンドウがアクティブな場合のみマウス移動量を読み取って画面中央へ戻す
    if (enableMouseLook && GetWindowActiveFlag() == TRUE)
    {
        int centerX = Config::SCREEN_WIDTH / 2;
        int centerY = Config::SCREEN_HEIGHT / 2;
        int mx = centerX;
        int my = centerY;
        GetMousePoint(&mx, &my);

        int dx = mx - centerX;
        int dy = my - centerY;

        if (dx != 0 || dy != 0)
        {
            m_angleH += static_cast<float>(dx) * mouseRotSpeed;
            m_angleV += static_cast<float>(dy) * mouseRotSpeed;
            SetMousePoint(centerX, centerY);
        }
    }

    // 矢印キーによるカメラ回転補助
    if (CheckHitKey(KEY_INPUT_LEFT))  m_angleH -= keyRotSpeed;
    if (CheckHitKey(KEY_INPUT_RIGHT)) m_angleH += keyRotSpeed;
    if (CheckHitKey(KEY_INPUT_UP))    m_angleV += keyRotSpeed * 0.7f;
    if (CheckHitKey(KEY_INPUT_DOWN))  m_angleV -= keyRotSpeed * 0.7f;

    // 垂直角度（ピッチ）を制限し、地面下への潜り込みや真上でのジンバルロックを防止
    m_angleV = MathHelper::Clamp(m_angleV, 0.08f, 1.15f);

    // [F]キーが押された瞬間、プレイヤーの向いている正面方向へカメラを即座にリセット
    bool resetKey = (CheckHitKey(KEY_INPUT_F) != 0);
    if (resetKey && !m_prevResetKey)
    {
        m_angleH = targetRotY;
    }
    m_prevResetKey = resetKey;

    // ------------------------------------------------------------------------
    // 2. 筋トレQTE中の正面アングルへの滑らかな回り込み補間
    // ------------------------------------------------------------------------
    float targetBlend = isFrontView ? 1.0f : 0.0f;
    m_frontViewBlend = MathHelper::Lerp(m_frontViewBlend, targetBlend, 0.14f);

    // 通常時の背後アングル(m_angleH)と、筋トレ中の正面アングル(targetRotY + π)を補間
    float frontAngleH = targetRotY + MathHelper::PI;
    float renderAngleH = MathHelper::LerpAngle(m_angleH, frontAngleH, m_frontViewBlend);
    float renderAngleV = MathHelper::Lerp(m_angleV, 0.22f, m_frontViewBlend);
    float renderDist   = MathHelper::Lerp(m_distance, 145.0f, m_frontViewBlend);

    // ------------------------------------------------------------------------
    // 3. 注視点のスムーズ追従および理想カメラ座標の算出
    // ------------------------------------------------------------------------
    VECTOR desiredTarget = VGet(targetPos.x, targetPos.y + m_heightOffset, targetPos.z);
    m_targetPos.x = MathHelper::Lerp(m_targetPos.x, desiredTarget.x, 0.25f);
    m_targetPos.y = MathHelper::Lerp(m_targetPos.y, desiredTarget.y, 0.25f);
    m_targetPos.z = MathHelper::Lerp(m_targetPos.z, desiredTarget.z, 0.25f);

    float horizontalDist = renderDist * std::cos(renderAngleV);
    float verticalDist   = renderDist * std::sin(renderAngleV);

    VECTOR desiredCamPos;
    desiredCamPos.x = m_targetPos.x - std::sin(renderAngleH) * horizontalDist;
    desiredCamPos.z = m_targetPos.z - std::cos(renderAngleH) * horizontalDist;
    desiredCamPos.y = m_targetPos.y + verticalDist;

    // ------------------------------------------------------------------------
    // 4. ステージ境界および壁ポリゴンとの衝突回避（カメラが壁の裏へ抜けるのを防止）
    // ------------------------------------------------------------------------
    const float camWallMargin = 12.0f;

    // ステージ外周バウンディングボックス内に制限
    if (minBounds && maxBounds)
    {
        desiredCamPos.x = MathHelper::Clamp(desiredCamPos.x, minBounds->x + camWallMargin, maxBounds->x - camWallMargin);
        desiredCamPos.z = MathHelper::Clamp(desiredCamPos.z, minBounds->z + camWallMargin, maxBounds->z - camWallMargin);
        if (desiredCamPos.y < targetPos.y + 8.0f)
        {
            desiredCamPos.y = targetPos.y + 8.0f;
        }
    }

    // 注視点から理想カメラ位置へ向けてレイキャストし、途中に壁があれば手前に寄せる
    if (stageModelHandle != -1)
    {
        VECTOR rayVec = VSub(desiredCamPos, m_targetPos);
        float rayLen = VSize(rayVec);
        if (rayLen > 0.001f)
        {
            VECTOR rayDir = VScale(rayVec, 1.0f / rayLen);
            VECTOR rayEnd = VAdd(desiredCamPos, VScale(rayDir, camWallMargin));

            MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageModelHandle, -1, m_targetPos, rayEnd);
            if (hit.HitFlag == 1)
            {
                // 床面以外の壁・障害物にヒットした場合、ヒット位置の手前へカメラを引き寄せる
                if (std::abs(hit.Normal.y) < 0.85f || hit.HitPosition.y > targetPos.y + 10.0f)
                {
                    VECTOR hitVec = VSub(hit.HitPosition, m_targetPos);
                    float hitDist = VSize(hitVec);
                    float safeDist = hitDist - camWallMargin;
                    if (safeDist < 28.0f)
                    {
                        safeDist = 28.0f; // キャラクターにめり込まない最小距離
                    }
                    if (safeDist < rayLen)
                    {
                        desiredCamPos = VAdd(m_targetPos, VScale(rayDir, safeDist));
                    }
                }
            }
        }
    }

    // 算出された目標カメラ位置へ滑らかに追従
    m_cameraPos.x = MathHelper::Lerp(m_cameraPos.x, desiredCamPos.x, 0.35f);
    m_cameraPos.y = MathHelper::Lerp(m_cameraPos.y, desiredCamPos.y, 0.35f);
    m_cameraPos.z = MathHelper::Lerp(m_cameraPos.z, desiredCamPos.z, 0.35f);

    // ------------------------------------------------------------------------
    // 5. カメラシェイクタイマーの減衰更新
    // ------------------------------------------------------------------------
    if (m_shakeTimer > 0)
    {
        m_shakeTimer--;
        m_shakeIntensity *= 0.88f;
        if (m_shakeTimer == 0)
        {
            m_shakeIntensity = 0.0f;
        }
    }
}

// ============================================================================
// カメラシェイクの発動
// ============================================================================
void Camera3D::TriggerShake(float intensity, int frames)
{
    if (intensity > m_shakeIntensity)
    {
        m_shakeIntensity = intensity;
    }
    if (frames > m_shakeTimer)
    {
        m_shakeTimer = frames;
    }
}

// ============================================================================
// DxLibの3Dカメラへ現在の視点・注視点を適用する
// ============================================================================
void Camera3D::Apply() const
{
    VECTOR finalCamPos = m_cameraPos;
    VECTOR finalTarget = m_targetPos;

    // シェイク中はカメラ位置と注視点にランダムな揺れオフセットを加算
    if (m_shakeTimer > 0 && m_shakeIntensity > 0.1f)
    {
        float rx = (static_cast<float>(rand() % 200 - 100) / 100.0f) * m_shakeIntensity;
        float ry = (static_cast<float>(rand() % 200 - 100) / 100.0f) * m_shakeIntensity;
        float rz = (static_cast<float>(rand() % 200 - 100) / 100.0f) * m_shakeIntensity;
        finalCamPos = VAdd(finalCamPos, VGet(rx, ry, rz));
        finalTarget = VAdd(finalTarget, VGet(rx * 0.5f, ry * 0.5f, rz * 0.5f));
    }

    SetCameraPositionAndTarget_UpVecY(finalCamPos, finalTarget);
}

// ============================================================================
// カメラの前方水平ベクトル（XZ平面上の単位ベクトル）を取得
// ============================================================================
VECTOR Camera3D::GetForwardXZ() const
{
    return VGet(std::sin(m_angleH), 0.0f, std::cos(m_angleH));
}

// ============================================================================
// カメラの右方水平ベクトル（XZ平面上の単位ベクトル）を取得
// ============================================================================
VECTOR Camera3D::GetRightXZ() const
{
    return VGet(std::cos(m_angleH), 0.0f, -std::sin(m_angleH));
}
