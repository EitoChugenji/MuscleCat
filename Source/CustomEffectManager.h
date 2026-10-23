#pragma once

#include "DxLib.h"
#include <vector>

// CustomEffectManager クラス (シングルトン)
// Effekseerとは独立して、DxLibの3Dポリゴン・ビルボード・プリミティブを用いた
// プレイヤー筋トレ中の「燃え上がる炎エフェクト」「パンプ成功時の炎爆発演出」
// 「高レベル時の体から立ち昇る湯気（蒸気）エフェクト」および「飛びつき攻撃の地面えぐり演出」を一括管理
class CustomEffectManager
{
private:
    // パーティクル種別
    enum class FlameParticleType
    {
        FlameTongue, // 立ち昇る炎の舌（テーパー付き3Dクアッド）
        FirePillar,  // 身体の周囲を渦巻きながら立ち昇る火柱リボン
        EmberSpark,  // 舞い散る火の粉・スパーク
        CoreGlow,    // 身体中心・足元の熱気グロー
        BodySmoke,   // Lv.10以降に過熱した体から立ち昇る白い湯気（蒸気）
        GougeDust,   // 飛びつき攻撃で地面をえぐった際に噴き上がる土煙・破片スプラッシュ
        RunDust      // 走行中・電撃疾走中に足元から巻き上がる土煙
    };

    // 3D炎・煙パーティクル構造体
    struct FlameParticle
    {
        FlameParticleType type = FlameParticleType::FlameTongue;
        VECTOR pos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR vel = VGet(0.0f, 0.0f, 0.0f);
        float  baseAngle = 0.0f;       // 猫中心からの周回角度
        float  angularSpeed = 0.0f;    // 渦巻き旋回速度
        float  orbitRadius = 0.0f;     // 猫中心からの半径
        float  width = 10.0f;          // 横幅
        float  height = 20.0f;         // 縦の長さ
        float  age = 0.0f;             // 経過フレーム
        float  maxLife = 30.0f;        // 寿命フレーム
        float  phase = 0.0f;           // 揺らぎ位相
        int    colorVariant = 0;       // 色バリエーション
        int    spawnRepLevel = 0;      // 発生時の筋トレレベル（レベルに応じた火の濃さ計算用）
        bool   followPlayerXZ = true;  // プレイヤーのXZ座標に追従するか
    };

    // 足元・空間に広がる火炎衝撃波リング構造体
    struct FireShockRing
    {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        float  radius = 10.0f;
        float  maxRadius = 100.0f;
        float  height = 8.0f;
        float  riseSpeed = 0.0f;
        float  age = 0.0f;
        float  maxLife = 24.0f;
        int    colorVariant = 0;
        int    repLevel = 0;
    };

    // 飛びつき攻撃で地面がえぐれた爪痕・亀裂・めくれ上がった3D岩塊セグメント構造体
    struct GroundGougeSegment
    {
        VECTOR startPos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR endPos   = VGet(0.0f, 0.0f, 0.0f);
        VECTOR dir      = VGet(0.0f, 0.0f, 1.0f);
        VECTOR right    = VGet(1.0f, 0.0f, 0.0f);
        float  width    = 36.0f;       // えぐれ幅
        float  ridgeHeight = 14.0f;    // 両脇にめくれ上がる3D岩塊・土塊の高さ
        float  age      = 0.0f;
        float  maxLife  = 65.0f;       // 約1秒間地面に爪痕と隆起が残る
        float  seed     = 0.0f;
        int    repLevel = 4;           // 発動時の筋トレレベル（派手さスケーリング用）
        bool   isCrater = false;       // 踏み切り・着地地点の放射状クレーターか
    };

    // 地面をえぐった際に空中へ飛び散り、重力で落下・バウンドする3D岩片・破片構造体
    struct GroundDebrisChunk
    {
        VECTOR pos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR vel = VGet(0.0f, 0.0f, 0.0f);
        float  groundY = 0.0f;
        float  size = 5.0f;
        float  rotAngle = 0.0f;
        float  rotSpeed = 0.2f;
        float  tiltAngle = 0.0f;
        float  age = 0.0f;
        float  maxLife = 36.0f;
        int    colorVariant = 0;
    };

    // スタン攻撃（電撃疾走）で突進軸に垂直に広がる「電撃ソニックブーム衝撃波リング」構造体
    struct SpaceTimeSlashArc
    {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        VECTOR planeU = VGet(1.0f, 0.0f, 0.0f); // リング平面の横軸（Right）
        VECTOR planeV = VGet(0.0f, 1.0f, 0.0f); // リング平面の縦軸（Up）
        VECTOR driftVel = VGet(0.0f, 0.0f, 0.0f); // 後方への気流ドリフト速度
        float  radius = 24.0f;
        float  maxRadius = 78.0f;
        float  bladeWidth = 14.0f;
        float  arcSpan = 6.283185f;
        float  age = 0.0f;
        float  maxLife = 16.0f;
        int    colorVariant = 0;
    };

    // スタン攻撃（電撃疾走）で身体の周囲〜後方へ猛スピードで流れる「3D電撃スピードライン（疾走風圧線）」構造体
    struct SpaceTimeRift
    {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        VECTOR mainAxis = VGet(0.0f, 0.0f, -1.0f); // スピードラインの伸びる方向（進行方向の逆＝後方）
        VECTOR sideAxis = VGet(1.0f, 0.0f, 0.0f);  // リボン幅方向
        VECTOR vel = VGet(0.0f, 0.0f, 0.0f);       // 後方へ高速で流れる気流速度
        float  length = 85.0f;
        float  maxOpenWidth = 8.5f;
        float  age = 0.0f;
        float  maxLife = 18.0f;
        float  seed = 0.0f;
        int    colorVariant = 0;
    };

    // 電撃疾走の気流に乗って後方へ弾け飛ぶ「高圧電撃スパーク結晶」構造体
    struct SpaceTimeShard
    {
        VECTOR pos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR vel = VGet(0.0f, 0.0f, 0.0f);
        float  size = 7.0f;
        float  rotAngle = 0.0f;
        float  rotSpeed = 0.15f;
        float  tiltAngle = 0.0f;
        float  age = 0.0f;
        float  maxLife = 24.0f;
        int    colorVariant = 0;
    };

    // スタン攻撃（電撃疾走）で全身に纏い、後方へ激しく尾を引く「3D稲妻・雷撃（ThunderBoltArc）」構造体
    struct ThunderBoltArc
    {
        VECTOR startPos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR endPos   = VGet(0.0f, 0.0f, 0.0f);
        VECTOR perpA    = VGet(1.0f, 0.0f, 0.0f);
        VECTOR perpB    = VGet(0.0f, 1.0f, 0.0f);
        float  thickness = 5.5f;
        float  jaggedAmp = 16.0f;
        float  age       = 0.0f;
        float  maxLife   = 14.0f;
        float  seed      = 0.0f;
        bool   hasBranch = true;
    };

    // 捕獲時の祝祭3Dキラキラ星・ダイヤ紙吹雪パーティクル
    struct CatchStarParticle
    {
        VECTOR pos = VGet(0.0f, 0.0f, 0.0f);
        VECTOR vel = VGet(0.0f, 0.0f, 0.0f);
        float  size = 5.0f;
        float  rot = 0.0f;
        float  rotSpeed = 0.2f;
        float  age = 0.0f;
        float  maxLife = 42.0f;
        int    colorVariant = 0; // 0:Gold, 1:Cyan, 2:Pink, 3:White, 4:Lime
        int    shapeType = 0;    // 0:CrossStar, 1:Diamond
    };

    // 捕獲地点の地面に広がる祝祭ゴールド衝撃波リング
    struct CatchShockwaveRing
    {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        float  radius = 4.0f;
        float  maxRadius = 65.0f;
        float  age = 0.0f;
        float  maxLife = 22.0f;
        bool   isFast = false;
        int    combo = 1;
    };

    // 捕獲瞬間に周囲へ放たれる3D放射状サンバースト光線
    struct CatchSunburstRay
    {
        VECTOR center = VGet(0.0f, 0.0f, 0.0f);
        VECTOR dir = VGet(0.0f, 1.0f, 0.0f);
        float  length = 15.0f;
        float  maxLength = 70.0f;
        float  age = 0.0f;
        float  maxLife = 16.0f;
        int    colorVariant = 0;
    };

    // 捕獲されたネズミのコミカル昇天ソウル（ミニ天使ネズミ・光輪＆スター）
    struct CatchAscendingSoul
    {
        VECTOR pos = VGet(0.0f, 0.0f, 0.0f);
        float  age = 0.0f;
        float  maxLife = 50.0f;
        float  swayPhase = 0.0f;
        bool   isFast = false;
    };

    std::vector<FlameParticle>      m_particles;
    std::vector<FireShockRing>      m_rings;
    std::vector<GroundGougeSegment> m_gougeSegments;
    std::vector<GroundDebrisChunk>  m_debrisChunks;
    std::vector<SpaceTimeSlashArc>  m_spaceSlashes;
    std::vector<SpaceTimeRift>      m_spaceRifts;
    std::vector<SpaceTimeShard>     m_spaceShards;
    std::vector<ThunderBoltArc>     m_thunderBolts;
    std::vector<CatchStarParticle>  m_catchStars;
    std::vector<CatchShockwaveRing> m_catchRings;
    std::vector<CatchSunburstRay>   m_catchSunbursts;
    std::vector<CatchAscendingSoul> m_catchSouls;
    VECTOR m_lastGougePos = VGet(0.0f, 0.0f, 0.0f);
    bool   m_hasLastGougePos = false;
    VECTOR m_lastRiftPos  = VGet(0.0f, 0.0f, 0.0f);
    bool   m_hasLastRiftPos  = false;

    // プロシージャル生成テクスチャハンドル（外部画像ファイル非依存で確実に美しく描画）
    int m_flameTexHandle  = -1; // 炎の舌（ティアドロップ型グラデーション）
    int m_glowTexHandle   = -1; // ソフト発光グロー
    int m_streakTexHandle = -1; // 縦長の火柱ストリーク
    int m_smokeTexHandle  = -1; // もくもくと立ち昇る煙テクスチャ

    // 現在の筋トレ燃焼・煙状態
    VECTOR m_targetPos = VGet(0.0f, 0.0f, 0.0f);
    bool   m_isBurning = false;
    bool   m_isSmokeActive = false; // Lv.10超えで体から煙を出すフラグ
    int    m_currentRepCount = 0;
    float  m_chargeProgress = 0.0f; // QTEゲージ進行度 (0.0f ~ 1.0f)
    float  m_time = 0.0f;
    int    m_burstTimer = 0;        // 筋トレ成功時の爆炎ブースト残りフレーム
    int    m_runDustCounter = 0;    // 走行土煙の発生間隔カウンタ

    // エフェクト有効フラグ（デバッグ切替連動用）
    bool   m_effectEnabled = true;

    CustomEffectManager() = default;
    ~CustomEffectManager() = default;

    // プロシージャルテクスチャ生成ヘルパー
    void CreateProceduralTextures();

    // パーティクルスポーンヘルパー
    void SpawnWorkoutFlameParticles(int count, bool isBurst);
    void SpawnEmberSparks(int count, bool isBurst);
    void SpawnBodySmokeParticles(int count, bool isBurst);
    void SpawnGougeDebrisAndDust(const VECTOR& groundPos, const VECTOR& dirXZ, int debrisCount, int dustCount, float scaleBoost, bool isImpact);

    // 3Dテーパー付きビルボードクアッド描画（Y軸・速度方向基準でカメラを向く炎・煙ポリゴン）
    void DrawFlameQuad3D(
        const VECTOR& centerPos,
        const VECTOR& upDir,
        const VECTOR& camPos,
        float bottomWidth,
        float topWidth,
        float height,
        COLOR_U8 bottomColor,
        COLOR_U8 topColor,
        int grHandle
    ) const;

    // 足元の渦巻く火炎リング描画
    void DrawGroundFireVortex(const VECTOR& center, float baseRadius, float intensity) const;

    // 地面えぐり爪痕・3Dめくれ上がり隆起・飛散岩片の描画ヘルパー
    void DrawGroundGougeSegments() const;
    void DrawGroundDebrisChunks() const;

    // スタン攻撃（タックル）の時空切り裂き・雷撃・次元亀裂・次元破片の生成＆描画ヘルパー
    void SpawnSpaceTimeSlashAndRift(const VECTOR& centerPos, const VECTOR& dirXZ, int repCount, float scaleBoost, bool isCrossSlash);
    void DrawSpaceTimeEffects() const;

    // ネズミ捕獲時の祝祭エフェクト更新・描画ヘルパー
    void UpdateCatchCelebration();
    void DrawCatchCelebration() const;

public:
    static constexpr int SMOKE_START_REP_LEVEL = 10; // 体から煙が出始めるレベル閾値 (Lv.10〜)

    static CustomEffectManager& GetInstance()
    {
        static CustomEffectManager instance;
        return instance;
    }

    CustomEffectManager(const CustomEffectManager&) = delete;
    CustomEffectManager& operator=(const CustomEffectManager&) = delete;

    void Init();
    void Release();
    void Clear();
    void Update();
    void Draw3D();

    // エフェクト表示の有効/無効設定
    void SetEffectEnabled(bool enabled);
    bool IsEffectEnabled() const { return m_effectEnabled; }
    void ToggleEffectEnabled();

    // 筋トレ中（スクワット中）の燃焼エフェクトおよびLv.10超え体煙の更新
    void UpdateWorkoutFlame(
        const VECTOR& playerPos,
        bool isWorkingOut,
        int repCount,
        float chargeProgress = 0.0f,
        bool allowBodySmoke = true
    );

    // 筋トレ成功時（Rep達成時）の爆炎バースト再生
    void PlayWorkoutSuccessBurst(const VECTOR& playerPos, int repCount);

    // 飛びつき攻撃（Pounce）時の地面えぐりエフェクト（踏み切り開始・飛翔中えぐり・着地フィニッシュ）
    void StartPounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, int repCount);
    void UpdatePounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, float progress, int repCount);
    void FinishPounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, int repCount);

    // スタン攻撃（Tackle）時の時空切り裂きエフェクト（発動時の次元十字斬・突進中の空間亀裂・フィニッシュ時空崩壊）
    void StartTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int repCount);
    void UpdateTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int timer, int repCount);
    void FinishTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int repCount);

    // 走行中（通常移動・電撃疾走）に足元から土煙を巻き上げる（移動中のフレームで毎フレーム呼び出す）
    void UpdateRunDust(const VECTOR& footPos, const VECTOR& moveDirXZ, float speedRatio, bool isDashing, int repCount);

    // ネズミ逃走時のミニ足元土煙（コミカルなパニック逃走演出）
    void SpawnMouseFleeDust(const VECTOR& footPos, const VECTOR& moveDirXZ);

    // ネズミ捕獲時の3D祝祭エフェクト（星＆ダイヤ噴水、ゴールド衝撃波、サンバースト光線、昇天ソウル）
    void SpawnMouseCatchCelebration(const VECTOR& mousePos, bool isFast, int comboCount);

    // 現在燃焼中（またはバースト余韻中）かどうか
    bool IsPlayerBurning() const { return m_effectEnabled && (m_isBurning || m_burstTimer > 0); }

    // プレイヤーモデルの発光ティント用強度 (0.0f ~ 1.0f)
    float GetPlayerFlameTintIntensity() const;
};
