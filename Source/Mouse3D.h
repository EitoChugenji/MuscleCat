#pragma once
#include "GameObject3D.h"
#include <memory>

class Player3D;

// ============================================================================
// 派生クラス：MouseBase3D（3Dネズミの基底クラス）
// ============================================================================
class MouseBase3D : public GameObject3D {
protected:
    float m_vx = 0.0f;
    float m_vz = 0.0f;
    float m_baseSpeed = 2.4f;
    float m_speedBonus = 0.0f; // 仲間の被捕食数 * 0.1f
    float m_animTime = 0.0f;
    int   m_stunTimer = 0;   // スタン残りフレーム数

public:
    MouseBase3D(const VECTOR& pos, float radius, ObjectType type, float baseSpeed);
    virtual ~MouseBase3D() = default;

    void SetSpeedBonus(float bonus) { m_speedBonus = bonus; }
    float GetSpeedBonus() const { return m_speedBonus; }
    virtual float GetCurrentSpeed() const { return IsStunned() ? 0.0f : (m_baseSpeed + m_speedBonus); }

    // スタン管理
    void ApplyStun(int frames) {
        if (frames > m_stunTimer) m_stunTimer = frames;
    }
    bool IsStunned() const { return m_stunTimer > 0; }
    float GetStunRemainingSeconds() const { return static_cast<float>(m_stunTimer) / 60.0f; }

    // スタンエフェクト（頭上のピヨピヨ星・パルス）描画
    void DrawStunEffect();

    // 壁際でのスライディング脱出＆プレイヤー回避移動ベクトル算出
    void CalculateMovementVector(const VECTOR& playerPos, float fleeDistance, float speed, bool isFast);
};

// ============================================================================
// 派生クラス：NormalMouse3D（通常ネズミ）
// ============================================================================
class NormalMouse3D : public MouseBase3D {
private:
    std::shared_ptr<Player3D> m_targetPlayer;
    int m_turnTimer = 0;

public:
    NormalMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player = nullptr);
    virtual ~NormalMouse3D() = default;

    void Update() override;
    void Draw3D() override;
};

// ============================================================================
// 派生クラス：FastMouse3D（高速逃走ネズミ）
// ============================================================================
class FastMouse3D : public MouseBase3D {
private:
    std::shared_ptr<Player3D> m_targetPlayer;
    float m_fleeDistance = 140.0f;
    float m_fleeSpeed    = 2.6f;
    int   m_wanderTimer  = 0;

public:
    FastMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player);
    virtual ~FastMouse3D() = default;

    float GetCurrentFleeSpeed() const { return m_fleeSpeed + m_speedBonus; }
    float GetCurrentWanderSpeed() const { return m_baseSpeed + m_speedBonus * 0.5f; }

    void Update() override;
    void Draw3D() override;
};
