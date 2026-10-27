// ============================================================================
// @file   Player3D.h
// @brief  プレイヤー操作キャラクター（マッスルねこ）の制御・アクション・描画クラス定義
// ============================================================================
#pragma once
#include "GameObject3D.h"
#include "ModelConfig.h"
#include <deque>
#include <vector>

// ============================================================================
// プレイヤーの筋肉状態列挙型
// ============================================================================
enum class MuscleState
{
    Normal,     // 通常状態 (0 Rep: 基本速度・通常体格)
    Muscular,   // パンプアップ状態 (1 Rep以上: 体格巨大化・速度上昇・Lv.4以上で攻撃スキル解放)
    Soreness    // 筋肉痛状態 (筋トレQTE失敗時のペナルティ: 速度 0.0、5秒間完全移動不可)
};

// ============================================================================
// 3D残像トレイル用構造体（飛びつき・スタン攻撃の高速移動時に背後に残る半透明残像）
// ============================================================================
struct TrailPoint3D
{
    VECTOR pos;      // 残像発生時のワールド座標
    float  rotY;     // 残像発生時のY軸回転角
    int    repCount; // 残像発生時の筋トレレベル（スケール算出用）
    int    alpha;    // 現在の不透明度（0〜255、毎フレーム減衰）
};

class Camera3D;

// ============================================================================
// Player3D クラス（GameObject3D 派生）
// TPSカメラ基準のWASD移動、タイミングゲージ制の筋トレQTE（スクワット＆バーベル保持）、
// レベル（Rep）に応じた巨大化＆加速、15秒放置による筋肉減衰、Lv.4以上で解放される
// 2種類の必殺アクション（飛びつき跳躍捕獲 / 電撃スタン突進）を一括管理する
// ============================================================================
class Player3D : public GameObject3D
{
public:
    // ------------------------------------------------------------------------
    // ゲームバランス定数定義
    // ------------------------------------------------------------------------
    static constexpr float SPEED_INITIAL       = 4.0f; // デフォルト基本移動速度 (Statu.csv: Cat base_speed)
    static constexpr float SPEED_SORENESS      = 0.0f; // 筋肉痛時の移動速度（完全停止）
    static constexpr int   MAX_EFFECTIVE_REP   = 15;   // ステータス・体格に反映される最大レベル上限 (Lv.15)
    static constexpr int   PUMP_DECAY_FRAMES   = 900;  // パンプ維持時間 (15秒 = 900フレーム、時間切れで -3 Rep)
    static constexpr int   REQUIRED_ATTACK_REP = 4;    // 飛びつき・スタン攻撃解放に必要な最低レベル (Lv.4)

    // Statu.csv の設定値（base_speed, speed_growth_per_unit, max_speed）から実効レベルに応じた速度を算出
    float CalculateSpeedForRep(int effectiveRep) const;

private:
    // ------------------------------------------------------------------------
    // 基本移動・物理・状態管理メンバ
    // ------------------------------------------------------------------------
    float                m_speed          = SPEED_INITIAL;               // 現在の通常移動スピード
    MuscleState          m_state          = MuscleState::Normal;         // 現在の筋肉状態
    int                  m_sorenessTimer  = 0;                           // 筋肉痛残りフレーム (5秒 = 300F)
    int                  m_pumpDecayTimer = 0;                           // パンプ維持残りフレーム (15秒 = 900F)
    float                m_groundY        = 0.0f;                        // 現在位置の接地床・坂道高さ
    VECTOR               m_prevPos        = VGet(0.0f, 0.0f, 0.0f);      // 前フレームの座標（崖落ち防止・実速度計測用）
    ModelConfig::MapType m_mapType        = ModelConfig::MapType::SlopeHills; // 現在プレイ中のマップ種別
    int                  m_animFrame      = 0;                           // 累積フレームカウンタ
    int                  m_footstepTimer  = 0;                           // 猫の走行足音インターバルタイマー
    float                m_animTime       = 0.0f;                        // 累積経過秒数（オーラ・湯気周期用）

    // ------------------------------------------------------------------------
    // 筋トレQTE（スキルチェック）＆ レベル（Rep）管理メンバ
    // ------------------------------------------------------------------------
    bool  m_isSkillChecking    = false; // 現在筋トレQTE中かどうか
    bool  m_isTitleSquat       = false; // タイトル/リザルト画面でのスクワット演出フラグ
    bool  m_isTitlePushUp      = false; // タイトル画面での腕立て伏せ（プッシュアップ）演出フラグ
    float m_scCursor           = 0.0f;  // QTEゲージ内の針の現在位置 (0.0f 〜 1.0f)
    float m_scCursorDir        = 1.0f;  // 針の往復移動方向 (+1.0f: 右進, -1.0f: 左進)
    float m_scZoneStart        = 0.55f; // 成功ゾーンの開始位置 (0.0f 〜 1.0f)
    float m_scZoneEnd          = 0.85f; // 成功ゾーンの終了位置 (0.0f 〜 1.0f)
    float m_prevScZoneCenter   = -1.0f; // 前回の成功ゾーン中心位置（連続で同じ場所にならないよう分散）
    int   m_scHoldingFrames    = 0;     // スペースキー長押し継続フレーム数（15秒超過で筋疲労失敗）
    int   m_repCount           = 0;     // 現在の筋トレレベル（Rep数）
    bool  m_prevTriggerKey     = false; // 前フレームの筋トレキー押下状態（エッジ検出用）
    int   m_resultShowTimer    = 0;     // 成功/失敗/減衰テキストの表示残りフレーム
    bool  m_lastResultSuccess  = false; // 直前の筋トレQTEが成功したか
    bool  m_wasDecayed         = false; // 直前の通知が15秒放置減衰によるものか
    int   m_scStoppedTimer     = 0;     // キーを離した瞬間に針をピタッと止めて見せる演出タイマー
    int   m_needRepNoticeTimer = 0;     // Lv.4未満で攻撃ボタンを押した際のガイド表示タイマー

    // ------------------------------------------------------------------------
    // 飛びつき攻撃（Pounce：左クリック / SHIFT）関連メンバ
    // ------------------------------------------------------------------------
    bool                     m_isPouncing     = false;                  // 飛びつき跳躍中フラグ
    int                      m_pounceTimer    = 0;                      // 飛びつき残りフレーム
    int                      m_pounceCooldown = 0;                      // 飛びつき再使用クールタイム残りフレーム
    VECTOR                   m_pounceDir      = VGet(0.0f, 0.0f, 1.0f); // 飛びつき進行方向単位ベクトル
    bool                     m_prevPounceKey  = false;                  // 前フレームの飛びつき入力状態
    std::deque<TrailPoint3D> m_trails;                                  // 高速移動時の3D残像リスト

    // ------------------------------------------------------------------------
    // スタン攻撃・タックル（Tackle：右クリック / E）関連メンバ
    // ------------------------------------------------------------------------
    bool   m_isTackling     = false;                  // タックル突進中フラグ
    int    m_tackleTimer    = 0;                      // タックル残りフレーム
    int    m_tackleCooldown = 0;                      // タックル再使用クールタイム残りフレーム
    VECTOR m_tackleDir      = VGet(0.0f, 0.0f, 1.0f); // タックル進行方向単位ベクトル
    bool   m_prevTackleKey  = false;                  // 前フレームのタックル入力状態

    // ------------------------------------------------------------------------
    // スタン（タックル中の壁・家具激突ペナルティ）＆ 祝祭演出メンバ
    // ------------------------------------------------------------------------
    int  m_stunTimer           = 0;     // 猫自身の気絶残りフレーム数（壁激突時に90F気絶）
    int  m_catchCelebrateTimer = 0;     // ネズミ捕獲直後のオーラ活性化フレーム数

    // デバッグチート用フラグ（F1デバッグメニューの[5]キーでクールタイム＆制限解除）
    bool m_cheatNoCooldown     = false;

    // ------------------------------------------------------------------------
    // 3Dモデル・ボーンアニメーション制御メンバ（cat.mv1 & cat_soreness.mv1 & Barbell.mv1）
    // ------------------------------------------------------------------------
    int   m_modelHandle          = -1;    // プレイヤー3Dモデル複製ハンドル
    int   m_sorenessModelHandle  = -1;    // 筋肉痛（06_MuscleSoreness）モーション保持用補助モデルハンドル
    int   m_attachAnimIndex      = -1;    // 現在アタッチ中のアニメーション番号
    int   m_currentAnimIndex     = -1;    // 現在再生中のアニメーション種別インデックス
    int   m_currentAnimSrcHandle = -1;    // 現在アタッチ元のモデルハンドル（-1: 自身のモデル）
    float m_animPlayTime         = 0.0f;  // 現在のアニメーション再生時刻

    // cat.mv1 / cat_soreness.mv1 内の各モーションインデックス（InitModel で名前検索して自動取得）
    int   m_animIndexRun         = 0;     // 01_Run: 走行モーション
    int   m_animIndexAttack      = 1;     // 02_Attack: 飛びつき跳躍モーション
    int   m_animIndexSquat       = 2;     // 03_squat: スクワット（筋トレQTE）モーション
    int   m_animIndexIdol        = 3;     // 04_Idol: 待機モーション
    int   m_animIndexPush        = 4;     // 05_Push / Attack: タックル突進モーション
    int   m_animIndexPushUp      = 4;     // 05_PushUp: 腕立て伏せモーション（タイトル演出用）
    int   m_animIndexSoreness    = 5;     // 06_MuscleSoreness: 筋肉痛モーション
    bool  m_pushFromSubModel     = false; // Pushアニメーションを補助モデルから参照するか
    bool  m_sorenessFromSubModel = false; // 筋肉痛アニメーションを補助モデルから参照するか

    // バーベルを両手に追従させるためのMixamoボーンフレーム番号キャッシュ
    int   m_lHandFrame         = -1;    // 左手首ボーン (mixamorig:LeftHand)
    int   m_rHandFrame         = -1;    // 右手首ボーン (mixamorig:RightHand)
    int   m_lHandKnuckleFrame  = -1;    // 左中指付け根ボーン (mixamorig:LeftHandMiddle1)
    int   m_rHandKnuckleFrame  = -1;    // 右中指付け根ボーン (mixamorig:RightHandMiddle1)

    // スクワット中に両手に持つバーベルの3Dモデルハンドル
    int   m_barbellModelHandle = -1;

    // 内部ヘルパーメソッド
    void InitModel();                   // モデル・アニメーション・ボーンフレームの初期化
    void UpdateAnimation(bool isMoving);// 現在の行動状態に応じたアニメーション切り替え・再生進行
    void DrawBarbell(float catScale);   // 両手ボーン座標から変換行列を構築してバーベルを描画

public:
    Player3D(const VECTOR& pos);
    virtual ~Player3D();

    // 毎フレームの更新・描画
    void Update() override;
    void UpdateWithCamera(Camera3D& camera, const std::vector<VECTOR>& targetPositions = {});
    void Draw3D() override;
    void Draw2D() override;

    // ------------------------------------------------------------------------
    // デバッグチート・レベル加算操作
    // ------------------------------------------------------------------------
    void AddRep(int amount = 1);
    void SetNoCooldown(bool enabled) { m_cheatNoCooldown = enabled; }
    bool IsNoCooldown() const        { return m_cheatNoCooldown; }
    void ToggleNoCooldown()          { m_cheatNoCooldown = !m_cheatNoCooldown; }

    // ------------------------------------------------------------------------
    // 状態・座標・速度ゲッター＆セッター
    // ------------------------------------------------------------------------
    MuscleState GetMuscleState() const { return m_state; }

    void SetGroundY(float groundY)
    {
        m_groundY = groundY;
        if (!m_isPouncing)
        {
            m_pos.y = m_groundY;
        }
    }

    float                GetGroundY() const                 { return m_groundY; }
    const VECTOR&        GetPrevPos() const                 { return m_prevPos; }
    void                 SetMapType(ModelConfig::MapType t) { m_mapType = t; }
    ModelConfig::MapType GetMapType() const                 { return m_mapType; }

    // 現在の設定移動速度（タックル・飛びつき中はそれぞれの突進速度、スタン中は 0.0f）
    float GetCurrentSpeed() const
    {
        if (m_isTackling) return GetTackleSpeed();
        if (m_isPouncing) return GetPounceSpeed();
        return (m_stunTimer > 0) ? 0.0f : m_speed;
    }

    // 前フレームから実際に移動したXZ平面距離（単位/フレーム）
    float GetActualMoveSpeed() const
    {
        return MathHelper::DistanceXZ(m_pos, m_prevPos);
    }

    // 現在の猫の身長（メートル換算：初期1.80m、筋トレレベルに応じて拡大）
    float GetCurrentHeightMeters() const
    {
        float currentScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(GetEffectiveRep()) * 0.0025f;
        return 1.8f * (currentScale / ModelConfig::CAT_MODEL_SCALE);
    }

    // ステータス計算用の実効レベル（最大 MAX_EFFECTIVE_REP = 15 で頭打ち）
    int GetEffectiveRep() const
    {
        return (m_repCount > MAX_EFFECTIVE_REP) ? MAX_EFFECTIVE_REP : m_repCount;
    }

    bool IsSkillChecking() const { return m_isSkillChecking; }

    // ------------------------------------------------------------------------
    // タイトル画面・リザルト画面用の自動トレーニングアニメーション制御
    // ------------------------------------------------------------------------
    enum class TitleWorkoutType
    {
        Idle = 0, // 待機・ポージング
        Squat,    // バーベルスクワット
        PushUp    // 腕立て伏せ（プッシュアップ）
    };

    void UpdateTitleAnimation(TitleWorkoutType workoutType)
    {
        m_animFrame++;
        m_animTime += 1.0f / 60.0f;
        m_isTitleSquat    = (workoutType == TitleWorkoutType::Squat);
        m_isTitlePushUp   = (workoutType == TitleWorkoutType::PushUp);
        m_isSkillChecking = false;
        m_isPouncing      = false;
        m_isTackling      = false;
        m_stunTimer       = 0;
        m_sorenessTimer   = 0;
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

    // ネズミ捕獲成功時のリアクション（パンプ維持タイマーを15秒に全快リセット）
    void OnCatchMouse(bool isFast, int comboCount)
    {
        m_catchCelebrateTimer = 35;
        if (m_repCount > 0)
        {
            m_pumpDecayTimer = PUMP_DECAY_FRAMES;
        }
    }

    int   GetRepCount() const                 { return m_repCount; }
    float GetSorenessRemainingSeconds() const { return static_cast<float>(m_sorenessTimer) / 60.0f; }
    float GetPumpDecayRemainingSeconds() const{ return static_cast<float>(m_pumpDecayTimer) / 60.0f; }

    // ------------------------------------------------------------------------
    // 飛びつき攻撃（Pounce）のレベル別パラメータ算出
    // ------------------------------------------------------------------------
    float GetPounceSpeed() const
    {
        int eff = GetEffectiveRep();
        if (eff <= 4) return 11.0f;
        if (eff == 5) return 13.5f;
        if (eff == 6) return 16.0f;
        return 18.0f + static_cast<float>(eff - 7) * 1.5f;
    }

    int GetPounceDuration() const
    {
        int eff = GetEffectiveRep();
        if (eff <= 4) return 12;
        if (eff == 5) return 14;
        if (eff == 6) return 16;
        return 18;
    }

    int GetPounceCooldownMax() const
    {
        int eff = GetEffectiveRep();
        if (eff <= 4) return 72; // 1.2秒
        if (eff == 5) return 60; // 1.0秒
        if (eff == 6) return 54; // 0.9秒
        return 48;               // 0.8秒
    }

    float GetPounceRadius() const
    {
        int eff = GetEffectiveRep();
        if (eff <= 4) return 26.0f;
        if (eff == 5) return 30.0f;
        if (eff == 6) return 35.0f;
        return 42.0f;
    }

    // ------------------------------------------------------------------------
    // スタン攻撃・タックル（Tackle）のレベル別パラメータ算出
    // ------------------------------------------------------------------------
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
        return 32.0f + static_cast<float>(GetEffectiveRep()) * 2.5f;
    }

    int GetMouseStunDuration() const
    {
        return 120 + GetEffectiveRep() * 25;
    }

    // ネズミとの接触判定用半径（スキル発動中はそれぞれの拡大攻撃半径を返す）
    float GetRadius() const override
    {
        if (m_isTackling) return GetTackleRadius();
        if (m_isPouncing) return GetPounceRadius();
        return m_radius;
    }

    // ステージ壁や家具との物理押し出し用の身体半径（攻撃判定半径と分離して通路での引っかかりを防止）
    float GetBodyRadius() const
    {
        return 18.0f + static_cast<float>(GetEffectiveRep()) * 0.3f;
    }

    // ------------------------------------------------------------------------
    // スキル発動可能判定・クールタイム取得
    // ------------------------------------------------------------------------
    bool IsPouncing() const { return m_isPouncing; }

    bool CanPounce() const
    {
        if (m_cheatNoCooldown)
        {
            return !m_isPouncing && !m_isTackling;
        }
        return (m_repCount >= REQUIRED_ATTACK_REP) &&
               (m_state != MuscleState::Soreness) &&
               (m_stunTimer <= 0) &&
               !m_isSkillChecking &&
               !m_isTackling &&
               (m_pounceCooldown <= 0);
    }

    int GetPounceCooldown() const { return m_pounceCooldown; }

    float GetPounceCooldownRatio() const
    {
        if (m_cheatNoCooldown) return 0.0f;
        int maxCd = GetPounceCooldownMax();
        return (m_pounceCooldown > 0) ? (static_cast<float>(m_pounceCooldown) / static_cast<float>(maxCd)) : 0.0f;
    }

    bool IsTackling() const { return m_isTackling; }

    bool CanTackle() const
    {
        if (m_cheatNoCooldown)
        {
            return !m_isTackling && !m_isPouncing;
        }
        return (m_repCount >= REQUIRED_ATTACK_REP) &&
               (m_state != MuscleState::Soreness) &&
               (m_stunTimer <= 0) &&
               !m_isSkillChecking &&
               !m_isPouncing &&
               !m_isTackling &&
               (m_tackleCooldown <= 0);
    }

    int  GetTackleCooldown() const { return m_tackleCooldown; }
    bool IsAttackUnlocked() const  { return (m_repCount >= REQUIRED_ATTACK_REP); }

    float GetTackleCooldownRatio() const
    {
        if (m_cheatNoCooldown) return 0.0f;
        int maxCd = GetTackleCooldownMax();
        return (m_tackleCooldown > 0) ? (static_cast<float>(m_tackleCooldown) / static_cast<float>(maxCd)) : 0.0f;
    }

    // ------------------------------------------------------------------------
    // 猫自身のスタン（壁激突ペナルティ）状態管理
    // ------------------------------------------------------------------------
    bool IsStunned() const { return m_stunTimer > 0; }

    void ApplyStun(int frames)
    {
        if (m_cheatNoCooldown) return;
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

    // 3D頭上気絶エフェクト（回転する星とリング）の描画
    void DrawStunEffect();
};
