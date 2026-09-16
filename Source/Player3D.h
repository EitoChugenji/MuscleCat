#pragma once
#include "GameObject3D.h"
#include <deque>

// ============================================================================
// プレイヤーの筋肉状態
// ============================================================================
enum class MuscleState {
    Normal,     // 通常 (0 Rep)
    Muscular,   // パンプアップ中 (1 Rep以上)
    Soreness    // 筋肉痛 (速度 0.0, 完全移動不可, 5秒間持続)
};

// 3D残像トレイル用構造体
struct TrailPoint3D {
    VECTOR pos;
    float  rotY;
    int    repCount;
    int    alpha;
};

class Camera3D;

// ============================================================================
// Player3D クラス
// ============================================================================
class Player3D : public GameObject3D {
public:
    static constexpr float SPEED_INITIAL  = 3.8f;
    static constexpr float SPEED_SORENESS = 0.0f;

private:
    float m_speed = SPEED_INITIAL;
    MuscleState m_state = MuscleState::Normal;

    int   m_sorenessTimer = 0;       // 筋肉痛残りフレーム (5秒 = 300f)
    int   m_pumpDecayTimer = 0;      // Rep維持残りフレーム (10秒 = 600f)
    int   m_animFrame = 0;
    float m_animTime = 0.0f;

    // スキルチェック & Rep関連
    bool  m_isSkillChecking = false;
    float m_scCursor = 0.0f;         // 針の位置 (0.0f 〜 1.0f)
    float m_scZoneStart = 0.55f;     // 成功ゾーン開始
    float m_scZoneEnd   = 0.85f;     // 成功ゾーン終了
    int   m_repCount = 0;            // レップ数
    bool  m_prevTriggerKey = false;
    int   m_resultShowTimer = 0;
    bool  m_lastResultSuccess = false;
    bool  m_wasDecayed = false;
    int   m_scStoppedTimer = 0;      // キーを押した瞬間に針を止めて結果を見せる演出タイマー

    // 飛びつき（Pounce）関連
    bool   m_isPouncing = false;
    int    m_pounceTimer = 0;
    int    m_pounceCooldown = 0;
    VECTOR m_pounceDir = VGet(0.0f, 0.0f, 1.0f);
    bool   m_prevPounceKey = false;
    std::deque<TrailPoint3D> m_trails; // 3D残像履歴

public:
    Player3D(const VECTOR& pos);
    virtual ~Player3D() = default;

    void Update() override;
    void UpdateWithCamera(const Camera3D& camera);
    void Draw3D() override;
    void Draw2D() override;

    MuscleState GetMuscleState() const { return m_state; }
    float GetCurrentSpeed() const { return m_isPouncing ? GetPounceSpeed() : m_speed; }
    bool IsSkillChecking() const { return m_isSkillChecking; }
    int GetRepCount() const { return m_repCount; }
    float GetSorenessRemainingSeconds() const { return static_cast<float>(m_sorenessTimer) / 60.0f; }
    float GetPumpDecayRemainingSeconds() const { return static_cast<float>(m_pumpDecayTimer) / 60.0f; }

    // 飛びつきパラメータ動的算出
    float GetPounceSpeed() const {
        if (m_repCount <= 4) return 7.5f;
        if (m_repCount == 5) return 9.5f;
        if (m_repCount == 6) return 12.0f;
        return 14.0f + static_cast<float>(m_repCount - 7) * 1.0f;
    }

    int GetPounceDuration() const {
        if (m_repCount <= 4) return 12;
        if (m_repCount == 5) return 14;
        if (m_repCount == 6) return 16;
        return 18;
    }

    int GetPounceCooldownMax() const {
        if (m_repCount <= 4) return 72; // 1.2秒
        if (m_repCount == 5) return 60; // 1.0秒
        if (m_repCount == 6) return 54; // 0.9秒
        return 48;                     // 0.8秒
    }

    float GetPounceRadius() const {
        if (m_repCount <= 4) return 18.0f;
        if (m_repCount == 5) return 21.0f;
        if (m_repCount == 6) return 24.0f;
        return 28.0f;
    }

    float GetRadius() const override {
        return m_isPouncing ? GetPounceRadius() : m_radius;
    }

    bool IsPouncing() const { return m_isPouncing; }
    bool CanPounce() const {
        return (m_repCount >= 4) && (m_state != MuscleState::Soreness) && !m_isSkillChecking && (m_pounceCooldown <= 0);
    }
    float GetPounceCooldownRatio() const {
        int maxCd = GetPounceCooldownMax();
        return (m_pounceCooldown > 0) ? (static_cast<float>(m_pounceCooldown) / static_cast<float>(maxCd)) : 0.0f;
    }
};
