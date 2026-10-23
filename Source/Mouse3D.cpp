#include "Mouse3D.h"
#include "Player3D.h"
#include "FontManager.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "CsvManager.h"
#include "Config.h"
#include "Common.h"
#include "CustomEffectManager.h"
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
    , m_walkAnimPhase(0.0f)
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

void MouseBase3D::UpdateAnimPhase()
{
    m_animTime += 1.0f / 60.0f;

    if (IsStunned())
    {
        return;
    }

    float speed = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    if (speed > 0.05f)
    {
        // 逃走中は必死に超高速ピョコピョコ、徘徊時はトコトコ歩き
        float phaseSpeed = m_isFleeingNow ? (28.0f + speed * 2.2f) : (15.0f + speed * 1.6f);
        m_walkAnimPhase += phaseSpeed * (1.0f / 60.0f);

        // 逃走中は足元からパニック土煙を発生（数フレームに1回控えめに）
        if (m_isFleeingNow && speed > 0.8f)
        {
            int frame = static_cast<int>(m_animTime * 60.0f);
            if (frame % 5 == 0)
            {
                CustomEffectManager::GetInstance().SpawnMouseFleeDust(m_pos, VGet(m_vx, 0.0f, m_vz));
            }
        }
    }
    else
    {
        m_walkAnimPhase += 6.0f * (1.0f / 60.0f);
    }
}

MouseAnimTransform MouseBase3D::GetAnimTransform(float baseScale, float modelRotY) const
{
    MouseAnimTransform t;
    t.drawPos = m_pos;
    t.scaleVec = VGet(baseScale, baseScale, baseScale);
    t.rotY = m_rotY + modelRotY;
    t.localPitchX = 0.0f;
    t.localRollZ = 0.0f;
    t.isStunned = IsStunned();

    if (t.isStunned)
    {
        // 【スタン（気絶）アニメーション】
        // バタンと横倒し（約83度ロール）＋手足ピクピク痙攣
        float fallAngle = MathHelper::PI * 0.46f;
        float twitch = std::sin(m_animTime * 34.0f) * 0.06f;
        t.localRollZ = fallAngle + twitch;
        t.localPitchX = -0.12f + std::cos(m_animTime * 26.0f) * 0.04f;

        // スタン解除直前（残り約0.4秒未満）：プルプル震えて起き上がろうとする
        if (m_stunTimer < 25)
        {
            float shake = std::sin(m_animTime * 65.0f) * 0.16f;
            t.localRollZ += shake;
            t.drawPos.y += std::abs(std::sin(m_animTime * 45.0f)) * 1.5f;
        }

        // 倒れた分高さを地面に近づけ、少しペシャっとコミカルに潰れる
        t.drawPos.y = m_pos.y + 1.5f;
        t.scaleVec.y = baseScale * 0.82f;
        t.scaleVec.x = baseScale * 1.08f;
        t.scaleVec.z = baseScale * 1.06f;

        return t;
    }

    float speed = std::sqrt(m_vx * m_vx + m_vz * m_vz);
    bool isMoving = (speed > 0.05f);

    if (isMoving)
    {
        // 【移動中（通常徘徊・逃走ダッシュ）アニメーション】
        // 1. ピョコピョコ跳ねるホッピングバウンス
        float hopCycle = std::abs(std::sin(m_walkAnimPhase));
        float hopHeight = m_isFleeingNow ? 4.8f : 2.5f;
        if (m_type == ObjectType::FastMouse)
        {
            hopHeight *= 1.15f;
        }
        t.drawPos.y = m_pos.y + hopCycle * hopHeight;

        // 2. 左右の足を交互に出すタタタッとした横揺れ（ロール）
        float rollAmp = m_isFleeingNow ? 0.16f : 0.09f;
        t.localRollZ = std::sin(m_walkAnimPhase * 0.5f) * rollAmp;

        // 3. 前傾姿勢（ピッチ）：逃走中は必死に頭を低くして前傾！
        if (m_isFleeingNow)
        {
            t.localPitchX = -0.24f + std::cos(m_walkAnimPhase) * 0.08f;
        }
        else
        {
            t.localPitchX = std::cos(m_walkAnimPhase) * 0.06f;
        }

        // 4. スカッシュ＆ストレッチ：跳ねる瞬間に縦に伸び、着地で少し横に広がる！
        float stretch = std::sin(m_walkAnimPhase * 2.0f);
        float stretchAmp = m_isFleeingNow ? 0.09f : 0.05f;
        t.scaleVec.y = baseScale * (1.0f - stretch * stretchAmp);
        t.scaleVec.x = baseScale * (1.0f + stretch * (stretchAmp * 0.6f));
        t.scaleVec.z = baseScale * (1.0f + stretch * stretchAmp);
    }
    else
    {
        // 【待機中（Idle）アニメーション】
        // 鼻先をクンクン上下させ、呼吸で微かに息づく
        float sniff = std::sin(m_animTime * 10.0f) * 0.5f;
        t.drawPos.y = m_pos.y + (std::max)(0.0f, sniff);
        t.localPitchX = std::sin(m_animTime * 14.0f) * 0.04f;
    }

    return t;
}

void MouseBase3D::DrawStunEffect()
{
    if (!IsStunned())
    {
        return;
    }

    // ネズミモデル（スケール10倍）の頭上に合わせて高さを16.0f、半径を16.0fに拡張
    float headY = m_pos.y + 16.0f;
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

    // 速度ベクトルの法線方向成分を打ち消し、壁に沿って滑らせる
    float dot = m_vx * nx + m_vz * nz;
    if (dot < 0.0f)
    {
        m_vx -= dot * nx * 1.5f; // 反射成分を加えて跳ね返らせる
        m_vz -= dot * nz * 1.5f;
    }

    // 壁と向かい合っている場合はパトロール目的地を即座に再選択
    ChooseNewPatrolTarget();
}

void MouseBase3D::UpdateWanderMovement(float speed)
{
    m_patrolTimer--;

    float distToTarget = MathHelper::DistanceXZ(m_pos, m_targetWaypoint);
    if (distToTarget < 35.0f || m_patrolTimer <= 0)
    {
        ChooseNewPatrolTarget();
    }

    // 目的地へ向かうスムーズな操舵
    float dirX = m_targetWaypoint.x - m_pos.x;
    float dirZ = m_targetWaypoint.z - m_pos.z;
    float len = std::sqrt(dirX * dirX + dirZ * dirZ);
    if (len > 0.001f)
    {
        dirX /= len;
        dirZ /= len;
    }

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

    // スタン時は完全に行動停止
    if (m_stunTimer > 0)
    {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        UpdateAnimPhase();
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

    UpdateAnimPhase();
}

void NormalMouse3D::Draw3D()
{
    MouseAnimTransform t = GetAnimTransform(ModelConfig::MOUSE_NORMAL_MODEL_SCALE, ModelConfig::MOUSE_NORMAL_MODEL_ROT_Y);

    if (!ModelManager::GetInstance().DrawModelWithTransform
        (
        ModelConfig::MOUSE_NORMAL_MODEL_PATH,
        t.drawPos,
        t.rotY,
        t.scaleVec,
        t.localPitchX,
        t.localRollZ
        ))
    {
        ModelManager::GetInstance().DrawFallbackNormalMouse(t.drawPos, t.rotY, m_animTime, t.localPitchX, t.localRollZ, t.isStunned);
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

    // スタン時は完全に行動停止
    if (m_stunTimer > 0)
    {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        UpdateAnimPhase();
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

    UpdateAnimPhase();
}

void FastMouse3D::Draw3D()
{
    MouseAnimTransform t = GetAnimTransform(ModelConfig::MOUSE_FAST_MODEL_SCALE, ModelConfig::MOUSE_FAST_MODEL_ROT_Y);

    if (!ModelManager::GetInstance().DrawModelWithTransform
        (
        ModelConfig::MOUSE_FAST_MODEL_PATH,
        t.drawPos,
        t.rotY,
        t.scaleVec,
        t.localPitchX,
        t.localRollZ
        ))
    {
        ModelManager::GetInstance().DrawFallbackFastMouse(t.drawPos, t.rotY, m_animTime, t.localPitchX, t.localRollZ, t.isStunned);
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
