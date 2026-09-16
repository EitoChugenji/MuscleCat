#include "Mouse3D.h"
#include "Player3D.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "Config.h"
#include "Common.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ============================================================================
// MouseBase3D 実装
// ============================================================================
MouseBase3D::MouseBase3D(const VECTOR& pos, float radius, ObjectType type, float baseSpeed)
    : GameObject3D(pos, radius, type)
    , m_baseSpeed(baseSpeed)
    , m_speedBonus(0.0f)
    , m_animTime(0.0f)
    , m_stunTimer(0) {
    float angle = static_cast<float>(rand() % 360) * MathHelper::DEG_TO_RAD;
    m_vx = std::sin(angle) * m_baseSpeed;
    m_vz = std::cos(angle) * m_baseSpeed;
    m_rotY = angle;
}

void MouseBase3D::DrawStunEffect() {
    if (!IsStunned()) return;

    // 頭上で回転する星・ひよこ（黄色い球とリング）
    float headY = m_pos.y + 14.0f;
    float spinSpeed = m_animTime * 10.0f;
    float ringRadius = 8.5f;

    // くるくる回る3つの星
    for (int i = 0; i < 3; ++i) {
        float angle = spinSpeed + static_cast<float>(i) * (2.0f * MathHelper::PI / 3.0f);
        float starX = m_pos.x + std::cos(angle) * ringRadius;
        float starZ = m_pos.z + std::sin(angle) * ringRadius;
        float starY = headY + std::sin(angle * 2.0f) * 2.0f;

        VECTOR starPos = VGet(starX, starY, starZ);
        DrawSphere3D(starPos, 2.2f, 8, GetColor(255, 230, 40), GetColor(255, 240, 100), TRUE);
    }

    // 頭上の黄色い気絶リング（ピヨピヨリング）
    int ringSegments = 16;
    for (int i = 0; i < ringSegments; ++i) {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / ringSegments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / ringSegments);
        VECTOR p1 = VGet(m_pos.x + std::cos(a1) * ringRadius, headY, m_pos.z + std::sin(a1) * ringRadius);
        VECTOR p2 = VGet(m_pos.x + std::cos(a2) * ringRadius, headY, m_pos.z + std::sin(a2) * ringRadius);
        DrawLine3D(p1, p2, GetColor(255, 240, 80));
    }
}

void MouseBase3D::CalculateMovementVector(const VECTOR& playerPos, float fleeDistance, float speed, bool isFast) {
    float dx = m_pos.x - playerPos.x;
    float dz = m_pos.z - playerPos.z;
    float dist = std::sqrt(dx * dx + dz * dz);

    float wallMargin = 40.0f;
    float leftDist   = m_pos.x - (-Config::STAGE_HALF_WIDTH);
    float rightDist  = Config::STAGE_HALF_WIDTH - m_pos.x;
    float backDist   = m_pos.z - (-Config::STAGE_HALF_DEPTH);
    float frontDist  = Config::STAGE_HALF_DEPTH - m_pos.z;

    bool nearLeft  = (leftDist < wallMargin);
    bool nearRight = (rightDist < wallMargin);
    bool nearBack  = (backDist < wallMargin);
    bool nearFront = (frontDist < wallMargin);
    bool nearWall  = (nearLeft || nearRight || nearBack || nearFront);

    if (dist < fleeDistance && dist > 0.001f) {
        // プレイヤーから離れる基本ベクトル
        float fleeDirX = dx / dist;
        float fleeDirZ = dz / dist;

        // 壁際で追い詰められたときの壁沿いスライディング＆内側回り込み脱出
        if (nearWall) {
            float wallNormalX = 0.0f;
            float wallNormalZ = 0.0f;

            if (nearLeft)  wallNormalX += (wallMargin - leftDist) / wallMargin;
            if (nearRight) wallNormalX -= (wallMargin - rightDist) / wallMargin;
            if (nearBack)  wallNormalZ += (wallMargin - backDist) / wallMargin;
            if (nearFront) wallNormalZ -= (wallMargin - frontDist) / wallMargin;

            // 壁に押し付けられている場合、壁に沿った接線方向に横滑り
            if (nearLeft || nearRight) {
                // Xの壁際: Z方向に逃げる（プレイヤーのZから離れる方向を優先）
                float tangentZ = (dz >= 0.0f) ? 1.0f : -1.0f;
                fleeDirZ = tangentZ * 1.2f;
                fleeDirX = wallNormalX * 1.5f; // 壁から離れる内向きの力
            }
            if (nearBack || nearFront) {
                // Zの壁際: X方向に逃げる（プレイヤーのXから離れる方向を優先）
                float tangentX = (dx >= 0.0f) ? 1.0f : -1.0f;
                fleeDirX = tangentX * 1.2f;
                fleeDirZ = wallNormalZ * 1.5f; // 壁から離れる内向きの力
            }
        }

        // ベクトル正規化
        float len = std::sqrt(fleeDirX * fleeDirX + fleeDirZ * fleeDirZ);
        if (len > 0.0001f) {
            fleeDirX /= len;
            fleeDirZ /= len;
        }

        m_vx = fleeDirX * speed;
        m_vz = fleeDirZ * speed;
    } else {
        // プレイヤーが近くにいない時でも、壁に近づきすぎたら自然に中央へ方向転換
        if (nearWall) {
            if (nearLeft && m_vx < 0.0f)   m_vx = std::abs(m_vx);
            if (nearRight && m_vx > 0.0f)  m_vx = -std::abs(m_vx);
            if (nearBack && m_vz < 0.0f)   m_vz = std::abs(m_vz);
            if (nearFront && m_vz > 0.0f)  m_vz = -std::abs(m_vz);
        }
    }
}

// ============================================================================
// NormalMouse3D 実装
// ============================================================================
NormalMouse3D::NormalMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player)
    : MouseBase3D(pos, 12.0f, ObjectType::NormalMouse, 2.4f)
    , m_targetPlayer(player) {
}

void NormalMouse3D::Update() {
    m_animTime += 1.0f / 60.0f;

    // スタン時は完全に行動停止
    if (m_stunTimer > 0) {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        return;
    }

    float currentSpeed = GetCurrentSpeed();

    bool isFleeing = false;
    if (m_targetPlayer && m_targetPlayer->IsAlive()) {
        VECTOR pPos = m_targetPlayer->GetPos();
        float dist = MathHelper::DistanceXZ(m_pos, pPos);
        if (dist < 110.0f) {
            CalculateMovementVector(pPos, 110.0f, currentSpeed, false);
            isFleeing = true;
        }
    }

    if (!isFleeing) {
        m_turnTimer--;
        if (m_turnTimer <= 0) {
            m_turnTimer = 40 + rand() % 80;
            float angle = static_cast<float>(rand() % 360) * MathHelper::DEG_TO_RAD;
            m_vx = std::sin(angle) * currentSpeed;
            m_vz = std::cos(angle) * currentSpeed;
        }

        // 壁からの自然な跳ね返り
        if (m_targetPlayer) {
            CalculateMovementVector(m_targetPlayer->GetPos(), 0.0f, currentSpeed, false);
        }
    }

    // 移動
    m_pos.x += m_vx;
    m_pos.z += m_vz;

    // 向きの更新
    if (std::abs(m_vx) > 0.01f || std::abs(m_vz) > 0.01f) {
        float targetAngle = std::atan2(m_vx, m_vz);
        m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.25f);
    }
}

void NormalMouse3D::Draw3D() {
    if (!ModelManager::GetInstance().DrawModelIfLoaded(
            ModelConfig::MOUSE_NORMAL_MODEL_PATH, m_pos, m_rotY + ModelConfig::MOUSE_NORMAL_MODEL_ROT_Y,
            ModelConfig::MOUSE_NORMAL_MODEL_SCALE)) {
        ModelManager::GetInstance().DrawFallbackNormalMouse(m_pos, m_rotY, m_animTime);
    }
    DrawStunEffect();
}

// ============================================================================
// FastMouse3D 実装
// ============================================================================
FastMouse3D::FastMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player)
    : MouseBase3D(pos, 11.0f, ObjectType::FastMouse, 2.2f)
    , m_targetPlayer(player) {
}

void FastMouse3D::Update() {
    m_animTime += 1.0f / 60.0f;

    // スタン時は完全に行動停止
    if (m_stunTimer > 0) {
        m_stunTimer--;
        m_vx = 0.0f;
        m_vz = 0.0f;
        return;
    }

    bool isFleeing = false;
    if (m_targetPlayer && m_targetPlayer->IsAlive()) {
        VECTOR pPos = m_targetPlayer->GetPos();
        float dist = MathHelper::DistanceXZ(m_pos, pPos);

        if (dist < m_fleeDistance) {
            isFleeing = true;
            float fleeSpeed = GetCurrentFleeSpeed();
            CalculateMovementVector(pPos, m_fleeDistance, fleeSpeed, true);
        }
    }

    if (!isFleeing) {
        m_wanderTimer--;
        if (m_wanderTimer <= 0) {
            m_wanderTimer = 30 + rand() % 50;
            float wanderSpeed = GetCurrentWanderSpeed();
            float angle = static_cast<float>(rand() % 360) * MathHelper::DEG_TO_RAD;
            m_vx = std::sin(angle) * wanderSpeed;
            m_vz = std::cos(angle) * wanderSpeed;
        }

        if (m_targetPlayer) {
            CalculateMovementVector(m_targetPlayer->GetPos(), 0.0f, GetCurrentWanderSpeed(), true);
        }
    }

    // 移動
    m_pos.x += m_vx;
    m_pos.z += m_vz;

    // 向きの更新
    if (std::abs(m_vx) > 0.01f || std::abs(m_vz) > 0.01f) {
        float targetAngle = std::atan2(m_vx, m_vz);
        m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.35f);
    }
}

void FastMouse3D::Draw3D() {
    if (!ModelManager::GetInstance().DrawModelIfLoaded(
            ModelConfig::MOUSE_FAST_MODEL_PATH, m_pos, m_rotY + ModelConfig::MOUSE_FAST_MODEL_ROT_Y,
            ModelConfig::MOUSE_FAST_MODEL_SCALE)) {
        ModelManager::GetInstance().DrawFallbackFastMouse(m_pos, m_rotY, m_animTime);
    }
    DrawStunEffect();
}
