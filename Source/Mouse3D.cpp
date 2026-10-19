#include "Mouse3D.h"
#include "Player3D.h"
#include "FontManager.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "CsvManager.h"
#include "Config.h"
#include "Common.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

// MouseBase3D 実装
MouseBase3D::MouseBase3D(const VECTOR& pos, float radius, ObjectType type, float baseSpeed, float speedGrowth, float maxSpeed)
    : GameObject3D(pos, radius, type)
    , m_baseSpeed(baseSpeed)
    , m_speedGrowthPerUnit(speedGrowth)
    , m_maxSpeed(maxSpeed)
    , m_speedBonus(0.0f)
    , m_animTime(0.0f)
    , m_stunTimer(0)
    , m_prevPos(pos)
    , m_targetWaypoint(pos)
    , m_patrolTimer(0)
    , m_isFleeingNow(false)
{
    float angle = static_cast<float>(rand() % 360) * MathHelper::DEG_TO_RAD;
    m_vx = std::sin(angle) * m_baseSpeed;
    m_vz = std::cos(angle) * m_baseSpeed;
    m_rotY = angle;
    ChooseNewPatrolTarget();
}

void MouseBase3D::DrawStunEffect()
{
    if (!IsStunned())
    {
        return;
    }

    // ネズミモデル（スケール10倍）の頭上に合わせて高さを30.0f、半径を16.0fに拡張
    float headY = m_pos.y + 30.0f;
    float spinSpeed = m_animTime * 10.0f;
    float ringRadius = 16.0f;

    // くるくる回る3つの星（球体サイズ3.8f）
    for (int i = 0; i < 3; ++i)
    {
        float angle = spinSpeed + static_cast<float>(i) * (2.0f * MathHelper::PI / 3.0f);
        float starX = m_pos.x + std::cos(angle) * ringRadius;
        float starZ = m_pos.z + std::sin(angle) * ringRadius;
        float starY = headY + std::sin(angle * 2.0f) * 3.5f;

        VECTOR starPos = VGet(starX, starY, starZ);
        DrawSphere3D(starPos, 3.8f, 8, GetColor(255, 230, 40), GetColor(255, 255, 120), TRUE);
    }

    // 頭上の黄色い気絶リング（ピヨピヨリング）
    int ringSegments = 20;
    for (int i = 0; i < ringSegments; ++i)
    {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / ringSegments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / ringSegments);
        VECTOR p1 = VGet(m_pos.x + std::cos(a1) * ringRadius, headY, m_pos.z + std::sin(a1) * ringRadius);
        VECTOR p2 = VGet(m_pos.x + std::cos(a2) * ringRadius, headY, m_pos.z + std::sin(a2) * ringRadius);
        DrawLine3D(p1, p2, GetColor(255, 240, 80));
    }
}

void MouseBase3D::Draw2D()
{
    if (!IsStunned())
    {
        return;
    }

    // ネズミの頭上3D座標をスクリーン座標に変換して2Dオーバーレイ表示
    VECTOR headPos3D = VGet(m_pos.x, m_pos.y + 36.0f, m_pos.z);
    VECTOR screenPos = ConvWorldPosToScreenPos(headPos3D);

    // カメラ視野内（画面前方）にある場合のみ描画
    if (screenPos.z > 0.0f)
    {
        int sx = static_cast<int>(screenPos.x);
        int sy = static_cast<int>(screenPos.y);

        const auto& fm = FontManager::GetInstance();
        int font16 = fm.GetFont16();

        // 点滅表示（気絶感を演出）
        int alpha = (m_stunTimer % 10 < 5) ? 255 : 190;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        char buf[64];
        std::snprintf(buf, sizeof(buf), "★ STUN! (%.1fs)", GetStunRemainingSeconds());
        int strW = GetDrawStringWidthToHandle(buf, static_cast<int>(std::strlen(buf)), font16);

        // 背景小パネル
        DrawBox(sx - strW / 2 - 6, sy - 18, sx + strW / 2 + 6, sy + 4, GetColor(25, 25, 35), TRUE);
        DrawBox(sx - strW / 2 - 6, sy - 18, sx + strW / 2 + 6, sy + 4, GetColor(255, 230, 40), FALSE);

        // テキスト
        DrawStringToHandle(sx - strW / 2, sy - 16, buf, GetColor(255, 240, 50), font16);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

void MouseBase3D::ChooseNewPatrolTarget()
{
    // Position.csv から読み込まれた巡回・スポーン座標リストを使用
    std::vector<VECTOR> points = CsvManager::GetInstance().GetMousePositionsForMap(m_mapType);
    if (points.empty())
    {
        points.push_back(VGet(0.0f, 5.0f, 0.0f));
    }

    int pointCount = static_cast<int>(points.size());
    float jitter = (m_mapType == ModelConfig::MapType::House) ? 30.0f : 60.0f;
    float minDist = (m_mapType == ModelConfig::MapType::House) ? 120.0f : 350.0f;

    // 現在地から一定以上離れた地点を優先してランダムに選択
    int bestIdx = rand() % pointCount;
    for (int retry = 0; retry < 5; ++retry)
    {
        int idx = rand() % pointCount;
        float d = MathHelper::DistanceXZ(m_pos, points[idx]);
        if (d > minDist)
        {
            bestIdx = idx;
            break;
        }
    }

    // 目的地の周囲にゆらぎを加えて同じ点に集まらないようにする
    float ox = static_cast<float>((rand() % static_cast<int>(jitter * 2.0f)) - jitter);
    float oz = static_cast<float>((rand() % static_cast<int>(jitter * 2.0f)) - jitter);
    m_targetWaypoint = VGet(points[bestIdx].x + ox, points[bestIdx].y, points[bestIdx].z + oz);
    m_patrolTimer = 140 + rand() % 140; // 2.3〜4.6秒
}

void MouseBase3D::OnWallCollision(const VECTOR& pushNormal)
{
    float nLen = std::sqrt(pushNormal.x * pushNormal.x + pushNormal.z * pushNormal.z);
    if (nLen < 0.001f)
    {
        return;
    }
    float nx = pushNormal.x / nLen;
    float nz = pushNormal.z / nLen;

    float currentSpd = GetCurrentSpeed();

    // 壁に向かっていた速度成分（内積）
    float dot = m_vx * nx + m_vz * nz;

    // 壁の接線ベクトル（壁沿いにスムーズに逃げる方向）
    float tx = -nz;
    float tz = nx;
    float tDot = m_vx * tx + m_vz * tz;
    if (tDot < 0.0f)
    {
        tx = -tx;
        tz = -tz;
    }

    if (dot < 0.0f)
    {
        // 壁に向かっていた場合: 壁法線への強い反発 + 壁沿いスライディング
        float bounceVx = -dot * nx;
        float bounceVz = -dot * nz;
        float slideVx = tx * currentSpd * 0.75f;
        float slideVz = tz * currentSpd * 0.75f;

        m_vx = bounceVx * 1.3f + slideVx;
        m_vz = bounceVz * 1.3f + slideVz;
    }
    else
    {
        // すでに壁から離れる方向なら、さらに法線方向へ加速＋壁沿いスライド
        m_vx = nx * currentSpd * 0.85f + tx * currentSpd * 0.5f;
        m_vz = nz * currentSpd * 0.85f + tz * currentSpd * 0.5f;
    }

    // スピードに合わせて正規化
    float vLen = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    if (vLen > 0.0001f)
    {
        m_vx = (m_vx / vLen) * currentSpd;
        m_vz = (m_vz / vLen) * currentSpd;
    }

    // 向きを即座に更新
    m_rotY = std::atan2(m_vx, m_vz);

    // 新たな安全な目標地点を再選択し、進行を継続（部屋の内側へ誘導）
    ChooseNewPatrolTarget();
    m_patrolTimer = 90 + rand() % 90;
}

void MouseBase3D::UpdateWanderMovement(float speed)
{
    m_patrolTimer--;

    float distToTarget = MathHelper::DistanceXZ(m_pos, m_targetWaypoint);
    if (distToTarget < 40.0f || m_patrolTimer <= 0)
    {
        ChooseNewPatrolTarget();
    }

    float toX = m_targetWaypoint.x - m_pos.x;
    float toZ = m_targetWaypoint.z - m_pos.z;
    float toLen = std::sqrt(toX * toX + toZ * toZ);

    if (toLen > 0.001f)
    {
        toX /= toLen;
        toZ /= toLen;
    }
    else
    {
        toX = 1.0f;
        toZ = 0.0f;
    }

    // ネズミらしい生き生きとした蛇行（サイン波）
    float wobble = std::sin(m_animTime * 6.5f) * 0.22f;
    float dirX = toX + (-toZ) * wobble;
    float dirZ = toZ + (toX) * wobble;
    float dLen = std::sqrt(dirX * dirX + dirZ * dirZ);
    if (dLen > 0.001f)
    {
        dirX /= dLen;
        dirZ /= dLen;
    }

    // 進行方向へスムーズに追従
    m_vx = MathHelper::Lerp(m_vx, dirX * speed, 0.18f);
    m_vz = MathHelper::Lerp(m_vz, dirZ * speed, 0.18f);

    float curLen = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    if (curLen > 0.0001f)
    {
        m_vx = (m_vx / curLen) * speed;
        m_vz = (m_vz / curLen) * speed;
    }
}

void MouseBase3D::UpdateFleeMovement(const VECTOR& playerPos, float speed)
{
    float dx = m_pos.x - playerPos.x;
    float dz = m_pos.z - playerPos.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist > 0.001f)
    {
        dx /= dist;
        dz /= dist;
    }
    else
    {
        dx = 1.0f;
        dz = 0.0f;
    }

    // プレイヤーから遠ざかる方向へ即時舵を切る
    m_vx = MathHelper::Lerp(m_vx, dx * speed, 0.28f);
    m_vz = MathHelper::Lerp(m_vz, dz * speed, 0.28f);

    float curLen = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    if (curLen > 0.0001f)
    {
        m_vx = (m_vx / curLen) * speed;
        m_vz = (m_vz / curLen) * speed;
    }
}

void MouseBase3D::CalculateMovementVector(const VECTOR& playerPos, float fleeDistance, float speed, bool isFast)
{
    float dist = MathHelper::DistanceXZ(m_pos, playerPos);
    if (dist < fleeDistance)
    {
        UpdateFleeMovement(playerPos, speed);
    }
    else
    {
        UpdateWanderMovement(speed);
    }
}

// NormalMouse3D 実装
NormalMouse3D::NormalMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player)
    : MouseBase3D(
        pos,
        12.0f,
        ObjectType::NormalMouse,
        CsvManager::GetInstance().GetMouseStatus().baseSpeed,
        CsvManager::GetInstance().GetMouseStatus().speedGrowthPerUnit,
        CsvManager::GetInstance().GetMouseStatus().maxSpeed
    )
    , m_targetPlayer(player)
{
}

void NormalMouse3D::Update()
{
    m_prevPos = m_pos;
    m_animTime += 1.0f / 60.0f;

    // スタン時は完全に行動停止
    if (m_stunTimer > 0)
    {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        return;
    }

    float currentSpeed = GetCurrentSpeed();

    m_isFleeingNow = false;
    if (m_targetPlayer && m_targetPlayer->IsAlive())
    {
        VECTOR pPos = m_targetPlayer->GetPos();
        float dist = MathHelper::DistanceXZ(m_pos, pPos);
        if (dist < 130.0f)
        {
            UpdateFleeMovement(pPos, currentSpeed);
            m_isFleeingNow = true;
        }
    }

    if (!m_isFleeingNow)
    {
        UpdateWanderMovement(currentSpeed * 0.85f);
    }

    // 移動
    m_pos.x += m_vx;
    m_pos.z += m_vz;

    // 向きの更新
    if (std::abs(m_vx) > 0.01f || std::abs(m_vz) > 0.01f)
    {
        float targetAngle = std::atan2(m_vx, m_vz);
        m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.30f);
    }
}

void NormalMouse3D::Draw3D()
{
    if (!ModelManager::GetInstance().DrawModelIfLoaded
        (
        ModelConfig::MOUSE_NORMAL_MODEL_PATH, m_pos, m_rotY + ModelConfig::MOUSE_NORMAL_MODEL_ROT_Y,
        ModelConfig::MOUSE_NORMAL_MODEL_SCALE
        ))
    {
        ModelManager::GetInstance().DrawFallbackNormalMouse(m_pos, m_rotY, m_animTime);
    }

    DrawStunEffect();
}

// FastMouse3D 実装
FastMouse3D::FastMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player)
    : MouseBase3D(
        pos,
        11.0f,
        ObjectType::FastMouse,
        CsvManager::GetInstance().GetFastMouseStatus().baseSpeed,
        CsvManager::GetInstance().GetFastMouseStatus().speedGrowthPerUnit,
        CsvManager::GetInstance().GetFastMouseStatus().maxSpeed
    )
    , m_targetPlayer(player)
    , m_fleeDistance(160.0f)
    , m_fleeSpeed(CsvManager::GetInstance().GetFastMouseStatus().baseSpeed)
{
}

void FastMouse3D::Update()
{
    m_prevPos = m_pos;
    m_animTime += 1.0f / 60.0f;

    // スタン時は完全に行動停止
    if (m_stunTimer > 0)
    {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        return;
    }

    m_isFleeingNow = false;
    if (m_targetPlayer && m_targetPlayer->IsAlive())
    {
        VECTOR pPos = m_targetPlayer->GetPos();
        float dist = MathHelper::DistanceXZ(m_pos, pPos);

        if (dist < m_fleeDistance)
        {
            m_isFleeingNow = true;
            float fleeSpeed = GetCurrentFleeSpeed();
            UpdateFleeMovement(pPos, fleeSpeed);
        }
    }

    if (!m_isFleeingNow)
    {
        float wanderSpeed = GetCurrentWanderSpeed();
        UpdateWanderMovement(wanderSpeed);
    }

    // 移動
    m_pos.x += m_vx;
    m_pos.z += m_vz;

    // 向きの更新
    if (std::abs(m_vx) > 0.01f || std::abs(m_vz) > 0.01f)
    {
        float targetAngle = std::atan2(m_vx, m_vz);
        m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.40f);
    }
}

void FastMouse3D::Draw3D()
{
    if (!ModelManager::GetInstance().DrawModelIfLoaded
        (
        ModelConfig::MOUSE_FAST_MODEL_PATH, m_pos, m_rotY + ModelConfig::MOUSE_FAST_MODEL_ROT_Y,
        ModelConfig::MOUSE_FAST_MODEL_SCALE
        ))
    {
        ModelManager::GetInstance().DrawFallbackFastMouse(m_pos, m_rotY, m_animTime);
    }

    DrawStunEffect();
}

void NormalMouse3D::Draw2D()
{
    // 基底のスタン表示（STUN!）
    MouseBase3D::Draw2D();

    // 逃走中のコミカルな汗漫符（バカゲー風）
    if (m_isFleeingNow && !IsStunned())
    {
        VECTOR headPos3D = VGet(m_pos.x, m_pos.y + 26.0f, m_pos.z);
        VECTOR screenPos = ConvWorldPosToScreenPos(headPos3D);
        if (screenPos.z > 0.0f)
        {
            int sx = static_cast<int>(screenPos.x);
            int sy = static_cast<int>(screenPos.y);
            // 水色のアニメ汗しずくマーク（プルプル揺れる）
            float sweatOffset = std::sin(m_animTime * 20.0f) * 3.0f;
            DrawCircle(sx + 14, sy - 10 + static_cast<int>(sweatOffset), 4, GetColor(100, 200, 255), TRUE);
            DrawCircle(sx + 14, sy - 10 + static_cast<int>(sweatOffset), 4, GetColor(255, 255, 255), FALSE);
            DrawTriangle
            (
                sx + 14, sy - 17 + static_cast<int>(sweatOffset),
                sx + 11, sy - 11 + static_cast<int>(sweatOffset),
                sx + 17, sy - 11 + static_cast<int>(sweatOffset),
                GetColor(100, 200, 255), TRUE
            );
        }
    }
}

void FastMouse3D::Draw2D()
{
    // 基底のスタン表示（STUN!）
    MouseBase3D::Draw2D();

    // 高速ネズミ逃走中の大パニック漫符（汗マーク2個＆「！？」）
    if (m_isFleeingNow && !IsStunned())
    {
        VECTOR headPos3D = VGet(m_pos.x, m_pos.y + 28.0f, m_pos.z);
        VECTOR screenPos = ConvWorldPosToScreenPos(headPos3D);
        if (screenPos.z > 0.0f)
        {
            int sx = static_cast<int>(screenPos.x);
            int sy = static_cast<int>(screenPos.y);
            int font13 = FontManager::GetInstance().GetFont13();

            // 「！？」コミカル吹き出し
            DrawFormatStringToHandle(sx - 10, sy - 28, GetColor(255, 60, 60), font13, "!?");

            // 飛び散るダブル汗
            float sw1 = std::sin(m_animTime * 22.0f) * 3.0f;
            float sw2 = std::cos(m_animTime * 22.0f) * 3.0f;
            DrawCircle(sx + 16, sy - 8 + static_cast<int>(sw1), 4, GetColor(80, 210, 255), TRUE);
            DrawCircle(sx - 16, sy - 8 + static_cast<int>(sw2), 4, GetColor(80, 210, 255), TRUE);
        }
    }
}