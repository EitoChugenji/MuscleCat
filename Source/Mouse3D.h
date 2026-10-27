// ============================================================================
// @file   Mouse3D.h
// @brief  捕獲対象キャラクター（通常ネズミ・高速逃走ネズミ）のAI・描画クラス定義
// ============================================================================
#pragma once
#include "GameObject3D.h"
#include "ModelConfig.h"
#include <memory>

class Player3D;

// ============================================================================
// ネズミのプロシージャルアニメーション姿勢・スケール構造体
// 外部ボーンアニメーションに頼らず、ホッピング跳躍・横揺れ・前傾姿勢・
// Squash & Stretch（伸縮）・スタン時の横倒し痙攣を行列合成で表現する
// ============================================================================
struct MouseAnimTransform
{
    VECTOR drawPos;      // ホッピング跳躍やスタン倒れ高さを反映した描画ワールド座標
    VECTOR scaleVec;     // Squash & Stretch（伸縮）を反映した3軸スケールベクトル
    float  rotY;         // 進行方向ヨー角（モデル補正角込み）
    float  localPitchX;  // 前傾・後傾角度（逃走時は頭を低くして前傾）
    float  localRollZ;   // 左右の足踏みロール揺れ、またはスタン気絶時の横倒し角度
    bool   isStunned;    // スタン気絶中かどうか
};

// ============================================================================
// MouseBase3D クラス（GameObject3D 派生：ネズミ共通基底クラス）
// 部屋巡回（パトロール）AI、プレイヤー接近時の逃走AI、仲間の被捕食に応じた速度成長、
// スタン（気絶）状態管理、プロシージャルアニメーション計算を共通提供する
// ============================================================================
class MouseBase3D : public GameObject3D
{
protected:
    float  m_vx                 = 0.0f;                 // X軸方向の現在速度
    float  m_vz                 = 0.0f;                 // Z軸方向の現在速度
    float  m_baseSpeed          = 4.2f;                 // 初期移動速度（Statu.csv から取得）
    float  m_speedGrowthPerUnit = 0.05f;                // 仲間が1匹捕まるごとの速度上昇量
    float  m_maxSpeed           = 4.7f;                 // 最大速度上限
    float  m_speedBonus         = 0.0f;                 // 現在の速度ボーナス（捕獲数 × 成長量）
    float  m_animTime           = 0.0f;                 // アニメーション累積経過秒数
    float  m_walkAnimPhase      = 0.0f;                 // 歩行ホッピングのサイン波位相
    int    m_stunTimer          = 0;                    // スタン（気絶）残りフレーム数
    VECTOR m_prevPos            = VGet(0.0f, 0.0f, 0.0f); // 前フレームの座標（床外・壁抜け防止用）

    // 室内巡回（パトロール）・逃走AI用パラメータ
    VECTOR m_targetWaypoint     = VGet(0.0f, 0.0f, 0.0f); // 現在向かっている巡回目標地点（Position.csv基準）
    int    m_patrolTimer        = 0;                    // 次の巡回地点へ切り替えるまでの残りフレーム
    bool   m_isFleeingNow       = false;                // 現在プレイヤーから逃走中かどうか（漫符・土煙演出と連動）

    static constexpr float MOUSE_MAX_SPEED = 4.7f;      // デフォルト最大速度定数
    ModelConfig::MapType   m_mapType       = ModelConfig::MapType::SlopeHills; // 現在のマップ種別

public:
    MouseBase3D(
        const VECTOR& pos,
        float radius,
        ObjectType type,
        float baseSpeed,
        float speedGrowth = 0.05f,
        float maxSpeed = MOUSE_MAX_SPEED
    );
    virtual ~MouseBase3D() = default;

    // マップ種別の設定（マップに応じた巡回座標リストから目的地を再選択する）
    void SetMapType(ModelConfig::MapType mapType)
    {
        m_mapType = mapType;
        ChooseNewPatrolTarget();
    }

    ModelConfig::MapType GetMapType() const { return m_mapType; }

    // 仲間が捕まった数に応じたスピードボーナスの設定・取得
    void  SetSpeedBonus(float bonus)     { m_speedBonus = bonus; }
    float GetSpeedBonus() const          { return m_speedBonus; }
    float GetSpeedGrowthPerUnit() const  { return m_speedGrowthPerUnit; }
    float GetMaxSpeed() const            { return m_maxSpeed; }

    // 現在の実効移動速度を取得（スタン中は 0.0f、上限 m_maxSpeed でクランプ）
    virtual float GetCurrentSpeed() const
    {
        if (IsStunned())
        {
            return 0.0f;
        }

        float s = m_baseSpeed + m_speedBonus;
        return (s > m_maxSpeed) ? m_maxSpeed : s;
    }

    // ------------------------------------------------------------------------
    // スタン（気絶）状態の付与・判定（プレイヤーのスタン攻撃ヒット時に呼ばれる）
    // ------------------------------------------------------------------------
    void ApplyStun(int frames)
    {
        if (frames > m_stunTimer)
        {
            m_stunTimer = frames;
        }
    }

    bool  IsStunned() const               { return m_stunTimer > 0; }
    float GetStunRemainingSeconds() const { return static_cast<float>(m_stunTimer) / 60.0f; }

    // ------------------------------------------------------------------------
    // 捕獲判定用の有効半径（物理押し出し半径より広めに設定し、快適な捕獲手応えを実現）
    // スタン気絶中はさらに捕獲ボーナス（+5.0f）が付与される
    // ------------------------------------------------------------------------
    virtual float GetCatchRadius() const
    {
        float r = m_radius * 1.55f;
        if (IsStunned())
        {
            r += 5.0f;
        }
        return r;
    }

    // スタン中の3D頭上ピヨピヨ星＆リング描画
    void DrawStunEffect();

    // 歩行・逃走・スタン状態に応じたプロシージャルアニメーション姿勢の算出と位相更新
    MouseAnimTransform GetAnimTransform(float baseScale, float modelRotY) const;
    void UpdateAnimPhase();

    // 2D頭上スタン残り時間表示
    void Draw2D() override;

    // 壁・境界衝突時の反射および新パトロール目標の再選択
    virtual void OnWallCollision(const VECTOR& pushNormal);

    // 座標・速度ゲッター＆セッター
    const VECTOR& GetPrevPos() const        { return m_prevPos; }
    void          SetPrevPos(const VECTOR& pos) { m_prevPos = pos; }
    float         GetVelocityX() const      { return m_vx; }
    float         GetVelocityZ() const      { return m_vz; }
    void          SetVelocity(float vx, float vz) { m_vx = vx; m_vz = vz; }

    // ------------------------------------------------------------------------
    // AI移動制御メソッド群
    // ------------------------------------------------------------------------
    void ChooseNewPatrolTarget();                                                                   // 新しい巡回目標地点をPosition.csvから選択
    void UpdateWanderMovement(float speed);                                                         // 巡回目標地点へ向けた滑らかな操舵
    void UpdateFleeMovement(const VECTOR& playerPos, float speed);                                  // プレイヤーから遠ざかる方向への逃走操舵
    void CalculateMovementVector(const VECTOR& playerPos, float fleeDistance, float speed, bool isFast); // 距離に応じた逃走/巡回の切り替え補助
};

// ============================================================================
// NormalMouse3D クラス（通常ネズミ）
// 普段はPosition.csvの各地点を巡回し、プレイヤーが距離130以内に近づくと逃走する
// ============================================================================
class NormalMouse3D : public MouseBase3D
{
private:
    std::shared_ptr<Player3D> m_targetPlayer; // 距離判定用のプレイヤー参照
    int                       m_turnTimer = 0;

public:
    NormalMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player = nullptr);
    virtual ~NormalMouse3D() = default;

    void Update() override;
    void Draw3D() override;
    void Draw2D() override;
};

// ============================================================================
// FastMouse3D クラス（高速逃走ネズミ）
// 通常ネズミより警戒範囲が広く（距離160）、逃走スピードも約1.1倍速い上位ターゲット。
// プレイヤーが筋トレでスピードアップするか、飛びつき・スタン攻撃を活用すると捕まえやすい
// ============================================================================
class FastMouse3D : public MouseBase3D
{
private:
    std::shared_ptr<Player3D> m_targetPlayer;        // 距離判定用のプレイヤー参照
    float                     m_fleeDistance = 160.0f; // 逃走を開始するプレイヤーとの距離閾値
    float                     m_fleeSpeed    = 4.62f;  // 逃走時の基準スピード
    int                       m_wanderTimer  = 0;

public:
    FastMouse3D(const VECTOR& pos, std::shared_ptr<Player3D> player);
    virtual ~FastMouse3D() = default;

    // 逃走時の現在スピードを取得
    float GetCurrentFleeSpeed() const
    {
        if (IsStunned())
        {
            return 0.0f;
        }
        float s = m_fleeSpeed + m_speedBonus;
        return (s > m_maxSpeed) ? m_maxSpeed : s;
    }

    // 通常徘徊時の現在スピードを取得
    float GetCurrentWanderSpeed() const
    {
        if (IsStunned())
        {
            return 0.0f;
        }
        float s = (m_baseSpeed * 0.85f) + m_speedBonus * 0.5f;
        return (s > m_maxSpeed) ? m_maxSpeed : s;
    }

    void Update() override;
    void Draw3D() override;
    void Draw2D() override;
};
