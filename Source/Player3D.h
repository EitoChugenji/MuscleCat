#pragma once
#include "GameObject3D.h"
#include "ModelConfig.h"
#include <deque>
#include <vector>

// プレイヤーの筋肉状態
enum class MuscleState
{
    Normal,     // 通常 (0 Rep)
    Muscular,   // パンプアップ中 (1 Rep以上)
    Soreness    // 筋肉痛 (速度 0.0, 完全移動不可, 5秒間持続)
};

// 3D残像トレイル用構造体
struct TrailPoint3D
{
    VECTOR pos;
    float  rotY;
    int    repCount;
    int    alpha;
};

class Camera3D;

// Player3D クラス
class Player3D : public GameObject3D
{
public:
    static constexpr float SPEED_INITIAL  = 4.0f;   // デフォルト基本速度 (Statu.csv: Cat base_speed)
    static constexpr float SPEED_SORENESS = 0.0f;
    static constexpr int   MAX_EFFECTIVE_REP = 15;  // 能力値反映上限 (レベル15)
    static constexpr int   PUMP_DECAY_FRAMES = 900; // Rep維持残りフレーム (15秒 = 900f)
    static constexpr int   REQUIRED_ATTACK_REP = 4; // 飛びつき・スタン攻撃(タックル)解放に必要な最低Rep数

    // Statu.csv の設定値（base_speed, speed_growth_per_unit, max_speed）から現在Repに応じた速度を算出
    float CalculateSpeedForRep(int effectiveRep) const;

private:
    float m_speed = SPEED_INITIAL;
    MuscleState m_state = MuscleState::Normal;

    int   m_sorenessTimer = 0;       // 筋肉痛残りフレーム (5秒 = 300f)
    int   m_pumpDecayTimer = 0;      // Rep維持残りフレーム (15秒 = 900f)
    float m_groundY = 0.0f;          // 現在の接地地面高さ（床・坂道対応）
    VECTOR m_prevPos = VGet(0.0f, 0.0f, 0.0f); // 移動前の位置（崖・はみ出し防止用）
    ModelConfig::MapType m_mapType = ModelConfig::MapType::SlopeHills;
    int   m_animFrame = 0;
    float m_animTime = 0.0f;

    // スキルチェック & Rep関連
    bool  m_isSkillChecking = false;
    bool  m_isTitleSquat = false;
    bool  m_isTitlePushUp = false;
    float m_scCursor = 0.0f;         // 針の位置 (0.0f 〜 1.0f)
    float m_scCursorDir = 1.0f;      // 針の反復移動方向 (1.0f: 右向き, -1.0f: 左向き)
    float m_scZoneStart = 0.55f;     // 成功ゾーン開始
    float m_scZoneEnd   = 0.85f;     // 成功ゾーン終了
    float m_prevScZoneCenter = -1.0f;// 前回の成功ゾーン中心位置（前回と被らないように離す用）
    int   m_scHoldingFrames = 0;     // スキルチェック長押しフレーム数（長時間放置セーフティ用）
    int   m_repCount = 0;            // レップ数
    bool  m_prevTriggerKey = false;
    int   m_resultShowTimer = 0;
    bool  m_lastResultSuccess = false;
    bool  m_wasDecayed = false;
    int   m_scStoppedTimer = 0;      // キーを押した瞬間に針を止めて結果を見せる演出タイマー
    int   m_needRepNoticeTimer = 0;  // Rep不足(Lv.4未満)時のアクション案内表示タイマー

    // 飛びつき（Pounce）関連
    bool   m_isPouncing = false;
    int    m_pounceTimer = 0;
    int    m_pounceCooldown = 0;
    VECTOR m_pounceDir = VGet(0.0f, 0.0f, 1.0f);
    bool   m_prevPounceKey = false;
    std::deque<TrailPoint3D> m_trails; // 3D残像履歴

    // タックル（Tackle: [E] キー）関連
    bool   m_isTackling = false;
    int    m_tackleTimer = 0;
    int    m_tackleCooldown = 0;
    VECTOR m_tackleDir = VGet(0.0f, 0.0f, 1.0f);
    bool   m_prevTackleKey = false;

    // スタン（壁・家具衝突時）関連
    int    m_stunTimer = 0;          // 猫のスタン残りフレーム数
    int    m_catchCelebrateTimer = 0;// ネズミ捕獲時の祝祭オーラ残りフレーム数

    // デバッグチート関連
    bool   m_cheatNoCooldown = false;

    // マウス操作（左クリック長押し移動）関連
    bool   m_isMouseMoving = false;
    int    m_mouseTargetX = 0;
    int    m_mouseTargetY = 0;

    // 3Dモデル & アニメーション関連（cat.mv1）
    int    m_modelHandle = -1;
    int    m_attachAnimIndex = -1;
    int    m_currentAnimIndex = -1;
    float  m_animPlayTime = 0.0f;

    // 各アニメーションのインデックス（cat.mv1）
    int    m_animIndexRun = 0;       // Armature|01_Run (走り・移動)
    int    m_animIndexAttack = 1;    // Armature|02_Attack (飛びつき)
    int    m_animIndexSquat = 2;     // Armature|03_squat (筋トレQTE / スクワット)
    int    m_animIndexIdol = 3;      // Armature|04_Idol (待機)
    int    m_animIndexPush = 4;      // Armature|05_Push または Attack (Eキー / タックル)
    int    m_animIndexPushUp = 4;    // Armature|05_PushUp (腕立て伏せ)
    int    m_animIndexSoreness = 5;  // Armature|06_MuscleSoreness (筋肉痛)

    // 手のボーンフレーム（バーベル両手アタッチ用）
    int    m_lHandFrame = -1;
    int    m_rHandFrame = -1;
    int    m_lHandKnuckleFrame = -1;
    int    m_rHandKnuckleFrame = -1;

    // アクセサリ（Barbell.mv1）
    int    m_barbellModelHandle = -1;

    void InitModel();
    void UpdateAnimation(bool isMoving);
    void DrawBarbell(float catScale);

public:
    Player3D(const VECTOR& pos);
    virtual ~Player3D();

    void Update() override;
    void UpdateWithCamera(Camera3D& camera, const std::vector<VECTOR>& targetPositions = {});
    void Draw3D() override;
    void Draw2D() override;

    // デバッグチート用メソッド
    void AddRep(int amount = 1);
    
    void SetNoCooldown(bool enabled)
    {
        m_cheatNoCooldown = enabled;
    }
    
    bool IsNoCooldown() const
    {
        return m_cheatNoCooldown;
    }
    
    void ToggleNoCooldown()
    {
        m_cheatNoCooldown = !m_cheatNoCooldown;
    }

    MuscleState GetMuscleState() const
    {
        return m_state;
    }

    void SetGroundY(float groundY)
    {
        m_groundY = groundY;
        if (!m_isPouncing)
        {
            m_pos.y = m_groundY;
        }
    }

    float GetGroundY() const
    {
        return m_groundY;
    }

    const VECTOR& GetPrevPos() const
    {
        return m_prevPos;
    }

    void SetMapType(ModelConfig::MapType type)
    {
        m_mapType = type;
    }

    ModelConfig::MapType GetMapType() const
    {
        return m_mapType;
    }
    
    float GetCurrentSpeed() const
    {
        if (m_isTackling)
        {
            return GetTackleSpeed();
        }
        
        if (m_isPouncing)
        {
            return GetPounceSpeed();
        }
        
        return (m_stunTimer > 0) ? 0.0f : m_speed;
    }

    // 直前フレームからの実移動スピード（水平面距離 / frame）
    float GetActualMoveSpeed() const
    {
        return MathHelper::DistanceXZ(m_pos, m_prevPos);
    }

    // 現在の猫の身長（メートル換算：初期1.80m、パンプアップで拡大）
    float GetCurrentHeightMeters() const
    {
        float currentScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(GetEffectiveRep()) * 0.0025f;
        return 1.8f * (currentScale / ModelConfig::CAT_MODEL_SCALE);
    }

    int GetEffectiveRep() const
    {
        return (m_repCount > MAX_EFFECTIVE_REP) ? MAX_EFFECTIVE_REP : m_repCount;
    }

    bool IsSkillChecking() const
    {
        return m_isSkillChecking;
    }

    enum class TitleWorkoutType
    {
        Idle = 0,
        Squat,
        PushUp
    };

    void UpdateTitleAnimation(TitleWorkoutType workoutType)
    {
        m_animFrame++;
        m_animTime += 1.0f / 60.0f;
        m_isTitleSquat = (workoutType == TitleWorkoutType::Squat);
        m_isTitlePushUp = (workoutType == TitleWorkoutType::PushUp);
        m_isSkillChecking = false;
        m_isPouncing = false;
        m_isTackling = false;
        m_stunTimer = 0;
        m_sorenessTimer = 0;
        if (m_state == MuscleState::Soreness)
        {
            m_state = (m_repCount > 0) ? MuscleState::Muscular : MuscleState::Normal;
        }
        m_pos.y = m_groundY;
        m_trails.clear();
        UpdateAnimation(false);
    }

    void UpdateTitleAnimation(bool isSquatting)
    {
        UpdateTitleAnimation(isSquatting ? TitleWorkoutType::Squat : TitleWorkoutType::Idle);
    }

    // ネズミ捕獲時の祝祭リアクション（パンプ維持リフレッシュ＆祝祭オーラ）
    void OnCatchMouse(bool isFast, int comboCount)
    {
        m_catchCelebrateTimer = 35;
        if (m_repCount > 0)
        {
            m_pumpDecayTimer = PUMP_DECAY_FRAMES; // 捕獲でパンプ維持タイマー全快リセット！
        }
    }
    
    int GetRepCount() const
    {
        return m_repCount;
    }
    
    float GetSorenessRemainingSeconds() const
    {
        return static_cast<float>(m_sorenessTimer) / 60.0f;
    }
    
    float GetPumpDecayRemainingSeconds() const
    {
        return static_cast<float>(m_pumpDecayTimer) / 60.0f;
    }

    // 飛びつきパラメータ動的算出（能力値は最大Lv15で頭打ち）
    float GetPounceSpeed() const
    {
        int eff = GetEffectiveRep();

        if (eff <= 4)
        {
            return 11.0f;
        }

        if (eff == 5)
        {
            return 13.5f;
        }

        if (eff == 6)
        {
            return 16.0f;
        }

        return 18.0f + static_cast<float>(eff - 7) * 1.5f;
    }

    int GetPounceDuration() const
    {
        int eff = GetEffectiveRep();

        if (eff <= 4)
        {
            return 12;
        }

        if (eff == 5)
        {
            return 14;
        }

        if (eff == 6)
        {
            return 16;
        }

        return 18;
    }

    int GetPounceCooldownMax() const
    {
        int eff = GetEffectiveRep();

        if (eff <= 4)
        {
            return 72; // 1.2秒
        }

        if (eff == 5)
        {
            return 60; // 1.0秒
        }

        if (eff == 6)
        {
            return 54; // 0.9秒
        }

        return 48; // 0.8秒
    }

    float GetPounceRadius() const
    {
        int eff = GetEffectiveRep();

        if (eff <= 4)
        {
            return 26.0f; // 拡大 (18.0f -> 26.0f)
        }

        if (eff == 5)
        {
            return 30.0f; // 拡大 (21.0f -> 30.0f)
        }

        if (eff == 6)
        {
            return 35.0f; // 拡大 (24.0f -> 35.0f)
        }

        return 42.0f;     // 拡大 (28.0f -> 42.0f)
    }

    // タックル（Eキー）パラメータ動的算出（4 Rep以上で使用可能、Repで強化、最大Lv15で頭打ち）
    float GetTackleSpeed() const
    {
        return 10.5f + static_cast<float>(GetEffectiveRep()) * 1.0f;
    }

    int GetTackleDuration() const
    {
        return 18 + GetEffectiveRep() * 2;
    }

    int GetTackleCooldownMax() const
    {
        int cd = 90 - GetEffectiveRep() * 3;
        return (cd < 45) ? 45 : cd;
    }

    float GetTackleRadius() const
    {
        return 32.0f + static_cast<float>(GetEffectiveRep()) * 2.5f; // 拡大 (22.0f -> 32.0f)
    }

    int GetMouseStunDuration() const
    {
        return 120 + GetEffectiveRep() * 25;
    }

    float GetRadius() const override
    {
        if (m_isTackling)
        {
            return GetTackleRadius();
        }
        
        if (m_isPouncing)
        {
            return GetPounceRadius();
        }
        
        return m_radius;
    }

    // 壁や障害物との物理的な当たり判定用身体半径（タックルの巨大攻撃範囲とは分離、通路引っかかり防止）
    float GetBodyRadius() const
    {
        return 18.0f + static_cast<float>(GetEffectiveRep()) * 0.3f;
    }


    bool IsPouncing() const
    {
        return m_isPouncing;
    }
    
    bool CanPounce() const
    {
        if (m_cheatNoCooldown)
        {
            return !m_isPouncing && !m_isTackling;
        }
        
        return (m_repCount >= REQUIRED_ATTACK_REP) && (m_state != MuscleState::Soreness) && (m_stunTimer <= 0) && !m_isSkillChecking && !m_isTackling && (m_pounceCooldown <= 0);
    }
    
    int GetPounceCooldown() const
    {
        return m_pounceCooldown;
    }

    float GetPounceCooldownRatio() const
    {
        if (m_cheatNoCooldown)
        {
            return 0.0f;
        }
        
        int maxCd = GetPounceCooldownMax();
        return (m_pounceCooldown > 0) ? (static_cast<float>(m_pounceCooldown) / static_cast<float>(maxCd)) : 0.0f;
    }

    // タックル状態確認
    bool IsTackling() const
    {
        return m_isTackling;
    }
    
    bool CanTackle() const
    {
        if (m_cheatNoCooldown)
        {
            return !m_isTackling && !m_isPouncing;
        }
        
        return (m_repCount >= REQUIRED_ATTACK_REP) && (m_state != MuscleState::Soreness) && (m_stunTimer <= 0) && !m_isSkillChecking && !m_isPouncing && !m_isTackling && (m_tackleCooldown <= 0);
    }
    
    int GetTackleCooldown() const
    {
        return m_tackleCooldown;
    }

    bool IsAttackUnlocked() const
    {
        return (m_repCount >= REQUIRED_ATTACK_REP);
    }

    float GetTackleCooldownRatio() const
    {
        if (m_cheatNoCooldown)
        {
            return 0.0f;
        }
        
        int maxCd = GetTackleCooldownMax();
        return (m_tackleCooldown > 0) ? (static_cast<float>(m_tackleCooldown) / static_cast<float>(maxCd)) : 0.0f;
    }

    // 猫のスタン状態確認・適用
    bool IsStunned() const
    {
        return m_stunTimer > 0;
    }
    
    void ApplyStun(int frames)
    {
        if (m_cheatNoCooldown)
        {
            return; // クールダウン無効中はスタンしない
        }
        
        if (frames > m_stunTimer)
        {
            m_stunTimer = frames;
        }
        
        m_isTackling = false;
        m_isPouncing = false;
    }
    
    float GetCatStunRemainingSeconds() const
    {
        return static_cast<float>(m_stunTimer) / 60.0f;
    }

    // スタンエフェクト描画
    void DrawStunEffect();
};