// ============================================================================
// @file   CustomEffectManager.cpp
// @brief  CustomEffectManager クラスの実装（炎・煙・地面えぐり・雷撃・捕獲祝祭）
// ============================================================================
#include "CustomEffectManager.h"
#include "Common.h"
#include "ModelConfig.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace
{
    // 0.0f ~ 1.0f の乱数生成ヘルパー
    inline float Rand01()
    {
        return static_cast<float>(std::rand() % 10000) / 10000.0f;
    }

    // minVal ~ maxVal の乱数生成ヘルパー
    inline float RandRange(float minVal, float maxVal)
    {
        return minVal + (maxVal - minVal) * Rand01();
    }

    // レベル(repLevel: 0〜15+)と寿命進行度(lifeRatio: 0.0〜1.0)に応じた炎カラー算出
    // レベルが上がるごとに「明るい黄橙(Lv.0) → 灼熱オレンジ(Lv.5) → 紅蓮クリムゾン(Lv.10) → 深紅・黒赤業火(Lv.15)」へと色が濃くなる
    inline void GetFlameColors(
        float lifeRatio,
        int variant,
        int repLevel,
        unsigned char baseAlphaByte,
        COLOR_U8& outBottomColor,
        COLOR_U8& outTopColor
    )
    {
        float t = MathHelper::Clamp(lifeRatio, 0.0f, 1.0f);
        float lvRatio = MathHelper::Clamp(static_cast<float>(repLevel) / 15.0f, 0.0f, 1.0f);

        // レベルが上がるほど不透明度（濃さ）も増す (Lv.0: 0.76 -> Lv.15: 1.00)
        float densityScale = 0.76f + lvRatio * 0.24f;
        unsigned char alphaByte = static_cast<unsigned char>(MathHelper::Clamp(static_cast<float>(baseAlphaByte) * densityScale, 0.0f, 255.0f));

        // 根元(Bottom)の基本色：Lv.0は明るいゴールデンイエロー(255, 235, 130) -> Lv.15は濃厚な深紅(205, 22, 5)
        float bR_base = MathHelper::Lerp(255.0f, 205.0f, lvRatio * lvRatio);
        float bG_base = MathHelper::Lerp(235.0f, 24.0f,  std::pow(lvRatio, 0.80f));
        float bB_base = MathHelper::Lerp(130.0f, 4.0f,   std::pow(lvRatio, 0.65f));

        // 先端(Top)の基本色：Lv.0は鮮やかなオレンジ(255, 140, 35) -> Lv.15は黒みを帯びた濃赤・業火色(115, 4, 2)
        float tR_base = MathHelper::Lerp(255.0f, 115.0f, std::pow(lvRatio, 0.85f));
        float tG_base = MathHelper::Lerp(140.0f, 4.0f,   std::pow(lvRatio, 0.70f));
        float tB_base = MathHelper::Lerp(35.0f,  2.0f,   std::pow(lvRatio, 0.60f));

        // バリエーションによる色味アクセント
        if (variant == 1)
        {
            // コア寄りの少し明るい炎（ただし高レベルではしっかり濃い紅蓮にシフト）
            bG_base = MathHelper::Lerp(bG_base, bG_base + 45.0f * (1.0f - lvRatio * 0.6f), 0.7f);
            tR_base = (std::min)(255.0f, tR_base + 25.0f);
            tG_base = MathHelper::Lerp(tG_base, tG_base + 30.0f * (1.0f - lvRatio * 0.7f), 0.6f);
        }
        else if (variant == 2)
        {
            // 外周寄りの一段と濃い深紅・暗赤炎
            bR_base *= 0.90f;
            bG_base *= 0.55f;
            bB_base *= 0.40f;
            tR_base *= 0.78f;
            tG_base *= 0.35f;
            tB_base *= 0.30f;
        }

        // 寿命経過(t: 0->1)に伴ってさらに色が濃く沈み込む
        int bR = static_cast<int>(bR_base * (1.0f - t * 0.18f));
        int bG = static_cast<int>(bG_base * (1.0f - t * 0.55f));
        int bB = static_cast<int>(bB_base * (1.0f - t * 0.80f));

        int tR = static_cast<int>(tR_base * (1.0f - t * 0.25f));
        int tG = static_cast<int>(tG_base * (1.0f - t * 0.70f));
        int tB = static_cast<int>(tB_base * (1.0f - t * 0.85f));

        bR = static_cast<int>(MathHelper::Clamp(static_cast<float>(bR), 0.0f, 255.0f));
        bG = static_cast<int>(MathHelper::Clamp(static_cast<float>(bG), 0.0f, 255.0f));
        bB = static_cast<int>(MathHelper::Clamp(static_cast<float>(bB), 0.0f, 255.0f));

        tR = static_cast<int>(MathHelper::Clamp(static_cast<float>(tR), 0.0f, 255.0f));
        tG = static_cast<int>(MathHelper::Clamp(static_cast<float>(tG), 0.0f, 255.0f));
        tB = static_cast<int>(MathHelper::Clamp(static_cast<float>(tB), 0.0f, 255.0f));

        unsigned char topAlpha = static_cast<unsigned char>(static_cast<int>(alphaByte) * 90 / 100);
        outBottomColor = GetColorU8(bR, bG, bB, alphaByte);
        outTopColor    = GetColorU8(tR, tG, tB, topAlpha);
    }
}

void CustomEffectManager::Init()
{
    Release();

    m_particles.reserve(750);
    m_rings.reserve(32);
    m_gougeSegments.reserve(96);
    m_debrisChunks.reserve(256);
    m_spaceSlashes.reserve(64);
    m_spaceRifts.reserve(64);
    m_spaceShards.reserve(256);
    m_thunderBolts.reserve(192);

    CreateProceduralTextures();

    m_isBurning = false;
    m_isSmokeActive = false;
    m_currentRepCount = 0;
    m_chargeProgress = 0.0f;
    m_time = 0.0f;
    m_burstTimer = 0;
    m_hasLastGougePos = false;
    m_hasLastRiftPos  = false;
    m_effectEnabled = true;
}

void CustomEffectManager::Release()
{
    Clear();

    if (m_flameTexHandle != -1)
    {
        DeleteGraph(m_flameTexHandle);
        m_flameTexHandle = -1;
    }

    if (m_glowTexHandle != -1)
    {
        DeleteGraph(m_glowTexHandle);
        m_glowTexHandle = -1;
    }

    if (m_streakTexHandle != -1)
    {
        DeleteGraph(m_streakTexHandle);
        m_streakTexHandle = -1;
    }

    if (m_smokeTexHandle != -1)
    {
        DeleteGraph(m_smokeTexHandle);
        m_smokeTexHandle = -1;
    }
}

void CustomEffectManager::Clear()
{
    m_particles.clear();
    m_rings.clear();
    m_gougeSegments.clear();
    m_debrisChunks.clear();
    m_spaceSlashes.clear();
    m_spaceRifts.clear();
    m_spaceShards.clear();
    m_thunderBolts.clear();
    m_catchStars.clear();
    m_catchRings.clear();
    m_catchSunbursts.clear();
    m_catchSouls.clear();
    m_isBurning = false;
    m_isSmokeActive = false;
    m_burstTimer = 0;
    m_chargeProgress = 0.0f;
    m_hasLastGougePos = false;
    m_hasLastRiftPos  = false;
}

void CustomEffectManager::CreateProceduralTextures()
{
    constexpr int TEX_SIZE = 96;

    // 1. 炎の舌テクスチャ（頂点カラーのレベル別変化が100%反映されるよう、高輝度コア＋引き締まった外縁を持つ炎シルエット）
    int softFlame = MakeARGB8ColorSoftImage(TEX_SIZE, TEX_SIZE);
    if (softFlame != -1)
    {
        for (int y = 0; y < TEX_SIZE; ++y)
        {
            float v = static_cast<float>(y) / static_cast<float>(TEX_SIZE - 1);

            float widthProfile = std::sin(std::pow(v, 0.58f) * MathHelper::PI);
            if (widthProfile < 0.04f) widthProfile = 0.04f;

            float verticalFade = std::sin(v * MathHelper::PI);
            verticalFade = std::pow(verticalFade, 0.48f);

            float waveCenter = std::sin(v * MathHelper::PI * 2.4f) * 0.10f * (1.0f - v);

            for (int x = 0; x < TEX_SIZE; ++x)
            {
                float u = (static_cast<float>(x) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
                float normX = std::abs(u - waveCenter) / widthProfile;

                if (normX >= 1.0f)
                {
                    DrawPixelSoftImage(softFlame, x, y, 0, 0, 0, 0);
                    continue;
                }

                float radial = 1.0f - normX * normX;
                float heat = radial * (0.35f + 0.65f * v);
                float alpha = std::pow(radial, 0.52f) * verticalFade;

                // 頂点カラー乗算で「黄橙〜深紅〜黒赤」まで自在に発色できるよう、
                // 中心は純白(255,255,255)、外縁は引き締まった濃赤リムを持たせる
                int r = static_cast<int>(MathHelper::Clamp(175.0f + heat * 80.0f, 0.0f, 255.0f));
                int g = static_cast<int>(MathHelper::Clamp(35.0f + std::pow(heat, 0.85f) * 220.0f, 0.0f, 255.0f));
                int b = static_cast<int>(MathHelper::Clamp(15.0f + std::pow(heat, 1.15f) * 240.0f, 0.0f, 255.0f));
                int a = static_cast<int>(MathHelper::Clamp(alpha * 252.0f, 0.0f, 255.0f));

                DrawPixelSoftImage(softFlame, x, y, r, g, b, a);
            }
        }
        m_flameTexHandle = CreateGraphFromSoftImage(softFlame);
        DeleteSoftImage(softFlame);
    }

    // 2. ソフト熱気・火の粉グローテクスチャ
    int softGlow = MakeARGB8ColorSoftImage(TEX_SIZE, TEX_SIZE);
    if (softGlow != -1)
    {
        for (int y = 0; y < TEX_SIZE; ++y)
        {
            float ny = (static_cast<float>(y) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
            for (int x = 0; x < TEX_SIZE; ++x)
            {
                float nx = (static_cast<float>(x) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
                float r2 = nx * nx + ny * ny;
                if (r2 >= 1.0f)
                {
                    DrawPixelSoftImage(softGlow, x, y, 0, 0, 0, 0);
                    continue;
                }

                float t = 1.0f - r2;
                float alpha = t * t;
                int r = 255;
                int g = static_cast<int>(MathHelper::Clamp(60.0f + t * 195.0f, 0.0f, 255.0f));
                int b = static_cast<int>(MathHelper::Clamp(20.0f + t * t * 235.0f, 0.0f, 255.0f));
                int a = static_cast<int>(MathHelper::Clamp(alpha * 245.0f, 0.0f, 255.0f));
                DrawPixelSoftImage(softGlow, x, y, r, g, b, a);
            }
        }
        m_glowTexHandle = CreateGraphFromSoftImage(softGlow);
        DeleteSoftImage(softGlow);
    }

    // 3. 縦長の火柱・オーラストリークテクスチャ
    int softStreak = MakeARGB8ColorSoftImage(TEX_SIZE, TEX_SIZE);
    if (softStreak != -1)
    {
        for (int y = 0; y < TEX_SIZE; ++y)
        {
            float v = static_cast<float>(y) / static_cast<float>(TEX_SIZE - 1);
            float vFade = std::pow(std::sin(v * MathHelper::PI), 0.58f);
            for (int x = 0; x < TEX_SIZE; ++x)
            {
                float u = (static_cast<float>(x) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
                float uFade = (std::max)(0.0f, 1.0f - std::abs(u));
                uFade = std::pow(uFade, 1.40f);

                float streakWave = 0.70f + 0.30f * std::cos(u * MathHelper::PI * 4.0f);
                float heat = uFade * (0.40f + 0.60f * v);
                float alpha = vFade * uFade * streakWave;

                int r = static_cast<int>(MathHelper::Clamp(185.0f + heat * 70.0f, 0.0f, 255.0f));
                int g = static_cast<int>(MathHelper::Clamp(45.0f + heat * 210.0f, 0.0f, 255.0f));
                int b = static_cast<int>(MathHelper::Clamp(20.0f + heat * heat * 235.0f, 0.0f, 255.0f));
                int a = static_cast<int>(MathHelper::Clamp(alpha * 248.0f, 0.0f, 255.0f));
                DrawPixelSoftImage(softStreak, x, y, r, g, b, a);
            }
        }
        m_streakTexHandle = CreateGraphFromSoftImage(softStreak);
        DeleteSoftImage(softStreak);
    }

    // 4. ふわっと立ち昇る白い湯気（蒸気）パフテクスチャ（Lv.10超えの体から出る湯気用）
    int softSmoke = MakeARGB8ColorSoftImage(TEX_SIZE, TEX_SIZE);
    if (softSmoke != -1)
    {
        for (int y = 0; y < TEX_SIZE; ++y)
        {
            float ny = (static_cast<float>(y) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
            for (int x = 0; x < TEX_SIZE; ++x)
            {
                float nx = (static_cast<float>(x) / static_cast<float>(TEX_SIZE - 1)) * 2.0f - 1.0f;
                float dist = std::sqrt(nx * nx + ny * ny);
                float angle = std::atan2(ny, nx);

                // 湯気らしいふんわりとした凹凸輪郭
                float lobe = 1.0f
                    + 0.13f * std::sin(angle * 3.0f + 0.7f)
                    + 0.09f * std::cos(angle * 5.0f - 1.2f)
                    + 0.05f * std::sin(angle * 7.0f + 2.1f);
                float normDist = dist / lobe;

                if (normDist >= 1.0f)
                {
                    DrawPixelSoftImage(softSmoke, x, y, 0, 0, 0, 0);
                    continue;
                }

                float falloff = 1.0f - normDist * normDist;
                // 湯気内部の柔らかな濃淡で空気感のある蒸気を表現
                float wispy = 0.85f + 0.15f * std::sin(nx * 4.0f + std::cos(ny * 4.0f));
                float alpha = std::pow(falloff, 1.30f) * wispy;

                // 中心は純白(255,255,255)、外縁は白い床の上でも湯気の輪郭が自然に浮かぶ淡いシルバーホワイト
                float coreRatio = std::pow(falloff, 0.55f);
                int r = static_cast<int>(MathHelper::Clamp(218.0f + coreRatio * 37.0f, 0.0f, 255.0f));
                int g = static_cast<int>(MathHelper::Clamp(226.0f + coreRatio * 29.0f, 0.0f, 255.0f));
                int b = static_cast<int>(MathHelper::Clamp(236.0f + coreRatio * 19.0f, 0.0f, 255.0f));
                int a = static_cast<int>(MathHelper::Clamp(alpha * 225.0f, 0.0f, 255.0f));

                DrawPixelSoftImage(softSmoke, x, y, r, g, b, a);
            }
        }
        m_smokeTexHandle = CreateGraphFromSoftImage(softSmoke);
        DeleteSoftImage(softSmoke);
    }
}

void CustomEffectManager::SetEffectEnabled(bool enabled)
{
    m_effectEnabled = enabled;
    if (!m_effectEnabled)
    {
        Clear();
    }
}

void CustomEffectManager::ToggleEffectEnabled()
{
    SetEffectEnabled(!m_effectEnabled);
}

float CustomEffectManager::GetPlayerFlameTintIntensity() const
{
    if (!m_effectEnabled)
    {
        return 0.0f;
    }

    float lvBoost = 1.0f + MathHelper::Clamp(static_cast<float>(m_currentRepCount) / 15.0f, 0.0f, 1.0f) * 0.35f;
    float intensity = 0.0f;
    if (m_isBurning)
    {
        float pulse = 0.78f + 0.22f * std::sin(m_time * 18.0f);
        intensity = (0.55f + m_chargeProgress * 0.45f) * pulse * lvBoost;
    }

    if (m_burstTimer > 0)
    {
        float burstRatio = static_cast<float>(m_burstTimer) / 38.0f;
        intensity = (std::max)(intensity, burstRatio * lvBoost);
    }

    return MathHelper::Clamp(intensity, 0.0f, 1.25f);
}

void CustomEffectManager::UpdateWorkoutFlame(
    const VECTOR& playerPos,
    bool isWorkingOut,
    int repCount,
    float chargeProgress,
    bool allowBodySmoke
)
{
    m_targetPos = playerPos;
    m_currentRepCount = repCount;
    m_chargeProgress = MathHelper::Clamp(chargeProgress, 0.0f, 1.0f);

    if (!m_effectEnabled)
    {
        m_isBurning = false;
        m_isSmokeActive = false;
        return;
    }

    // Lv.10以上（SMOKE_START_REP_LEVEL以上）なら体から常時・筋トレ中に熱煙が立ち昇る
    m_isSmokeActive = (allowBodySmoke && repCount >= SMOKE_START_REP_LEVEL);

    // 筋トレ開始した瞬間に最初の炎がすぐ立ち昇るよう初期スパートをかける
    bool justStarted = (isWorkingOut && !m_isBurning);
    m_isBurning = isWorkingOut;

    if (justStarted)
    {
        SpawnWorkoutFlameParticles(24, false);
        SpawnEmberSparks(14, false);
        if (m_isSmokeActive)
        {
            SpawnBodySmokeParticles(8, false);
        }
    }
}

void CustomEffectManager::PlayWorkoutSuccessBurst(const VECTOR& playerPos, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_targetPos = playerPos;
    m_currentRepCount = repCount;
    m_burstTimer = 38; // 約0.63秒間、超爆炎ブースト

    int effRep = (std::min)(repCount, 15);
    float scaleFactor = 1.0f + static_cast<float>(effRep) * 0.045f;

    // 1. 地面と中空に広がる2重の火炎衝撃波リング（レベルに応じて火の色も濃くなる）
    {
        FireShockRing ring1;
        ring1.center       = VGet(playerPos.x, playerPos.y + 2.0f, playerPos.z);
        ring1.radius       = 22.0f * scaleFactor;
        ring1.maxRadius    = 135.0f * scaleFactor;
        ring1.height       = 26.0f * scaleFactor;
        ring1.riseSpeed    = 0.4f;
        ring1.age          = 0.0f;
        ring1.maxLife      = 24.0f;
        ring1.colorVariant = 0;
        ring1.repLevel     = repCount;
        m_rings.push_back(ring1);

        FireShockRing ring2;
        ring2.center       = VGet(playerPos.x, playerPos.y + 24.0f, playerPos.z);
        ring2.radius       = 18.0f * scaleFactor;
        ring2.maxRadius    = 105.0f * scaleFactor;
        ring2.height       = 38.0f * scaleFactor;
        ring2.riseSpeed    = 3.2f;
        ring2.age          = 0.0f;
        ring2.maxLife      = 22.0f;
        ring2.colorVariant = 1;
        ring2.repLevel     = repCount;
        m_rings.push_back(ring2);
    }

    // 2. 全身から天高く噴き上がる爆炎パーティクル＆火柱の一斉放出
    SpawnWorkoutFlameParticles(48 + effRep * 2, true);

    // 3. 周囲へ弾け飛ぶ大量の紅蓮スパーク（火の粉）
    SpawnEmberSparks(40 + effRep * 2, true);

    // 4. Lv.10以上ではパンプ成功時にも体から白い湯気（蒸気）が勢いよく噴き出す
    if (repCount >= SMOKE_START_REP_LEVEL)
    {
        SpawnBodySmokeParticles(18 + (effRep - SMOKE_START_REP_LEVEL) * 3, true);
    }
}

void CustomEffectManager::SpawnGougeDebrisAndDust(
    const VECTOR& groundPos,
    const VECTOR& dirXZ,
    int debrisCount,
    int dustCount,
    float scaleBoost,
    bool isImpact
)
{
    VECTOR forward = dirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen > 0.001f)
    {
        forward = VScale(forward, 1.0f / fLen);
    }
    else
    {
        forward = VGet(0.0f, 0.0f, 1.0f);
    }
    VECTOR right = VGet(forward.z, 0.0f, -forward.x);

    // 1. 地面からえぐれて空中へ弾け飛ぶ3D岩片・床破片（重力＆バウンド付き）
    for (int i = 0; i < debrisCount; ++i)
    {
        GroundDebrisChunk chunk;
        chunk.groundY = groundPos.y;
        float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;

        if (isImpact)
        {
            float angle = RandRange(0.0f, MathHelper::PI * 2.0f);
            float dist  = RandRange(4.0f, 26.0f) * scaleBoost;
            chunk.pos = VGet(
                groundPos.x + std::cos(angle) * dist,
                groundPos.y + RandRange(2.0f, 8.0f),
                groundPos.z + std::sin(angle) * dist
            );
            float hSpd = RandRange(2.2f, 6.8f) * scaleBoost;
            chunk.vel = VGet(
                std::cos(angle) * hSpd - forward.x * RandRange(0.5f, 2.5f),
                RandRange(5.2f, 11.5f) * scaleBoost,
                std::sin(angle) * hSpd - forward.z * RandRange(0.5f, 2.5f)
            );
        }
        else
        {
            // 飛翔中のえぐり：進行方向の両脇へV字状に土塊・岩片を跳ね飛ばす
            float sideDist = RandRange(6.0f, 22.0f) * scaleBoost * sideSign;
            float backOffset = RandRange(-8.0f, 8.0f);
            chunk.pos = VGet(
                groundPos.x + right.x * sideDist + forward.x * backOffset,
                groundPos.y + RandRange(2.0f, 6.0f),
                groundPos.z + right.z * sideDist + forward.z * backOffset
            );
            float sideSpd = RandRange(2.4f, 5.8f) * scaleBoost * sideSign;
            float backSpd = RandRange(-3.5f, -0.8f);
            chunk.vel = VGet(
                right.x * sideSpd + forward.x * backSpd,
                RandRange(4.0f, 8.8f) * scaleBoost,
                right.z * sideSpd + forward.z * backSpd
            );
        }

        chunk.size         = RandRange(3.6f, 8.2f) * scaleBoost * (isImpact ? 1.15f : 1.0f);
        chunk.rotAngle     = RandRange(0.0f, MathHelper::PI * 2.0f);
        chunk.rotSpeed     = RandRange(-0.28f, 0.28f);
        chunk.tiltAngle    = RandRange(0.2f, 1.2f);
        chunk.age          = 0.0f;
        chunk.maxLife      = isImpact ? RandRange(32.0f, 52.0f) : RandRange(24.0f, 40.0f);
        chunk.colorVariant = std::rand() % 3;

        m_debrisChunks.push_back(chunk);
    }

    // 2. えぐれた溝から斜め上方へ激しく噴き上がる土煙スプラッシュ（GougeDust）
    for (int i = 0; i < dustCount; ++i)
    {
        FlameParticle p;
        p.type = FlameParticleType::GougeDust;
        p.spawnRepLevel = m_currentRepCount;
        p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);

        float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
        float sideOffset = RandRange(6.0f, 24.0f) * scaleBoost * sideSign;
        p.pos = VGet(
            groundPos.x + right.x * sideOffset + RandRange(-6.0f, 6.0f),
            groundPos.y + RandRange(4.0f, 12.0f),
            groundPos.z + right.z * sideOffset + RandRange(-6.0f, 6.0f)
        );

        float sideVel = RandRange(1.4f, 3.6f) * scaleBoost * sideSign;
        float backVel = isImpact ? RandRange(-2.2f, 2.2f) : RandRange(-2.5f, -0.4f);
        p.vel = VGet(
            right.x * sideVel + forward.x * backVel,
            RandRange(1.8f, 4.2f) * scaleBoost,
            right.z * sideVel + forward.z * backVel
        );

        p.angularSpeed   = RandRange(-0.04f, 0.04f);
        p.width          = RandRange(20.0f, 34.0f) * scaleBoost * (isImpact ? 1.25f : 1.0f);
        p.height         = p.width * RandRange(0.85f, 1.20f);
        p.age            = 0.0f;
        p.maxLife        = isImpact ? RandRange(24.0f, 38.0f) : RandRange(18.0f, 30.0f);
        p.colorVariant   = std::rand() % 3;
        p.followPlayerXZ = false;

        m_particles.push_back(p);
    }
}

void CustomEffectManager::StartPounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    // レベルが上がるほどサイズ・破片数・派手さが大幅に強化される
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.075f;

    VECTOR forward = dirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen > 0.001f)
    {
        forward = VScale(forward, 1.0f / fLen);
    }
    else
    {
        forward = VGet(0.0f, 0.0f, 1.0f);
    }
    VECTOR right = VGet(forward.z, 0.0f, -forward.x);

    // 踏み切り地点の地面えぐりクレーター生成
    GroundGougeSegment startCrater;
    startCrater.startPos    = groundPos;
    startCrater.endPos      = VAdd(groundPos, VScale(forward, 26.0f * scaleBoost));
    startCrater.dir         = forward;
    startCrater.right       = right;
    startCrater.width       = 38.0f * scaleBoost;
    startCrater.ridgeHeight = 15.0f * scaleBoost;
    startCrater.age         = 0.0f;
    startCrater.maxLife     = 65.0f + extraLv * 1.5f;
    startCrater.seed        = RandRange(0.0f, 100.0f);
    startCrater.repLevel    = repCount;
    startCrater.isCrater    = true;
    m_gougeSegments.push_back(startCrater);

    m_lastGougePos = groundPos;
    m_hasLastGougePos = true;

    // 踏み切り時の激しい岩片＆土煙バースト（レベルに応じて破片数・土煙数が増加）
    SpawnGougeDebrisAndDust(groundPos, forward, 14 + extraLv * 2, 10 + extraLv, scaleBoost, true);
}

void CustomEffectManager::UpdatePounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, float progress, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.075f;

    VECTOR forward = dirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen > 0.001f)
    {
        forward = VScale(forward, 1.0f / fLen);
    }
    else
    {
        forward = VGet(0.0f, 0.0f, 1.0f);
    }
    VECTOR right = VGet(forward.z, 0.0f, -forward.x);

    if (!m_hasLastGougePos)
    {
        m_lastGougePos = groundPos;
        m_hasLastGougePos = true;
    }

    float distXZ = MathHelper::DistanceXZ(groundPos, m_lastGougePos);
    if (distXZ >= 11.0f)
    {
        VECTOR segVec = VSub(groundPos, m_lastGougePos);
        segVec.y = 0.0f;
        float segLen = VSize(segVec);
        VECTOR segDir = (segLen > 0.001f) ? VScale(segVec, 1.0f / segLen) : forward;
        VECTOR segRight = VGet(segDir.z, 0.0f, -segDir.x);

        GroundGougeSegment seg;
        seg.startPos    = m_lastGougePos;
        seg.endPos      = groundPos;
        seg.dir         = segDir;
        seg.right       = segRight;
        seg.width       = (34.0f + std::sin(progress * MathHelper::PI) * 8.0f) * scaleBoost;
        seg.ridgeHeight = (12.0f + std::sin(progress * MathHelper::PI) * 6.0f) * scaleBoost;
        seg.age         = 0.0f;
        seg.maxLife     = 62.0f + extraLv * 1.5f;
        seg.seed        = RandRange(0.0f, 100.0f);
        seg.repLevel    = repCount;
        seg.isCrater    = false;
        m_gougeSegments.push_back(seg);

        m_lastGougePos = groundPos;
    }

    // 飛翔中も毎フレーム地面を裂きながら岩片と土煙を左右へ巻き上げる（高レベルほど大量に飛散）
    SpawnGougeDebrisAndDust(groundPos, forward, 3 + (extraLv / 2), 2 + (extraLv / 3), scaleBoost, false);
}

void CustomEffectManager::FinishPounceGroundGouge(const VECTOR& groundPos, const VECTOR& dirXZ, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.075f;

    VECTOR forward = dirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen > 0.001f)
    {
        forward = VScale(forward, 1.0f / fLen);
    }
    else
    {
        forward = VGet(0.0f, 0.0f, 1.0f);
    }
    VECTOR right = VGet(forward.z, 0.0f, -forward.x);

    if (m_hasLastGougePos && MathHelper::DistanceXZ(groundPos, m_lastGougePos) > 2.0f)
    {
        GroundGougeSegment seg;
        seg.startPos    = m_lastGougePos;
        seg.endPos      = groundPos;
        seg.dir         = forward;
        seg.right       = right;
        seg.width       = 36.0f * scaleBoost;
        seg.ridgeHeight = 14.0f * scaleBoost;
        seg.age         = 0.0f;
        seg.maxLife     = 65.0f + extraLv * 1.5f;
        seg.seed        = RandRange(0.0f, 100.0f);
        seg.repLevel    = repCount;
        seg.isCrater    = false;
        m_gougeSegments.push_back(seg);
    }

    // 着地フィニッシュ地点：前方の地面を豪快に叩き割る大型クレーター＆隆起岩塊
    VECTOR impactCenter = VAdd(groundPos, VScale(forward, 14.0f * scaleBoost));
    impactCenter.y = groundPos.y;

    GroundGougeSegment endCrater;
    endCrater.startPos    = groundPos;
    endCrater.endPos      = VAdd(impactCenter, VScale(forward, 28.0f * scaleBoost));
    endCrater.dir         = forward;
    endCrater.right       = right;
    endCrater.width       = 46.0f * scaleBoost;
    endCrater.ridgeHeight = 20.0f * scaleBoost;
    endCrater.age         = 0.0f;
    endCrater.maxLife     = 72.0f + extraLv * 2.0f;
    endCrater.seed        = RandRange(0.0f, 100.0f);
    endCrater.repLevel    = repCount;
    endCrater.isCrater    = true;
    m_gougeSegments.push_back(endCrater);

    m_hasLastGougePos = false;

    // 着地叩きつけ時の特大破片＆土煙スプラッシュ（レベルが高いほどド派手に噴出）
    SpawnGougeDebrisAndDust(impactCenter, forward, 18 + extraLv * 3, 12 + extraLv * 2, scaleBoost * 1.15f, true);

    // Lv.7以上では着地衝撃で地割れマグマ衝撃波リングも広がる
    if (repCount >= 7)
    {
        FireShockRing shock;
        shock.center       = VGet(impactCenter.x, impactCenter.y + 1.5f, impactCenter.z);
        shock.radius       = 20.0f * scaleBoost;
        shock.maxRadius    = (95.0f + extraLv * 8.0f) * scaleBoost;
        shock.height       = (16.0f + extraLv * 1.8f) * scaleBoost;
        shock.riseSpeed    = 0.25f;
        shock.age          = 0.0f;
        shock.maxLife      = 20.0f;
        shock.colorVariant = 0;
        shock.repLevel     = repCount;
        m_rings.push_back(shock);
    }
}

void CustomEffectManager::SpawnSpaceTimeSlashAndRift(
    const VECTOR& centerPos,
    const VECTOR& dirXZ,
    int repCount,
    float scaleBoost,
    bool isCrossSlash
)
{
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4); // 0 (Lv.4) 〜 11 (Lv.15)

    VECTOR forward = dirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen > 0.001f)
    {
        forward = VScale(forward, 1.0f / fLen);
    }
    else
    {
        forward = VGet(0.0f, 0.0f, 1.0f);
    }
    VECTOR backward = VScale(forward, -1.0f);
    VECTOR right    = VGet(forward.z, 0.0f, -forward.x);
    VECTOR up       = VGet(0.0f, 1.0f, 0.0f);

    // 1. 音速突破の「電撃ソニックブーム衝撃波リング」（進行方向に垂直な円環が猫の周囲で弾け広がる）
    int ringCount = isCrossSlash ? (1 + extraLv / 6) : ((std::rand() % 4 == 0) ? 1 : 0);
    for (int i = 0; i < ringCount; ++i)
    {
        SpaceTimeSlashArc sonicRing;
        float fwdOffset = isCrossSlash
            ? (18.0f - static_cast<float>(i) * 18.0f) * scaleBoost
            : RandRange(-8.0f, 14.0f) * scaleBoost;
        sonicRing.center = VAdd(centerPos, VAdd(VScale(forward, fwdOffset), VGet(0.0f, RandRange(-4.0f, 6.0f), 0.0f)));

        // 進行方向（forward）に直交する平面（right × up）で円環を構成し、そこを猫が突き抜ける
        sonicRing.planeU   = right;
        sonicRing.planeV   = up;
        sonicRing.driftVel = VScale(backward, RandRange(2.2f, 4.5f));

        sonicRing.radius       = (isCrossSlash ? (20.0f + i * 8.0f) : 18.0f) * scaleBoost;
        sonicRing.maxRadius    = (isCrossSlash ? (76.0f + i * 16.0f) : 58.0f) * scaleBoost * RandRange(0.95f, 1.15f);
        sonicRing.bladeWidth   = (isCrossSlash ? 14.0f : 10.5f) * scaleBoost;
        sonicRing.arcSpan      = MathHelper::PI * 2.0f;
        sonicRing.age          = 0.0f;
        sonicRing.maxLife      = isCrossSlash ? 16.0f : 12.0f;
        sonicRing.colorVariant = (i + std::rand()) % 2;

        m_spaceSlashes.push_back(sonicRing);
    }

    // 2. 猛烈な疾走感を生む「3D電撃スピードライン（疾走風圧ストリーク）」
    // 猫の周囲360度（円筒状）および足元から後方（-forward）へ長大な黄色い閃光ラインが猛スピードで流れる
    int speedLineCount = isCrossSlash ? (4 + extraLv / 3) : (2 + extraLv / 5);
    for (int r = 0; r < speedLineCount; ++r)
    {
        SpaceTimeRift speedLine;

        // 視界を遮らないよう猫の体の外周（半径 20〜62）にトンネル状に配置
        float cylAngle = RandRange(0.0f, MathHelper::PI * 2.0f);
        float cylRadius = RandRange(20.0f, 58.0f) * scaleBoost;
        float sideOff   = std::cos(cylAngle) * cylRadius;
        float heightOff = std::sin(cylAngle) * (cylRadius * 0.68f) + RandRange(-6.0f, 12.0f);
        // 地面より下には潜らないようにクランプ
        if (centerPos.y + heightOff < centerPos.y - 24.0f * scaleBoost)
        {
            heightOff = -24.0f * scaleBoost + RandRange(2.0f, 10.0f);
        }

        float fwdOff = RandRange(-28.0f, 26.0f) * scaleBoost;
        speedLine.center = VAdd(
            centerPos,
            VAdd(VScale(forward, fwdOff), VAdd(VScale(right, sideOff), VGet(0.0f, heightOff, 0.0f)))
        );

        // 進行方向の真後ろ（backward）を主軸とし、わずかに外側へ広がるスピードコーン角度をつける
        VECTOR radialOut = VNorm(VAdd(VScale(right, std::cos(cylAngle)), VScale(up, std::sin(cylAngle) * 0.5f)));
        speedLine.mainAxis = VNorm(VAdd(backward, VScale(radialOut, RandRange(0.02f, 0.12f))));

        VECTOR perp = VCross(speedLine.mainAxis, up);
        if (VSize(perp) < 0.001f)
        {
            perp = VCross(speedLine.mainAxis, right);
        }
        speedLine.sideAxis = VNorm(perp);

        // 後方へ猛スピードで流れる気流速度
        float streakSpd = (isCrossSlash ? RandRange(11.0f, 20.0f) : RandRange(8.5f, 16.5f)) * (0.9f + scaleBoost * 0.15f);
        speedLine.vel = VScale(speedLine.mainAxis, streakSpd);

        speedLine.length       = (isCrossSlash ? RandRange(75.0f, 130.0f) : RandRange(60.0f, 105.0f)) * scaleBoost;
        speedLine.maxOpenWidth = (isCrossSlash ? RandRange(5.5f, 9.5f)    : RandRange(4.2f, 7.8f)) * scaleBoost;
        speedLine.age          = 0.0f;
        speedLine.maxLife      = isCrossSlash ? RandRange(12.0f, 18.0f) : RandRange(9.0f, 14.0f);
        speedLine.seed         = RandRange(0.0f, 100.0f);
        speedLine.colorVariant = (r + std::rand()) % 2;

        m_spaceRifts.push_back(speedLine);
    }

    // 3. 全身に纏い＆疾走軌跡として後方へ激しく尾を引く「黄色い3D稲妻・雷撃（ThunderBoltArc）」
    // レベルが高いほど本数・長さ・太さが増し、雷光の尾を引きながら超高速移動する
    int boltCount = isCrossSlash ? (3 + extraLv / 3) : (1 + extraLv / 5);
    for (int b = 0; b < boltCount; ++b)
    {
        ThunderBoltArc bolt;
        float angle = RandRange(0.0f, MathHelper::PI * 2.0f);
        float bodyR = RandRange(8.0f, 26.0f) * scaleBoost;

        // 始点は猫の身体の表面（肩・背中・脇腹・脚まわり）
        bolt.startPos = VAdd(
            centerPos,
            VAdd(
                VScale(right, std::cos(angle) * bodyR),
                VAdd(
                    VGet(0.0f, RandRange(-22.0f, 24.0f) * scaleBoost, 0.0f),
                    VScale(forward, RandRange(-10.0f, 18.0f) * scaleBoost)
                )
            )
        );

        if (b % 3 != 0)
        {
            // 【疾走雷撃テイル（約2/3）】身体から後方（-forward）へ長く伸びてバチバチと尾を引く稲妻
            float trailDist = (isCrossSlash ? RandRange(55.0f, 120.0f) : RandRange(48.0f, 105.0f)) * scaleBoost;
            float spreadSide = std::cos(angle) * RandRange(14.0f, 46.0f) * scaleBoost;
            float spreadY    = (b % 2 == 0)
                ? RandRange(-26.0f, -10.0f) * scaleBoost  // 足元の地面スレスレを這う電撃トレイル
                : RandRange(-10.0f,  28.0f) * scaleBoost; // 背中〜肩から後方空中へ流れる電撃トレイル

            bolt.endPos = VAdd(
                centerPos,
                VAdd(
                    VScale(backward, trailDist),
                    VAdd(VScale(right, spreadSide), VGet(0.0f, spreadY, 0.0f))
                )
            );
        }
        else
        {
            // 【全身スパーク放電（約1/3）】身体の周囲を取り囲むようにバチッと走る電撃オーラ
            float nextAngle = angle + RandRange(0.8f, 1.8f) * ((b % 2 == 0) ? 1.0f : -1.0f);
            float outerR = RandRange(24.0f, 54.0f) * scaleBoost;
            bolt.endPos = VAdd(
                centerPos,
                VAdd(
                    VScale(right, std::cos(nextAngle) * outerR),
                    VAdd(
                        VGet(0.0f, RandRange(-22.0f, 28.0f) * scaleBoost, 0.0f),
                        VScale(backward, RandRange(8.0f, 42.0f) * scaleBoost)
                    )
                )
            );
        }

        VECTOR boltDir = VSub(bolt.endPos, bolt.startPos);
        if (VSize(boltDir) < 0.01f)
        {
            boltDir = backward;
        }
        else
        {
            boltDir = VNorm(boltDir);
        }

        VECTOR pA = VCross(boltDir, up);
        if (VSize(pA) < 0.01f)
        {
            pA = VCross(boltDir, right);
        }
        bolt.perpA = VNorm(pA);
        bolt.perpB = VNorm(VCross(boltDir, bolt.perpA));

        bolt.thickness = RandRange(4.5f, 8.2f) * scaleBoost * (isCrossSlash ? 1.15f : 1.0f);
        bolt.jaggedAmp = RandRange(10.0f, 22.0f) * scaleBoost;
        bolt.age       = 0.0f;
        bolt.maxLife   = isCrossSlash ? RandRange(10.0f, 16.0f) : RandRange(7.0f, 12.0f);
        bolt.seed      = RandRange(0.0f, 100.0f);
        bolt.hasBranch = (b % 2 == 0);

        m_thunderBolts.push_back(bolt);
    }

    // 4. 疾走する猫の足元・身体から後方へ弾け飛ぶ「高圧電撃スパーク粒（SpaceTimeShard）」
    int sparkCount = isCrossSlash ? (5 + extraLv / 2) : (2 + extraLv / 5);
    for (int s = 0; s < sparkCount; ++s)
    {
        SpaceTimeShard spark;
        float angle = RandRange(0.0f, MathHelper::PI * 2.0f);
        float radius = RandRange(8.0f, 38.0f) * scaleBoost;
        spark.pos = VAdd(
            centerPos,
            VAdd(
                VScale(right, std::cos(angle) * radius),
                VGet(0.0f, RandRange(-22.0f, 22.0f) * scaleBoost, 0.0f)
            )
        );

        // 後方（backward）へ高速で吹き飛ぶスパーク速度
        float backSpd = isCrossSlash ? RandRange(6.0f, 13.5f) : RandRange(5.0f, 11.0f);
        float sideSpd = std::cos(angle) * RandRange(1.0f, 4.2f);
        spark.vel = VAdd(
            VScale(backward, backSpd),
            VAdd(VScale(right, sideSpd), VGet(0.0f, RandRange(-1.0f, 3.2f), 0.0f))
        );
        spark.size         = RandRange(4.8f, 10.5f) * scaleBoost;
        spark.rotAngle     = RandRange(0.0f, MathHelper::PI * 2.0f);
        spark.rotSpeed     = RandRange(-0.30f, 0.30f);
        spark.tiltAngle    = RandRange(0.15f, 1.35f);
        spark.age          = 0.0f;
        spark.maxLife      = isCrossSlash ? RandRange(16.0f, 26.0f) : RandRange(12.0f, 20.0f);
        spark.colorVariant = s % 3;

        m_spaceShards.push_back(spark);
    }
}

void CustomEffectManager::StartTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.08f;
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float chestY = playerPos.y + 435.4f * catScale * 0.45f;

    VECTOR dashCenter = VGet(playerPos.x, chestY, playerPos.z);
    SpawnSpaceTimeSlashAndRift(dashCenter, dirXZ, repCount, scaleBoost, true);

    m_lastRiftPos = dashCenter;
    m_hasLastRiftPos = true;
}

void CustomEffectManager::UpdateTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int timer, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.08f;
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float chestY = playerPos.y + 435.4f * catScale * 0.45f;

    VECTOR curCenter = VGet(playerPos.x, chestY, playerPos.z);
    if (!m_hasLastRiftPos)
    {
        m_lastRiftPos = curCenter;
        m_hasLastRiftPos = true;
    }

    // 電撃疾走中は毎フレーム途切れなくスピードラインと雷撃トレイルを発生させ、圧倒的な疾走感を演出
    if (timer % 2 == 0) SpawnSpaceTimeSlashAndRift(curCenter, dirXZ, repCount, scaleBoost * 0.92f, false);

    m_lastRiftPos = curCenter;
}

void CustomEffectManager::FinishTackleSpaceTimeRift(const VECTOR& playerPos, const VECTOR& dirXZ, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_currentRepCount = repCount;
    int effRep = (std::min)(repCount, 15);
    int extraLv = (std::max)(0, effRep - 4);
    float scaleBoost = 1.0f + static_cast<float>(extraLv) * 0.08f;
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float chestY = playerPos.y + 435.4f * catScale * 0.45f;

    VECTOR finishCenter = VGet(playerPos.x, chestY, playerPos.z);
    SpawnSpaceTimeSlashAndRift(finishCenter, dirXZ, repCount, scaleBoost * 1.12f, true);
    m_hasLastRiftPos = false;
}

void CustomEffectManager::UpdateRunDust(const VECTOR& footPos, const VECTOR& moveDirXZ, float speedRatio, bool isDashing, int repCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    // 発生間隔：通常走行は3フレームに1回、電撃疾走中は毎フレーム
    m_runDustCounter++;
    int interval = isDashing ? 1 : 3;
    if (m_runDustCounter < interval)
    {
        return;
    }
    m_runDustCounter = 0;

    VECTOR forward = moveDirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen < 0.001f)
    {
        return;
    }
    forward = VScale(forward, 1.0f / fLen);
    VECTOR backward = VScale(forward, -1.0f);
    VECTOR right    = VGet(forward.z, 0.0f, -forward.x);

    int effRep = (std::min)((std::max)(repCount, 0), 15);
    float lvScale = 1.0f + static_cast<float>(effRep) * 0.03f;
    float spdScale = MathHelper::Clamp(speedRatio, 0.5f, 1.8f);

    int count = isDashing ? 2 : 1;
    for (int i = 0; i < count; ++i)
    {
        FlameParticle p;
        p.type = FlameParticleType::RunDust;
        p.spawnRepLevel = repCount;
        p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);

        float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
        float sideOff  = sideSign * RandRange(3.0f, 10.0f) * lvScale;
        float backOff  = RandRange(3.0f, 12.0f);
        p.pos = VAdd(footPos, VAdd(VScale(backward, backOff), VAdd(VScale(right, sideOff), VGet(0.0f, RandRange(1.0f, 3.5f), 0.0f))));

        float backSpd = (isDashing ? RandRange(1.4f, 2.8f) : RandRange(0.5f, 1.2f)) * spdScale;
        float sideSpd = sideSign * RandRange(0.15f, 0.7f);
        float upSpd   = isDashing ? RandRange(0.8f, 1.5f) : RandRange(0.4f, 0.9f);
        p.vel = VAdd(VScale(backward, backSpd), VAdd(VScale(right, sideSpd), VGet(0.0f, upSpd, 0.0f)));

        p.width   = RandRange(8.0f, 14.0f) * lvScale * (isDashing ? 1.25f : 1.0f);
        p.height  = p.width * RandRange(0.70f, 0.95f);
        p.age     = 0.0f;
        p.maxLife = isDashing ? RandRange(18.0f, 26.0f) : RandRange(16.0f, 24.0f);
        p.colorVariant = std::rand() % 2;
        p.followPlayerXZ = false;

        m_particles.push_back(p);
    }
}

void CustomEffectManager::SpawnMouseFleeDust(const VECTOR& footPos, const VECTOR& moveDirXZ)
{
    if (!m_effectEnabled)
    {
        return;
    }

    VECTOR forward = moveDirXZ;
    forward.y = 0.0f;
    float fLen = VSize(forward);
    if (fLen < 0.001f)
    {
        return;
    }
    forward = VScale(forward, 1.0f / fLen);
    VECTOR backward = VScale(forward, -1.0f);
    VECTOR right    = VGet(forward.z, 0.0f, -forward.x);

    FlameParticle p;
    p.type = FlameParticleType::RunDust;
    p.spawnRepLevel = 0;
    p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);

    float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
    float sideOff  = sideSign * RandRange(2.0f, 5.0f);
    float backOff  = RandRange(3.0f, 7.0f);
    p.pos = VAdd(footPos, VAdd(VScale(backward, backOff), VAdd(VScale(right, sideOff), VGet(0.0f, RandRange(0.5f, 2.0f), 0.0f))));

    float backSpd = RandRange(0.7f, 1.6f);
    float sideSpd = sideSign * RandRange(0.1f, 0.4f);
    float upSpd   = RandRange(0.35f, 0.8f);
    p.vel = VAdd(VScale(backward, backSpd), VAdd(VScale(right, sideSpd), VGet(0.0f, upSpd, 0.0f)));

    p.width   = RandRange(4.0f, 7.0f); // ネズミ用ミニサイズ
    p.height  = p.width * 0.85f;
    p.age     = 0.0f;
    p.maxLife = RandRange(12.0f, 20.0f);
    p.colorVariant = std::rand() % 2;
    p.followPlayerXZ = false;

    m_particles.push_back(p);
}

void CustomEffectManager::SpawnWorkoutFlameParticles(int count, bool isBurst)
{
    int effRep = (std::min)(m_currentRepCount, 15);
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float catHeight = 435.4f * catScale;
    float bodyRadius = 26.0f * (catScale / ModelConfig::CAT_MODEL_SCALE);
    float chargeBoost = 1.0f + m_chargeProgress * 0.45f;

    VECTOR camPos = GetCameraPosition();
    float camAngle = std::atan2(camPos.z - m_targetPos.z, camPos.x - m_targetPos.x);

    for (int i = 0; i < count; ++i)
    {
        FlameParticle p;
        p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);
        p.spawnRepLevel = m_currentRepCount;

        float rRoll = Rand01();
        if (rRoll < 0.34f)
        {
            // 【火柱リボン】猫の左右側面〜背面（±95度〜180度）から豪快に立ち昇る炎の柱
            p.type = FlameParticleType::FirePillar;

            float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
            float angleOffset = sideSign * RandRange(0.52f * MathHelper::PI, 0.98f * MathHelper::PI);
            float angle = camAngle + angleOffset;
            p.baseAngle = angle;

            p.orbitRadius = bodyRadius * RandRange(0.85f, 1.55f);
            float spawnOffsetY = RandRange(0.0f, catHeight * 0.42f);
            p.pos = VGet(
                m_targetPos.x + std::cos(angle) * p.orbitRadius,
                m_targetPos.y + spawnOffsetY,
                m_targetPos.z + std::sin(angle) * p.orbitRadius
            );

            float upSpeed = (isBurst ? RandRange(4.8f, 8.6f) : RandRange(3.0f, 5.5f)) * chargeBoost;
            p.vel = VGet(0.0f, upSpeed, 0.0f);
            p.angularSpeed = sideSign * RandRange(0.01f, 0.035f);
            p.width  = RandRange(18.0f, 30.0f) * (isBurst ? 1.35f : 1.0f);
            p.height = RandRange(58.0f, 96.0f) * (isBurst ? 1.35f : chargeBoost);
            p.maxLife = isBurst ? RandRange(20.0f, 30.0f) : RandRange(16.0f, 25.0f);
            p.colorVariant = (std::rand() % 3 == 0) ? 2 : ((std::rand() % 3 == 0) ? 1 : 0);
            p.followPlayerXZ = true;
        }
        else if (rRoll < 0.88f)
        {
            // 【炎の舌】足元全周および左右側面〜背面の腕・肩・バーベル周辺からメラメラと立ち昇るメインの炎
            p.type = FlameParticleType::FlameTongue;

            bool isLowFootFlame = (Rand01() < 0.28f);
            float angle = 0.0f;
            float spawnOffsetY = 0.0f;

            if (isLowFootFlame)
            {
                angle = RandRange(0.0f, MathHelper::PI * 2.0f);
                spawnOffsetY = RandRange(0.0f, catHeight * 0.12f);
                p.orbitRadius = bodyRadius * RandRange(0.85f, 1.40f);
            }
            else
            {
                float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
                float angleOffset = sideSign * RandRange(0.51f * MathHelper::PI, 0.98f * MathHelper::PI);
                angle = camAngle + angleOffset;
                spawnOffsetY = RandRange(catHeight * 0.12f, catHeight * 0.72f);
                p.orbitRadius = bodyRadius * RandRange(0.75f, 1.50f);
            }

            p.baseAngle = angle;
            p.pos = VGet(
                m_targetPos.x + std::cos(angle) * p.orbitRadius,
                m_targetPos.y + spawnOffsetY,
                m_targetPos.z + std::sin(angle) * p.orbitRadius
            );

            float outSpeed = isBurst ? RandRange(0.8f, 2.6f) : RandRange(-0.10f, 0.55f);
            float upSpeed  = (isBurst ? RandRange(3.8f, 7.4f) : (isLowFootFlame ? RandRange(1.0f, 2.0f) : RandRange(2.4f, 4.6f))) * chargeBoost;
            p.vel = VGet(
                std::cos(angle) * outSpeed,
                upSpeed,
                std::sin(angle) * outSpeed
            );
            p.angularSpeed = RandRange(-0.02f, 0.02f);
            p.width  = RandRange(18.0f, 32.0f) * (isBurst ? 1.30f : 1.0f);
            p.height = (isLowFootFlame ? RandRange(18.0f, 30.0f) : RandRange(38.0f, 66.0f)) * (isBurst ? 1.35f : chargeBoost);
            p.maxLife = isBurst ? RandRange(20.0f, 32.0f) : (isLowFootFlame ? RandRange(10.0f, 16.0f) : RandRange(16.0f, 26.0f));
            p.colorVariant = (std::rand() % 3 == 0) ? 2 : ((std::rand() % 3 == 0) ? 1 : 0);
            p.followPlayerXZ = !isBurst;
        }
        else
        {
            // 【コア熱気グロー】猫の背面〜側面で輝く灼熱のバックライトオーラ
            p.type = FlameParticleType::CoreGlow;
            float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
            float angle = camAngle + sideSign * RandRange(0.56f * MathHelper::PI, 1.0f * MathHelper::PI);
            p.baseAngle = angle;
            p.orbitRadius = bodyRadius * RandRange(0.55f, 1.20f);
            float spawnOffsetY = RandRange(12.0f, catHeight * 0.65f);
            p.pos = VGet(
                m_targetPos.x + std::cos(angle) * p.orbitRadius,
                m_targetPos.y + spawnOffsetY,
                m_targetPos.z + std::sin(angle) * p.orbitRadius
            );
            p.vel = VGet(0.0f, RandRange(1.5f, 3.0f), 0.0f);
            p.angularSpeed = RandRange(-0.02f, 0.02f);
            p.width  = RandRange(32.0f, 48.0f);
            p.height = p.width;
            p.maxLife = RandRange(14.0f, 22.0f);
            p.colorVariant = 0;
            p.followPlayerXZ = true;
        }

        p.age = 0.0f;
        m_particles.push_back(p);
    }
}

void CustomEffectManager::SpawnEmberSparks(int count, bool isBurst)
{
    int effRep = (std::min)(m_currentRepCount, 15);
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float catHeight = 435.4f * catScale;
    float bodyRadius = 26.0f * (catScale / ModelConfig::CAT_MODEL_SCALE);

    for (int i = 0; i < count; ++i)
    {
        FlameParticle p;
        p.type = FlameParticleType::EmberSpark;
        p.spawnRepLevel = m_currentRepCount;

        float angle = RandRange(0.0f, MathHelper::PI * 2.0f);
        p.baseAngle = angle;
        p.orbitRadius = bodyRadius * RandRange(0.75f, 1.75f);
        p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);

        float spawnOffsetY = RandRange(6.0f, catHeight * 0.85f);
        p.pos = VGet(
            m_targetPos.x + std::cos(angle) * p.orbitRadius,
            m_targetPos.y + spawnOffsetY,
            m_targetPos.z + std::sin(angle) * p.orbitRadius
        );

        float radialSpd = isBurst ? RandRange(1.6f, 5.2f) : RandRange(0.35f, 1.5f);
        float upSpd     = isBurst ? RandRange(4.2f, 8.8f) : RandRange(2.8f, 5.8f);

        p.vel = VGet(
            std::cos(angle) * radialSpd,
            upSpd,
            std::sin(angle) * radialSpd
        );
        p.angularSpeed = RandRange(-0.10f, 0.10f);
        p.width  = RandRange(5.0f, 9.0f);
        p.height = RandRange(11.0f, 20.0f);
        p.age    = 0.0f;
        p.maxLife = isBurst ? RandRange(24.0f, 42.0f) : RandRange(20.0f, 34.0f);
        p.colorVariant = (std::rand() % 2);
        p.followPlayerXZ = false;

        m_particles.push_back(p);
    }
}

void CustomEffectManager::SpawnBodySmokeParticles(int count, bool isBurst)
{
    int effRep = (std::min)(m_currentRepCount, 15);
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float catHeight = 435.4f * catScale;
    float bodyRadius = 24.0f * (catScale / ModelConfig::CAT_MODEL_SCALE);

    // Lv.10からLv.15にかけて湯気のサイズ・勢いも増す
    float smokeLvRatio = MathHelper::Clamp(static_cast<float>(m_currentRepCount - SMOKE_START_REP_LEVEL) / 5.0f, 0.0f, 1.2f);
    float sizeScale = 1.0f + smokeLvRatio * 0.28f;

    VECTOR camPos = GetCameraPosition();
    float camAngle = std::atan2(camPos.z - m_targetPos.z, camPos.x - m_targetPos.x);

    for (int i = 0; i < count; ++i)
    {
        FlameParticle p;
        p.type = FlameParticleType::BodySmoke;
        p.spawnRepLevel = m_currentRepCount;
        p.phase = RandRange(0.0f, MathHelper::PI * 2.0f);

        // 猫の肩・背中・両腕・胸上部から白い湯気がシューッと立ち昇る
        float sideSign = (Rand01() < 0.5f) ? 1.0f : -1.0f;
        float angleOffset = sideSign * RandRange(0.32f * MathHelper::PI, 0.96f * MathHelper::PI);
        float angle = camAngle + angleOffset;
        p.baseAngle = angle;

        p.orbitRadius = bodyRadius * RandRange(0.38f, 1.05f);
        float spawnOffsetY = RandRange(catHeight * 0.35f, catHeight * 0.88f);

        p.pos = VGet(
            m_targetPos.x + std::cos(angle) * p.orbitRadius,
            m_targetPos.y + spawnOffsetY,
            m_targetPos.z + std::sin(angle) * p.orbitRadius
        );

        float outSpd = isBurst ? RandRange(0.7f, 2.2f) : RandRange(0.20f, 0.80f);
        float upSpd  = isBurst ? RandRange(2.2f, 4.4f) : RandRange(1.4f, 2.8f);

        p.vel = VGet(
            std::cos(angle) * outSpd,
            upSpd,
            std::sin(angle) * outSpd
        );
        p.angularSpeed = RandRange(-0.025f, 0.025f);
        p.width  = RandRange(15.0f, 26.0f) * sizeScale * (isBurst ? 1.2f : 1.0f);
        p.height = p.width * RandRange(1.15f, 1.50f);
        p.age    = 0.0f;
        p.maxLife = isBurst ? RandRange(30.0f, 48.0f) : RandRange(26.0f, 44.0f);
        p.colorVariant = (std::rand() % 3);
        // 移動時には湯気の尾が空間に残るようワールド空間で上昇させる
        p.followPlayerXZ = false;

        m_particles.push_back(p);
    }
}

void CustomEffectManager::Update()
{
    if (!m_effectEnabled)
    {
        return;
    }

    m_time += 1.0f / 60.0f;

    if (m_burstTimer > 0)
    {
        m_burstTimer--;
    }

    // 1. 筋トレ中（または成功バースト余韻中）は毎フレーム新たな炎と火の粉を発生させる
    if (m_isBurning || m_burstTimer > 0)
    {
        int effRep = (std::min)(m_currentRepCount, 15);
        int baseFlameCount = 6 + (effRep / 3) + static_cast<int>(m_chargeProgress * 5.0f);
        if (m_burstTimer > 18)
        {
            baseFlameCount += 5;
        }

        SpawnWorkoutFlameParticles(baseFlameCount, false);

        int sparkCount = 2 + (effRep / 4) + ((m_chargeProgress > 0.5f) ? 2 : 1);
        SpawnEmberSparks(sparkCount, false);
    }

    // 2. Lv.10超え（SMOKE_START_REP_LEVEL以上）なら体から煙を継続発生させる（通常移動時＆筋トレ中ともに有効）
    if (m_isSmokeActive && m_currentRepCount >= SMOKE_START_REP_LEVEL)
    {
        int extraLv = (std::min)(m_currentRepCount - SMOKE_START_REP_LEVEL, 8);
        // 筋トレ燃焼中は毎フレーム2〜4個、通常パンプ維持中も2フレームに1回〜毎フレーム体から煙が湧き出る
        int smokeCount = 0;
        if (m_isBurning || m_burstTimer > 0)
        {
            smokeCount = 2 + (extraLv / 2);
        }
        else
        {
            smokeCount = 1 + (extraLv / 3);
        }
        SpawnBodySmokeParticles(smokeCount, false);
    }

    // 3. 全パーティクルの物理・寿命更新
    for (auto it = m_particles.begin(); it != m_particles.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_particles.erase(it);
            continue;
        }

        float lifeRatio = it->age / it->maxLife;
        it->baseAngle += it->angularSpeed;

        if (it->type == FlameParticleType::BodySmoke)
        {
            // 湯気パーティクル：上昇するにつれてふわっと横に揺らぎながら拡散する
            float swayX = std::sin(m_time * 7.5f + it->phase + it->age * 0.14f) * 0.55f;
            float swayZ = std::cos(m_time * 7.5f + it->phase + it->age * 0.14f) * 0.55f;
            it->pos.x += it->vel.x + swayX;
            it->pos.y += it->vel.y;
            it->pos.z += it->vel.z + swayZ;

            it->vel.x *= 0.97f;
            it->vel.z *= 0.97f;
            it->vel.y *= 0.992f;
        }
        else if (it->type == FlameParticleType::RunDust)
        {
            // 走行土煙：後方へ流れながら減速し、ふわっと広がって漂う
            it->pos.x += it->vel.x;
            it->pos.y += it->vel.y;
            it->pos.z += it->vel.z;
            it->vel.x *= 0.90f;
            it->vel.z *= 0.90f;
            it->vel.y *= 0.88f;
        }
        else if (it->type == FlameParticleType::GougeDust)
        {
            // 地面えぐり土煙パーティクル：斜め上方へ噴き上がり減速しながら広がる
            it->pos.x += it->vel.x;
            it->pos.y += it->vel.y;
            it->pos.z += it->vel.z;

            it->vel.x *= 0.92f;
            it->vel.z *= 0.92f;
            it->vel.y *= 0.90f;
        }
        else if (it->followPlayerXZ)
        {
            float taperRadius = it->orbitRadius * (1.0f - lifeRatio * 0.15f);
            float sway = std::sin(m_time * 12.0f + it->phase + it->age * 0.25f) * 3.8f;
            it->pos.x = m_targetPos.x + std::cos(it->baseAngle) * (taperRadius + sway);
            it->pos.z = m_targetPos.z + std::sin(it->baseAngle) * (taperRadius + sway);
            it->pos.y += it->vel.y;
        }
        else
        {
            float turbX = std::sin(m_time * 15.0f + it->phase + it->age * 0.3f) * 0.65f;
            float turbZ = std::cos(m_time * 15.0f + it->phase + it->age * 0.3f) * 0.65f;
            it->pos.x += it->vel.x + turbX;
            it->pos.y += it->vel.y;
            it->pos.z += it->vel.z + turbZ;

            it->vel.x *= 0.95f;
            it->vel.z *= 0.95f;
        }

        ++it;
    }

    // 4. 火炎衝撃波リングの更新
    for (auto it = m_rings.begin(); it != m_rings.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_rings.erase(it);
            continue;
        }

        float t = it->age / it->maxLife;
        float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
        it->radius = MathHelper::Lerp(18.0f, it->maxRadius, easeOut);
        it->center.x = m_targetPos.x;
        it->center.z = m_targetPos.z;
        it->center.y += it->riseSpeed;

        ++it;
    }

    // 5. 地面えぐり爪痕・3D隆起セグメントの寿命更新
    for (auto it = m_gougeSegments.begin(); it != m_gougeSegments.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_gougeSegments.erase(it);
            continue;
        }
        ++it;
    }

    // 6. 地面からえぐれて飛び散る3D岩片・破片の物理（重力・バウンド）更新
    for (auto it = m_debrisChunks.begin(); it != m_debrisChunks.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_debrisChunks.erase(it);
            continue;
        }

        it->pos.x += it->vel.x;
        it->pos.y += it->vel.y;
        it->pos.z += it->vel.z;

        // 重力加速度
        it->vel.y -= 0.52f;
        it->rotAngle += it->rotSpeed;

        // 地面バウンド判定
        float minH = it->groundY + it->size * 0.35f;
        if (it->pos.y < minH)
        {
            it->pos.y = minH;
            it->vel.y = -it->vel.y * 0.42f;
            it->vel.x *= 0.68f;
            it->vel.z *= 0.68f;
            it->rotSpeed *= 0.75f;
        }

        ++it;
    }

    // 7. スタン攻撃（電撃疾走）の電撃ソニックブーム衝撃波リング更新
    for (auto it = m_spaceSlashes.begin(); it != m_spaceSlashes.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_spaceSlashes.erase(it);
            continue;
        }
        float t = it->age / it->maxLife;
        float easeOut = 1.0f - (1.0f - t) * (1.0f - t);
        it->radius = MathHelper::Lerp(it->maxRadius * 0.28f, it->maxRadius, easeOut);
        it->center = VAdd(it->center, it->driftVel);
        ++it;
    }

    // 8. 電撃疾走で後方へ猛スピードで流れる「3D電撃スピードライン（疾走風圧ストリーク）」更新
    for (auto it = m_spaceRifts.begin(); it != m_spaceRifts.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_spaceRifts.erase(it);
            continue;
        }
        // 後方へ高速移動させることで強烈な疾走感を演出
        it->center = VAdd(it->center, it->vel);
        ++it;
    }

    // 9. 疾走気流で後方へ弾け飛ぶ「高圧電撃スパーク粒」更新
    for (auto it = m_spaceShards.begin(); it != m_spaceShards.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_spaceShards.erase(it);
            continue;
        }

        it->pos.x += it->vel.x;
        it->pos.y += it->vel.y;
        it->pos.z += it->vel.z;

        it->vel.x *= 0.94f;
        it->vel.y *= 0.94f;
        it->vel.z *= 0.94f;
        it->rotAngle += it->rotSpeed;

        ++it;
    }

    // 10. スタン攻撃で周囲に迸る「黄色い3D稲妻・雷撃（ThunderBoltArc）」更新
    for (auto it = m_thunderBolts.begin(); it != m_thunderBolts.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_thunderBolts.erase(it);
            continue;
        }
        ++it;
    }

    // 11. ネズミ捕獲時の祝祭エフェクト（星噴水・衝撃波・光線・昇天ソウル）更新
    UpdateCatchCelebration();
}

void CustomEffectManager::DrawFlameQuad3D(
    const VECTOR& centerPos,
    const VECTOR& upDir,
    const VECTOR& camPos,
    float bottomWidth,
    float topWidth,
    float height,
    COLOR_U8 bottomColor,
    COLOR_U8 topColor,
    int grHandle
) const
{
    VECTOR toCam = VSub(camPos, centerPos);
    VECTOR right = VCross(upDir, toCam);
    float rightLen = VSize(right);
    if (rightLen < 0.0001f)
    {
        right = VGet(1.0f, 0.0f, 0.0f);
    }
    else
    {
        right = VScale(right, 1.0f / rightLen);
    }

    VECTOR bottomCenter = VSub(centerPos, VScale(upDir, height * 0.38f));
    VECTOR topCenter    = VAdd(centerPos, VScale(upDir, height * 0.62f));

    VECTOR bl = VSub(bottomCenter, VScale(right, bottomWidth * 0.5f));
    VECTOR br = VAdd(bottomCenter, VScale(right, bottomWidth * 0.5f));
    VECTOR tl = VSub(topCenter,    VScale(right, topWidth * 0.5f));
    VECTOR tr = VAdd(topCenter,    VScale(right, topWidth * 0.5f));

    VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
    COLOR_U8 spc = GetColorU8(0, 0, 0, 0);

    VERTEX3D verts[6] = {};

    // Triangle 1: Top-Left, Top-Right, Bottom-Left
    verts[0].pos = tl; verts[0].norm = norm; verts[0].dif = topColor;    verts[0].spc = spc; verts[0].u = 0.0f; verts[0].v = 0.0f;
    verts[1].pos = tr; verts[1].norm = norm; verts[1].dif = topColor;    verts[1].spc = spc; verts[1].u = 1.0f; verts[1].v = 0.0f;
    verts[2].pos = bl; verts[2].norm = norm; verts[2].dif = bottomColor; verts[2].spc = spc; verts[2].u = 0.0f; verts[2].v = 1.0f;

    // Triangle 2: Bottom-Left, Top-Right, Bottom-Right
    verts[3].pos = bl; verts[3].norm = norm; verts[3].dif = bottomColor; verts[3].spc = spc; verts[3].u = 0.0f; verts[3].v = 1.0f;
    verts[4].pos = tr; verts[4].norm = norm; verts[4].dif = topColor;    verts[4].spc = spc; verts[4].u = 1.0f; verts[4].v = 0.0f;
    verts[5].pos = br; verts[5].norm = norm; verts[5].dif = bottomColor; verts[5].spc = spc; verts[5].u = 1.0f; verts[5].v = 1.0f;

    DrawPolygon3D(verts, 2, (grHandle != -1) ? grHandle : DX_NONE_GRAPH, TRUE);
}

void CustomEffectManager::DrawGroundFireVortex(const VECTOR& center, float baseRadius, float intensity) const
{
    constexpr int SEGMENTS = 32;
    VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
    COLOR_U8 spc = GetColorU8(0, 0, 0, 0);

    // レベルに応じて足元の火炎渦も「明るい黄橙(Lv.0) → 濃い深紅・黒赤(Lv.15)」へ変化
    float lvRatio = MathHelper::Clamp(static_cast<float>(m_currentRepCount) / 15.0f, 0.0f, 1.0f);

    // A. 足元の地面を染める灼熱サークル
    {
        float discRadius = baseRadius * (1.45f + lvRatio * 0.20f);
        unsigned char centerAlpha = static_cast<unsigned char>(MathHelper::Clamp((185.0f + lvRatio * 45.0f) * intensity, 0.0f, 255.0f));
        unsigned char midAlpha    = static_cast<unsigned char>(MathHelper::Clamp((145.0f + lvRatio * 55.0f) * intensity, 0.0f, 255.0f));

        int cR = static_cast<int>(MathHelper::Lerp(255.0f, 210.0f, lvRatio));
        int cG = static_cast<int>(MathHelper::Lerp(185.0f, 22.0f,  lvRatio));
        int cB = static_cast<int>(MathHelper::Lerp(40.0f,  4.0f,   lvRatio));

        int mR = static_cast<int>(MathHelper::Lerp(240.0f, 130.0f, lvRatio));
        int mG = static_cast<int>(MathHelper::Lerp(75.0f,  6.0f,   lvRatio));
        int mB = static_cast<int>(MathHelper::Lerp(12.0f,  2.0f,   lvRatio));

        COLOR_U8 centerCol = GetColorU8(cR, cG, cB, centerAlpha);
        COLOR_U8 midCol    = GetColorU8(mR, mG, mB, midAlpha);
        COLOR_U8 edgeCol   = GetColorU8(mR / 2, 0, 0, 0);

        std::vector<VERTEX3D> discVerts;
        discVerts.reserve(SEGMENTS * 9);
        VECTOR cPos = VGet(center.x, center.y + 0.6f, center.z);
        float midRadius = discRadius * 0.65f;

        for (int s = 0; s < SEGMENTS; ++s)
        {
            float a1 = static_cast<float>(s) * (2.0f * MathHelper::PI / static_cast<float>(SEGMENTS));
            float a2 = static_cast<float>(s + 1) * (2.0f * MathHelper::PI / static_cast<float>(SEGMENTS));

            VECTOR m1 = VGet(center.x + std::cos(a1) * midRadius, center.y + 0.6f, center.z + std::sin(a1) * midRadius);
            VECTOR m2 = VGet(center.x + std::cos(a2) * midRadius, center.y + 0.6f, center.z + std::sin(a2) * midRadius);
            VECTOR e1 = VGet(center.x + std::cos(a1) * discRadius, center.y + 0.6f, center.z + std::sin(a1) * discRadius);
            VECTOR e2 = VGet(center.x + std::cos(a2) * discRadius, center.y + 0.6f, center.z + std::sin(a2) * discRadius);

            VERTEX3D vIn[3] = {};
            vIn[0].pos = cPos; vIn[0].norm = norm; vIn[0].dif = centerCol; vIn[0].spc = spc;
            vIn[1].pos = m1;   vIn[1].norm = norm; vIn[1].dif = midCol;    vIn[1].spc = spc;
            vIn[2].pos = m2;   vIn[2].norm = norm; vIn[2].dif = midCol;    vIn[2].spc = spc;
            discVerts.push_back(vIn[0]);
            discVerts.push_back(vIn[1]);
            discVerts.push_back(vIn[2]);

            VERTEX3D vOut[6] = {};
            vOut[0].pos = m1; vOut[0].norm = norm; vOut[0].dif = midCol;  vOut[0].spc = spc;
            vOut[1].pos = e1; vOut[1].norm = norm; vOut[1].dif = edgeCol; vOut[1].spc = spc;
            vOut[2].pos = e2; vOut[2].norm = norm; vOut[2].dif = edgeCol; vOut[2].spc = spc;
            vOut[3].pos = m1; vOut[3].norm = norm; vOut[3].dif = midCol;  vOut[3].spc = spc;
            vOut[4].pos = e2; vOut[4].norm = norm; vOut[4].dif = edgeCol; vOut[4].spc = spc;
            vOut[5].pos = m2; vOut[5].norm = norm; vOut[5].dif = midCol;  vOut[5].spc = spc;
            for (int k = 0; k < 6; ++k) discVerts.push_back(vOut[k]);
        }

        DrawPolygon3D(discVerts.data(), SEGMENTS * 3, DX_NONE_GRAPH, TRUE);
    }

    // B. 内側リングと外側リングで激しく波打つ3D火炎の王冠メッシュ
    for (int layer = 0; layer < 2; ++layer)
    {
        float r = baseRadius * (layer == 0 ? 1.12f : 0.78f);
        float wallH = (layer == 0 ? 32.0f : 20.0f) * (0.85f + intensity * 0.45f) * (1.0f + lvRatio * 0.25f);
        float timeSign = (layer == 0) ? 1.0f : -1.35f;

        unsigned char baseAlpha = static_cast<unsigned char>(MathHelper::Clamp((layer == 0 ? 220.0f : 240.0f) * intensity, 0.0f, 255.0f));
        unsigned char tipAlpha  = static_cast<unsigned char>(MathHelper::Clamp(65.0f * intensity, 0.0f, 255.0f));

        COLOR_U8 bottomCol, topCol;
        GetFlameColors(0.15f, (layer == 0 ? 2 : 0), m_currentRepCount, baseAlpha, bottomCol, topCol);
        topCol.a = tipAlpha;

        std::vector<VERTEX3D> ringVerts;
        ringVerts.reserve(SEGMENTS * 6);

        for (int s = 0; s < SEGMENTS; ++s)
        {
            float a1 = static_cast<float>(s) * (2.0f * MathHelper::PI / static_cast<float>(SEGMENTS)) + m_time * 4.0f * timeSign;
            float a2 = static_cast<float>(s + 1) * (2.0f * MathHelper::PI / static_cast<float>(SEGMENTS)) + m_time * 4.0f * timeSign;

            float wave1 = 0.55f + 0.45f * std::sin(a1 * 8.0f + m_time * 18.0f);
            float wave2 = 0.55f + 0.45f * std::sin(a2 * 8.0f + m_time * 18.0f);

            VECTOR b1 = VGet(center.x + std::cos(a1) * r, center.y + 0.8f, center.z + std::sin(a1) * r);
            VECTOR b2 = VGet(center.x + std::cos(a2) * r, center.y + 0.8f, center.z + std::sin(a2) * r);

            float topR1 = r + std::sin(a1 * 6.0f - m_time * 12.0f) * 4.5f;
            float topR2 = r + std::sin(a2 * 6.0f - m_time * 12.0f) * 4.5f;
            VECTOR t1 = VGet(center.x + std::cos(a1) * topR1, center.y + 0.8f + wallH * wave1, center.z + std::sin(a1) * topR1);
            VECTOR t2 = VGet(center.x + std::cos(a2) * topR2, center.y + 0.8f + wallH * wave2, center.z + std::sin(a2) * topR2);

            VERTEX3D v[6] = {};
            v[0].pos = t1; v[0].norm = norm; v[0].dif = topCol;    v[0].spc = spc; v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos = t2; v[1].norm = norm; v[1].dif = topCol;    v[1].spc = spc; v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos = b1; v[2].norm = norm; v[2].dif = bottomCol; v[2].spc = spc; v[2].u = 0.0f; v[2].v = 1.0f;

            v[3].pos = b1; v[3].norm = norm; v[3].dif = bottomCol; v[3].spc = spc; v[3].u = 0.0f; v[3].v = 1.0f;
            v[4].pos = t2; v[4].norm = norm; v[4].dif = topCol;    v[4].spc = spc; v[4].u = 1.0f; v[4].v = 0.0f;
            v[5].pos = b2; v[5].norm = norm; v[5].dif = bottomCol; v[5].spc = spc; v[5].u = 1.0f; v[5].v = 1.0f;

            for (int k = 0; k < 6; ++k)
            {
                ringVerts.push_back(v[k]);
            }
        }

        DrawPolygon3D(ringVerts.data(), SEGMENTS * 2, DX_NONE_GRAPH, TRUE);
    }
}

void CustomEffectManager::DrawGroundGougeSegments() const
{
    if (m_gougeSegments.empty())
    {
        return;
    }

    VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
    COLOR_U8 spc = GetColorU8(0, 0, 0, 0);

    std::vector<VERTEX3D> verts;
    verts.reserve(m_gougeSegments.size() * 60);

    auto pushTri = [&](const VECTOR& p0, const VECTOR& p1, const VECTOR& p2, COLOR_U8 c0, COLOR_U8 c1, COLOR_U8 c2)
    {
        VERTEX3D v[3] = {};
        v[0].pos = p0; v[0].norm = norm; v[0].dif = c0; v[0].spc = spc;
        v[1].pos = p1; v[1].norm = norm; v[1].dif = c1; v[1].spc = spc;
        v[2].pos = p2; v[2].norm = norm; v[2].dif = c2; v[2].spc = spc;
        verts.push_back(v[0]);
        verts.push_back(v[1]);
        verts.push_back(v[2]);
    };

    auto pushQuad = [&](const VECTOR& bl, const VECTOR& br, const VECTOR& tl, const VECTOR& tr, COLOR_U8 bCol, COLOR_U8 tCol)
    {
        pushTri(tl, tr, bl, tCol, tCol, bCol);
        pushTri(bl, tr, br, bCol, tCol, bCol);
    };

    for (const auto& seg : m_gougeSegments)
    {
        float lifeRatio = seg.age / seg.maxLife;
        // 出現直後にガッと隆起し、後半で徐々に地面へ沈みながらフェードアウト
        float popScale = (lifeRatio < 0.12f) ? (lifeRatio / 0.12f) : (1.0f - (std::max)(0.0f, (lifeRatio - 0.55f) / 0.45f));
        popScale = MathHelper::Clamp(popScale, 0.0f, 1.0f);

        unsigned char alphaOuter = static_cast<unsigned char>(MathHelper::Clamp(popScale * 225.0f, 0.0f, 255.0f));
        unsigned char alphaInner = static_cast<unsigned char>(MathHelper::Clamp(popScale * 248.0f, 0.0f, 255.0f));
        if (alphaInner <= 3)
        {
            continue;
        }

        // えぐれた地面の色（高レベルほど溝の底がマグマのような灼熱オレンジ〜深紅を帯びて派手になる）
        float lvRatio = MathHelper::Clamp(static_cast<float>((std::max)(0, seg.repLevel - 4)) / 11.0f, 0.0f, 1.0f);
        COLOR_U8 rimCol   = GetColorU8(
            static_cast<int>(MathHelper::Lerp(138.0f, 225.0f, lvRatio)),
            static_cast<int>(MathHelper::Lerp(110.0f, 95.0f,  lvRatio)),
            static_cast<int>(MathHelper::Lerp(80.0f,  30.0f,  lvRatio)),
            alphaOuter
        );
        COLOR_U8 abyssCol = GetColorU8(28, 20, 15, alphaInner);

        // 3D隆起岩塊の面カラー（上面明部・側面中間・内側断崖暗部、高レベルほど熱を帯びた赤熱エッジが混じる）
        COLOR_U8 ridgeTopCol   = GetColorU8(
            static_cast<int>(MathHelper::Lerp(168.0f, 235.0f, lvRatio * 0.65f)),
            static_cast<int>(MathHelper::Lerp(138.0f, 125.0f, lvRatio * 0.65f)),
            static_cast<int>(MathHelper::Lerp(102.0f, 55.0f,  lvRatio * 0.65f)),
            alphaInner
        );
        COLOR_U8 ridgeOuterCol = GetColorU8(112, 88, 64, alphaOuter);
        COLOR_U8 ridgeInnerCol = GetColorU8(52, 40, 30, alphaInner);

        float baseY = (seg.startPos.y + seg.endPos.y) * 0.5f;

        // A. 3本の平行な爪痕・地面えぐり溝（中央＋左右の爪）
        const float laneOffsets[3] = { 0.0f, -seg.width * 0.35f, seg.width * 0.35f };
        const float laneWidths[3]  = { seg.width * 0.28f, seg.width * 0.18f, seg.width * 0.18f };

        for (int lane = 0; lane < 3; ++lane)
        {
            VECTOR laneShift = VScale(seg.right, laneOffsets[lane]);
            VECTOR sCenter = VAdd(seg.startPos, laneShift);
            VECTOR eCenter = VAdd(seg.endPos, laneShift);
            sCenter.y = baseY + 0.55f;
            eCenter.y = baseY + 0.55f;

            float hwOuter = laneWidths[lane] * 0.5f;
            float hwInner = laneWidths[lane] * 0.26f;

            // 外側のめくれた土石縁
            VECTOR sL_out = VSub(sCenter, VScale(seg.right, hwOuter));
            VECTOR sR_out = VAdd(sCenter, VScale(seg.right, hwOuter));
            VECTOR eL_out = VSub(eCenter, VScale(seg.right, hwOuter));
            VECTOR eR_out = VAdd(eCenter, VScale(seg.right, hwOuter));
            pushQuad(sL_out, sR_out, eL_out, eR_out, rimCol, rimCol);

            // 内側の深くえぐれた黒い溝
            VECTOR sCenterIn = sCenter; sCenterIn.y += 0.20f;
            VECTOR eCenterIn = eCenter; eCenterIn.y += 0.20f;
            VECTOR sL_in = VSub(sCenterIn, VScale(seg.right, hwInner));
            VECTOR sR_in = VAdd(sCenterIn, VScale(seg.right, hwInner));
            VECTOR eL_in = VSub(eCenterIn, VScale(seg.right, hwInner));
            VECTOR eR_in = VAdd(eCenterIn, VScale(seg.right, hwInner));
            pushQuad(sL_in, sR_in, eL_in, eR_in, abyssCol, abyssCol);
        }

        // B. 両脇にガッとめくれ上がる3D岩塊・土塊の隆起スパイク（3D三角錐プリズム）
        VECTOR midPos = VScale(VAdd(seg.startPos, seg.endPos), 0.5f);
        midPos.y = baseY + 0.5f;
        float curRidgeH = seg.ridgeHeight * popScale;

        for (int side = -1; side <= 1; side += 2)
        {
            float sideF = static_cast<float>(side);
            VECTOR sideBase = VAdd(midPos, VScale(seg.right, sideF * (seg.width * 0.52f)));

            // 前後方向・外方向に広がる基底3頂点と、斜め上外側に突き出す頂点peak
            VECTOR pFront = VAdd(sideBase, VScale(seg.dir, 9.5f));
            VECTOR pBack  = VSub(sideBase, VScale(seg.dir, 9.5f));
            VECTOR pOuter = VAdd(sideBase, VScale(seg.right, sideF * 13.0f));
            VECTOR pPeak  = VAdd(sideBase, VAdd(VScale(seg.right, sideF * 5.5f), VGet(0.0f, curRidgeH, 0.0f)));

            pushTri(pFront, pBack, pPeak, ridgeInnerCol, ridgeInnerCol, ridgeTopCol);
            pushTri(pBack, pOuter, pPeak, ridgeOuterCol, ridgeOuterCol, ridgeTopCol);
            pushTri(pOuter, pFront, pPeak, ridgeOuterCol, ridgeOuterCol, ridgeTopCol);
        }

        // C. 踏み切り・着地クレーターの場合は前方・周囲にも放射状の隆起岩片を配置（レベルに応じて岩塊数が増える）
        if (seg.isCrater)
        {
            int craterSpikes = 6 + (std::max)(0, (std::min)(seg.repLevel, 15) - 4) / 2;
            for (int k = 0; k < craterSpikes; ++k)
            {
                float angle = -0.70f * MathHelper::PI + (static_cast<float>(k) / static_cast<float>(craterSpikes - 1)) * (1.40f * MathHelper::PI);
                VECTOR radialDir = VAdd(VScale(seg.dir, std::cos(angle)), VScale(seg.right, std::sin(angle)));
                VECTOR tangentDir = VGet(radialDir.z, 0.0f, -radialDir.x);

                float spikeDist = seg.width * 0.58f;
                VECTOR sBase = VAdd(seg.endPos, VScale(radialDir, spikeDist));
                sBase.y = baseY + 0.5f;

                float spikeH = curRidgeH * (0.85f + 0.30f * std::sin(seg.seed + static_cast<float>(k) * 2.1f));
                VECTOR bL = VSub(sBase, VScale(tangentDir, 7.5f));
                VECTOR bR = VAdd(sBase, VScale(tangentDir, 7.5f));
                VECTOR bOut = VAdd(sBase, VScale(radialDir, 12.0f));
                VECTOR peak = VAdd(sBase, VAdd(VScale(radialDir, 5.0f), VGet(0.0f, spikeH, 0.0f)));

                pushTri(bL, bR, peak, ridgeInnerCol, ridgeInnerCol, ridgeTopCol);
                pushTri(bR, bOut, peak, ridgeOuterCol, ridgeOuterCol, ridgeTopCol);
                pushTri(bOut, bL, peak, ridgeOuterCol, ridgeOuterCol, ridgeTopCol);
            }
        }

        // D. 地面を走る稲妻状のひび割れライン
        unsigned int crackCol = GetColor(38, 30, 22);
        for (int side = -1; side <= 1; side += 2)
        {
            float sideF = static_cast<float>(side);
            VECTOR c0 = VAdd(midPos, VScale(seg.right, sideF * seg.width * 0.45f));
            c0.y = baseY + 0.8f;
            VECTOR c1 = VAdd(c0, VAdd(VScale(seg.right, sideF * 14.0f), VScale(seg.dir, 6.0f * std::sin(seg.seed + sideF))));
            c1.y = baseY + 0.8f;
            VECTOR c2 = VAdd(c1, VAdd(VScale(seg.right, sideF * 10.0f), VScale(seg.dir, -8.0f * std::cos(seg.seed))));
            c2.y = baseY + 0.8f;

            DrawLine3D(c0, c1, crackCol);
            DrawLine3D(c1, c2, crackCol);
        }
    }

    if (!verts.empty())
    {
        DrawPolygon3D(verts.data(), static_cast<int>(verts.size()) / 3, DX_NONE_GRAPH, TRUE);
    }
}

void CustomEffectManager::DrawGroundDebrisChunks() const
{
    if (m_debrisChunks.empty())
    {
        return;
    }

    VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
    COLOR_U8 spc = GetColorU8(0, 0, 0, 0);

    std::vector<VERTEX3D> verts;
    verts.reserve(m_debrisChunks.size() * 24);

    auto pushTri = [&](const VECTOR& p0, const VECTOR& p1, const VECTOR& p2, COLOR_U8 col)
    {
        VERTEX3D v[3] = {};
        v[0].pos = p0; v[0].norm = norm; v[0].dif = col; v[0].spc = spc;
        v[1].pos = p1; v[1].norm = norm; v[1].dif = col; v[1].spc = spc;
        v[2].pos = p2; v[2].norm = norm; v[2].dif = col; v[2].spc = spc;
        verts.push_back(v[0]);
        verts.push_back(v[1]);
        verts.push_back(v[2]);
    };

    for (const auto& c : m_debrisChunks)
    {
        float lifeRatio = c.age / c.maxLife;
        float alphaRatio = (lifeRatio < 0.70f) ? 1.0f : (1.0f - (lifeRatio - 0.70f) / 0.30f);
        unsigned char alpha = static_cast<unsigned char>(MathHelper::Clamp(alphaRatio * 250.0f, 0.0f, 255.0f));
        if (alpha <= 4)
        {
            continue;
        }

        // 岩片・土塊の3色バリエーション（明るい上面と暗い下面で立体的な回転破片を表現）
        COLOR_U8 topCol, midCol, botCol;
        if (c.colorVariant == 0)
        {
            topCol = GetColorU8(175, 145, 110, alpha);
            midCol = GetColorU8(128, 100, 72,  alpha);
            botCol = GetColorU8(68,  52,  38,  alpha);
        }
        else if (c.colorVariant == 1)
        {
            topCol = GetColorU8(145, 138, 130, alpha);
            midCol = GetColorU8(102, 95,  88,  alpha);
            botCol = GetColorU8(55,  50,  46,  alpha);
        }
        else
        {
            topCol = GetColorU8(158, 122, 86, alpha);
            midCol = GetColorU8(110, 82,  55, alpha);
            botCol = GetColorU8(58,  42,  28, alpha);
        }

        float cs = std::cos(c.rotAngle);
        float sn = std::sin(c.rotAngle);
        float r  = c.size;
        float h  = c.size * 0.85f;

        VECTOR axX = VGet(cs * r, std::sin(c.tiltAngle) * r * 0.4f, sn * r);
        VECTOR axZ = VGet(-sn * r * 0.85f, std::cos(c.tiltAngle) * r * 0.3f, cs * r * 0.85f);
        VECTOR axY = VGet(0.0f, h, 0.0f);

        VECTOR pTop = VAdd(c.pos, axY);
        VECTOR pBot = VSub(c.pos, axY);
        VECTOR pPX  = VAdd(c.pos, axX);
        VECTOR pNX  = VSub(c.pos, axX);
        VECTOR pPZ  = VAdd(c.pos, axZ);
        VECTOR pNZ  = VSub(c.pos, axZ);

        // 上半分4面 + 下半分4面の8面体ロックチャンク
        pushTri(pTop, pPX, pPZ, topCol);
        pushTri(pTop, pPZ, pNX, midCol);
        pushTri(pTop, pNX, pNZ, topCol);
        pushTri(pTop, pNZ, pPX, midCol);

        pushTri(pBot, pPZ, pPX, botCol);
        pushTri(pBot, pNX, pPZ, botCol);
        pushTri(pBot, pNZ, pNX, botCol);
        pushTri(pBot, pPX, pNZ, botCol);
    }

    if (!verts.empty())
    {
        DrawPolygon3D(verts.data(), static_cast<int>(verts.size()) / 3, DX_NONE_GRAPH, TRUE);
    }
}

void CustomEffectManager::DrawSpaceTimeEffects() const
{
    if (m_spaceSlashes.empty() && m_spaceRifts.empty() && m_spaceShards.empty() && m_thunderBolts.empty())
    {
        return;
    }

    VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
    COLOR_U8 spc = GetColorU8(0, 0, 0, 0);

    std::vector<VERTEX3D> verts;
    verts.reserve(m_spaceSlashes.size() * 260 + m_spaceRifts.size() * 120 + m_thunderBolts.size() * 140 + m_spaceShards.size() * 12);

    auto pushTri = [&](const VECTOR& p0, const VECTOR& p1, const VECTOR& p2, COLOR_U8 c0, COLOR_U8 c1, COLOR_U8 c2)
    {
        VERTEX3D v[3] = {};
        v[0].pos = p0; v[0].norm = norm; v[0].dif = c0; v[0].spc = spc;
        v[1].pos = p1; v[1].norm = norm; v[1].dif = c1; v[1].spc = spc;
        v[2].pos = p2; v[2].norm = norm; v[2].dif = c2; v[2].spc = spc;
        verts.push_back(v[0]);
        verts.push_back(v[1]);
        verts.push_back(v[2]);
    };

    auto pushQuad = [&](const VECTOR& bl, const VECTOR& br, const VECTOR& tl, const VECTOR& tr, COLOR_U8 cBL, COLOR_U8 cBR, COLOR_U8 cTL, COLOR_U8 cTR)
    {
        pushTri(tl, tr, bl, cTL, cTR, cBL);
        pushTri(bl, tr, br, cBL, cTR, cBR);
    };

    // -------------------------------------------------------------------------
    // A. 猛烈な疾走感を演出する「3D電撃スピードライン（疾走風圧ストリーク：SpaceTimeRift）」
    // -------------------------------------------------------------------------
    for (const auto& speedLine : m_spaceRifts)
    {
        float lifeRatio = speedLine.age / speedLine.maxLife;
        float alphaEnv = (lifeRatio < 0.15f) ? (lifeRatio / 0.15f) : (1.0f - (lifeRatio - 0.15f) / 0.85f);
        alphaEnv = MathHelper::Clamp(alphaEnv, 0.0f, 1.0f);
        if (alphaEnv <= 0.02f)
        {
            continue;
        }

        unsigned char coreAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 255.0f, 0.0f, 255.0f));
        unsigned char midAlpha  = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 240.0f, 0.0f, 255.0f));
        unsigned char edgeAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 215.0f, 0.0f, 255.0f));

        // 白いステージでも鮮烈に映える「黒金アンバー外縁 ＋ エレクトリックイエロー本体 ＋ 白黄フラッシュ芯」
        COLOR_U8 outerEdgeCol = GetColorU8(45, 26, 2, edgeAlpha);
        COLOR_U8 midYellowCol = (speedLine.colorVariant == 0)
            ? GetColorU8(255, 228, 12, midAlpha)
            : GetColorU8(255, 192, 0,  midAlpha);
        COLOR_U8 coreWhiteCol = GetColorU8(255, 255, 200, coreAlpha);

        VECTOR orthoAxis = VCross(speedLine.mainAxis, speedLine.sideAxis);
        if (VSize(orthoAxis) < 0.001f)
        {
            orthoAxis = VGet(0.0f, 1.0f, 0.0f);
        }
        else
        {
            orthoAxis = VNorm(orthoAxis);
        }

        constexpr int LINE_SEGS = 6;
        float halfLen = speedLine.length * 0.5f;

        for (int s = 0; s < LINE_SEGS; ++s)
        {
            float t0 = static_cast<float>(s) / static_cast<float>(LINE_SEGS);
            float t1 = static_cast<float>(s + 1) / static_cast<float>(LINE_SEGS);

            // 先頭（進行方向側 t=0）が鋭く尖り、胴体で太くなり、後方テイル（t=1）へ鋭く抜ける流線型プロファイル
            float profile0 = std::sin(t0 * MathHelper::PI);
            float profile1 = std::sin(t1 * MathHelper::PI);

            // 電撃を帯びた微細な高速振動（ジッター）
            float zap0 = (s == 0 || s == LINE_SEGS) ? 0.0f : std::sin(speedLine.seed + static_cast<float>(s) * 3.5f + m_time * 52.0f) * 2.2f * profile0;
            float zap1 = (s + 1 == LINE_SEGS) ? 0.0f : std::sin(speedLine.seed + static_cast<float>(s + 1) * 3.5f + m_time * 52.0f) * 2.2f * profile1;

            VECTOR c0 = VAdd(speedLine.center, VAdd(VScale(speedLine.mainAxis, (t0 * 2.0f - 1.0f) * halfLen), VScale(speedLine.sideAxis, zap0)));
            VECTOR c1 = VAdd(speedLine.center, VAdd(VScale(speedLine.mainAxis, (t1 * 2.0f - 1.0f) * halfLen), VScale(speedLine.sideAxis, zap1)));

            float wCore0 = speedLine.maxOpenWidth * 0.30f * profile0;
            float wCore1 = speedLine.maxOpenWidth * 0.30f * profile1;
            float wMid0  = speedLine.maxOpenWidth * 0.68f * profile0;
            float wMid1  = speedLine.maxOpenWidth * 0.68f * profile1;
            float wOut0  = speedLine.maxOpenWidth * 1.05f * profile0;
            float wOut1  = speedLine.maxOpenWidth * 1.05f * profile1;

            // 直交2平面（sideAxis / orthoAxis）で3Dスピードラインを描画し、どのカメラ角度からもシャープな疾走線に見せる
            for (int plane = 0; plane < 2; ++plane)
            {
                const VECTOR& sideVec = (plane == 0) ? speedLine.sideAxis : orthoAxis;

                VECTOR oL0 = VSub(c0, VScale(sideVec, wOut0));
                VECTOR mL0 = VSub(c0, VScale(sideVec, wMid0));
                VECTOR iL0 = VSub(c0, VScale(sideVec, wCore0));
                VECTOR iR0 = VAdd(c0, VScale(sideVec, wCore0));
                VECTOR mR0 = VAdd(c0, VScale(sideVec, wMid0));
                VECTOR oR0 = VAdd(c0, VScale(sideVec, wOut0));

                VECTOR oL1 = VSub(c1, VScale(sideVec, wOut1));
                VECTOR mL1 = VSub(c1, VScale(sideVec, wMid1));
                VECTOR iL1 = VSub(c1, VScale(sideVec, wCore1));
                VECTOR iR1 = VAdd(c1, VScale(sideVec, wCore1));
                VECTOR mR1 = VAdd(c1, VScale(sideVec, wMid1));
                VECTOR oR1 = VAdd(c1, VScale(sideVec, wOut1));

                pushQuad(oL0, mL0, oL1, mL1, outerEdgeCol, midYellowCol, outerEdgeCol, midYellowCol);
                pushQuad(mL0, iL0, mL1, iL1, midYellowCol, coreWhiteCol, midYellowCol, coreWhiteCol);
                pushQuad(iL0, iR0, iL1, iR1, coreWhiteCol, coreWhiteCol, coreWhiteCol, coreWhiteCol);
                pushQuad(iR0, mR0, iR1, mR1, coreWhiteCol, midYellowCol, coreWhiteCol, midYellowCol);
                pushQuad(mR0, oR0, mR1, oR1, midYellowCol, outerEdgeCol, midYellowCol, outerEdgeCol);
            }
        }
    }

    // -------------------------------------------------------------------------
    // B. 音速突破の「電撃ソニックブーム衝撃波リング（SpaceTimeSlashArc）」
    // -------------------------------------------------------------------------
    for (const auto& ring : m_spaceSlashes)
    {
        float lifeRatio = ring.age / ring.maxLife;
        float alphaEnv = (lifeRatio < 0.15f) ? (lifeRatio / 0.15f) : (1.0f - (lifeRatio - 0.15f) / 0.85f);
        alphaEnv = MathHelper::Clamp(alphaEnv, 0.0f, 1.0f);
        if (alphaEnv <= 0.02f)
        {
            continue;
        }

        unsigned char coreAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 240.0f, 0.0f, 255.0f));
        unsigned char midAlpha  = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 225.0f, 0.0f, 255.0f));
        unsigned char darkAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 200.0f, 0.0f, 255.0f));

        COLOR_U8 darkAmberEdgeCol = GetColorU8(42, 24, 4, darkAlpha);
        COLOR_U8 yellowRingCol    = (ring.colorVariant == 0)
            ? GetColorU8(255, 225, 10, midAlpha)
            : GetColorU8(255, 190, 0,  midAlpha);
        COLOR_U8 whiteGoldCoreCol = GetColorU8(255, 255, 195, coreAlpha);

        constexpr int RING_SEGS = 24;
        float curThickness = ring.bladeWidth * (1.0f - lifeRatio * 0.48f);

        for (int s = 0; s < RING_SEGS; ++s)
        {
            float a0 = static_cast<float>(s) * (MathHelper::PI * 2.0f / static_cast<float>(RING_SEGS));
            float a1 = static_cast<float>(s + 1) * (MathHelper::PI * 2.0f / static_cast<float>(RING_SEGS));

            // 地面（下側）めり込みを防ぎつつ電撃らしくギザギザと脈動する半径
            float zapR0 = ring.radius + std::sin(a0 * 7.0f + m_time * 42.0f) * 3.8f;
            float zapR1 = ring.radius + std::sin(a1 * 7.0f + m_time * 42.0f) * 3.8f;

            VECTOR dir0 = VAdd(VScale(ring.planeU, std::cos(a0)), VScale(ring.planeV, std::sin(a0) * 0.78f));
            VECTOR dir1 = VAdd(VScale(ring.planeU, std::cos(a1)), VScale(ring.planeV, std::sin(a1) * 0.78f));

            VECTOR pInner0 = VAdd(ring.center, VScale(dir0, zapR0 - curThickness * 0.45f));
            VECTOR pCore0  = VAdd(ring.center, VScale(dir0, zapR0));
            VECTOR pMid0   = VAdd(ring.center, VScale(dir0, zapR0 + curThickness * 0.45f));
            VECTOR pOuter0 = VAdd(ring.center, VScale(dir0, zapR0 + curThickness * 0.85f));

            VECTOR pInner1 = VAdd(ring.center, VScale(dir1, zapR1 - curThickness * 0.45f));
            VECTOR pCore1  = VAdd(ring.center, VScale(dir1, zapR1));
            VECTOR pMid1   = VAdd(ring.center, VScale(dir1, zapR1 + curThickness * 0.45f));
            VECTOR pOuter1 = VAdd(ring.center, VScale(dir1, zapR1 + curThickness * 0.85f));

            pushQuad(pInner0, pCore0, pInner1, pCore1, darkAmberEdgeCol, whiteGoldCoreCol, darkAmberEdgeCol, whiteGoldCoreCol);
            pushQuad(pCore0, pMid0, pCore1, pMid1, whiteGoldCoreCol, yellowRingCol, whiteGoldCoreCol, yellowRingCol);
            pushQuad(pMid0, pOuter0, pMid1, pOuter1, yellowRingCol, darkAmberEdgeCol, yellowRingCol, darkAmberEdgeCol);
        }
    }

    // -------------------------------------------------------------------------
    // C. 全身に纏い＆後方へ激しく尾を引く黄色い3D稲妻・雷撃（ThunderBoltArc）
    // -------------------------------------------------------------------------
    for (const auto& bolt : m_thunderBolts)
    {
        float lifeRatio = bolt.age / bolt.maxLife;
        float alphaEnv = (lifeRatio < 0.20f) ? 1.0f : (1.0f - (lifeRatio - 0.20f) / 0.80f);
        alphaEnv = MathHelper::Clamp(alphaEnv, 0.0f, 1.0f);
        if (alphaEnv <= 0.03f)
        {
            continue;
        }

        unsigned char coreAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 255.0f, 0.0f, 255.0f));
        unsigned char edgeAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 235.0f, 0.0f, 255.0f));

        // 雷撃カラー：黒金アンバーの外縁 ＋ 鮮やかな黄金イエローの雷光 ＋ 白黄コア
        COLOR_U8 boltEdgeCol = GetColorU8(50, 30, 0, edgeAlpha);
        COLOR_U8 boltMidCol  = GetColorU8(255, 220, 0, coreAlpha);
        COLOR_U8 boltCoreCol = GetColorU8(255, 255, 190, coreAlpha);

        constexpr int BOLT_SEGS = 7;
        VECTOR pts[BOLT_SEGS + 1];

        for (int s = 0; s <= BOLT_SEGS; ++s)
        {
            float t = static_cast<float>(s) / static_cast<float>(BOLT_SEGS);
            VECTOR basePt = VAdd(bolt.startPos, VScale(VSub(bolt.endPos, bolt.startPos), t));

            if (s > 0 && s < BOLT_SEGS)
            {
                float env = std::sin(t * MathHelper::PI);
                // 毎フレーム激しくジグザグ形状が跳ねる電撃ノイズ
                float jA = std::sin(bolt.seed + static_cast<float>(s) * 3.7f + m_time * 55.0f) * bolt.jaggedAmp * env;
                float jB = std::cos(bolt.seed * 1.3f + static_cast<float>(s) * 5.1f - m_time * 48.0f) * bolt.jaggedAmp * 0.75f * env;
                basePt = VAdd(basePt, VAdd(VScale(bolt.perpA, jA), VScale(bolt.perpB, jB)));
            }
            pts[s] = basePt;
        }

        for (int s = 0; s < BOLT_SEGS; ++s)
        {
            float t0 = static_cast<float>(s) / static_cast<float>(BOLT_SEGS);
            float t1 = static_cast<float>(s + 1) / static_cast<float>(BOLT_SEGS);
            float w0 = bolt.thickness * (1.0f - t0 * 0.55f);
            float w1 = bolt.thickness * (1.0f - t1 * 0.55f);

            // 互いに直交する2平面（perpA / perpB）で立体3D稲妻リボンを描画し、どの角度から見ても太く鮮烈な雷にする
            for (int plane = 0; plane < 2; ++plane)
            {
                const VECTOR& sideVec = (plane == 0) ? bolt.perpA : bolt.perpB;

                VECTOR pOutL0 = VSub(pts[s],     VScale(sideVec, w0));
                VECTOR pMidL0 = VSub(pts[s],     VScale(sideVec, w0 * 0.45f));
                VECTOR pMidR0 = VAdd(pts[s],     VScale(sideVec, w0 * 0.45f));
                VECTOR pOutR0 = VAdd(pts[s],     VScale(sideVec, w0));

                VECTOR pOutL1 = VSub(pts[s + 1], VScale(sideVec, w1));
                VECTOR pMidL1 = VSub(pts[s + 1], VScale(sideVec, w1 * 0.45f));
                VECTOR pMidR1 = VAdd(pts[s + 1], VScale(sideVec, w1 * 0.45f));
                VECTOR pOutR1 = VAdd(pts[s + 1], VScale(sideVec, w1));

                pushQuad(pOutL0, pMidL0, pOutL1, pMidL1, boltEdgeCol, boltMidCol, boltEdgeCol, boltMidCol);
                pushQuad(pMidL0, pMidR0, pMidL1, pMidR1, boltMidCol, boltCoreCol, boltMidCol, boltCoreCol);
                pushQuad(pMidR0, pOutR0, pMidR1, pOutR1, boltMidCol, boltEdgeCol, boltMidCol, boltEdgeCol);
            }

            // 枝分かれするサブ稲妻（フォーク）
            if (bolt.hasBranch && (s == 2 || s == 4))
            {
                VECTOR branchDir = VAdd(VSub(pts[s + 1], pts[s]), VScale(bolt.perpA, (s == 2 ? 18.0f : -18.0f)));
                VECTOR bMid = VAdd(pts[s], VScale(branchDir, 0.65f));
                VECTOR bEnd = VAdd(bMid, VAdd(VScale(branchDir, 0.55f), VScale(bolt.perpB, 10.0f)));
                DrawLine3D(pts[s], bMid, GetColor(255, 225, 0));
                DrawLine3D(bMid, bEnd, GetColor(210, 140, 0));
            }
        }
    }

    // -------------------------------------------------------------------------
    // D. 疾走気流で後方へ弾け飛ぶ「高圧電撃スパーク粒（SpaceTimeShard）」
    // -------------------------------------------------------------------------
    for (const auto& sh : m_spaceShards)
    {
        float lifeRatio = sh.age / sh.maxLife;
        float alphaEnv = (lifeRatio < 0.65f) ? 1.0f : (1.0f - (lifeRatio - 0.65f) / 0.35f);
        unsigned char alpha = static_cast<unsigned char>(MathHelper::Clamp(alphaEnv * 245.0f, 0.0f, 255.0f));
        if (alpha <= 4)
        {
            continue;
        }

        // 黄金×白黄の電撃スパーク結晶（速度方向に細長く伸びてスピード感を強調）
        COLOR_U8 edgeCol = GetColorU8(48, 28, 2, alpha);
        COLOR_U8 tipCol1 = (sh.colorVariant == 0)
            ? GetColorU8(255, 235, 20, alpha)
            : ((sh.colorVariant == 1) ? GetColorU8(255, 195, 0, alpha) : GetColorU8(255, 255, 195, alpha));
        COLOR_U8 tipCol2 = GetColorU8(255, 255, 210, alpha);

        VECTOR velDir = (VSize(sh.vel) > 0.01f) ? VNorm(sh.vel) : VGet(0.0f, 0.0f, -1.0f);
        VECTOR sideDir = VCross(velDir, VGet(0.0f, 1.0f, 0.0f));
        if (VSize(sideDir) < 0.01f)
        {
            sideDir = VGet(1.0f, 0.0f, 0.0f);
        }
        else
        {
            sideDir = VNorm(sideDir);
        }

        float len = sh.size * 2.2f;
        float wid = sh.size * 0.42f;

        VECTOR pFront = VSub(sh.pos, VScale(velDir, len * 0.6f));
        VECTOR pBack  = VAdd(sh.pos, VScale(velDir, len * 0.9f));
        VECTOR pLeft  = VSub(sh.pos, VScale(sideDir, wid));
        VECTOR pRight = VAdd(sh.pos, VScale(sideDir, wid));

        pushTri(pFront, pRight, pLeft, tipCol2, tipCol1, edgeCol);
        pushTri(pBack,  pLeft,  pRight, edgeCol, tipCol1, tipCol2);
    }

    if (!verts.empty())
    {
        DrawPolygon3D(verts.data(), static_cast<int>(verts.size()) / 3, DX_NONE_GRAPH, TRUE);
    }
}

void CustomEffectManager::Draw3D()
{
    if (!m_effectEnabled)
    {
        return;
    }

    if (!m_isBurning && !m_isSmokeActive && m_burstTimer <= 0 &&
        m_particles.empty() && m_rings.empty() && m_gougeSegments.empty() && m_debrisChunks.empty() &&
        m_spaceSlashes.empty() && m_spaceRifts.empty() && m_spaceShards.empty() && m_thunderBolts.empty() &&
        m_catchStars.empty() && m_catchRings.empty() && m_catchSunbursts.empty() && m_catchSouls.empty())
    {
        return;
    }

    VECTOR camPos = GetCameraPosition();

    VECTOR toCamXZ = VSub(camPos, m_targetPos);
    toCamXZ.y = 0.0f;
    float toCamDistXZ = VSize(toCamXZ);
    if (toCamDistXZ > 0.001f)
    {
        toCamXZ = VScale(toCamXZ, 1.0f / toCamDistXZ);
    }

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(FALSE);
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

    int effRep = (std::min)(m_currentRepCount, 15);
    float catScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effRep) * 0.0025f;
    float baseAuraRadius = 26.0f * (catScale / ModelConfig::CAT_MODEL_SCALE);

    // 0. 飛びつき攻撃でえぐれた地面の爪痕・亀裂・3D岩塊隆起・飛散破片、およびスタン攻撃の時空切り裂き描画
    DrawGroundGougeSegments();
    DrawGroundDebrisChunks();
    DrawSpaceTimeEffects();
    DrawCatchCelebration();

    // 1. 燃焼中の足元火炎渦（グラウンド・ファイア・ヴォルテックス）
    if (m_isBurning || m_burstTimer > 0)
    {
        float vortexIntensity = m_isBurning ? (0.80f + m_chargeProgress * 0.20f) : (static_cast<float>(m_burstTimer) / 38.0f);
        DrawGroundFireVortex(m_targetPos, baseAuraRadius * 1.15f, vortexIntensity);
    }

    // 2. パンプ成功時の拡大する火炎衝撃波リング
    for (const auto& ring : m_rings)
    {
        float lifeRatio = ring.age / ring.maxLife;
        float alphaRatio = 1.0f - lifeRatio;
        unsigned char bottomAlpha = static_cast<unsigned char>(MathHelper::Clamp(alphaRatio * 245.0f, 0.0f, 255.0f));
        unsigned char topAlpha    = static_cast<unsigned char>(MathHelper::Clamp(alphaRatio * 85.0f, 0.0f, 255.0f));

        COLOR_U8 bCol, tCol;
        GetFlameColors(lifeRatio * 0.5f, ring.colorVariant, ring.repLevel, bottomAlpha, bCol, tCol);
        tCol.a = topAlpha;

        constexpr int RING_SEGS = 32;
        VECTOR norm = VGet(0.0f, 1.0f, 0.0f);
        COLOR_U8 spc = GetColorU8(0, 0, 0, 0);
        std::vector<VERTEX3D> verts;
        verts.reserve(RING_SEGS * 6);

        float curWallH = ring.height * (1.0f - lifeRatio * 0.35f);

        for (int s = 0; s < RING_SEGS; ++s)
        {
            float a1 = static_cast<float>(s) * (2.0f * MathHelper::PI / static_cast<float>(RING_SEGS));
            float a2 = static_cast<float>(s + 1) * (2.0f * MathHelper::PI / static_cast<float>(RING_SEGS));

            float wave1 = 0.7f + 0.3f * std::sin(a1 * 8.0f + m_time * 20.0f);
            float wave2 = 0.7f + 0.3f * std::sin(a2 * 8.0f + m_time * 20.0f);

            VECTOR b1 = VGet(ring.center.x + std::cos(a1) * ring.radius, ring.center.y, ring.center.z + std::sin(a1) * ring.radius);
            VECTOR b2 = VGet(ring.center.x + std::cos(a2) * ring.radius, ring.center.y, ring.center.z + std::sin(a2) * ring.radius);
            VECTOR t1 = VGet(b1.x, b1.y + curWallH * wave1, b1.z);
            VECTOR t2 = VGet(b2.x, b2.y + curWallH * wave2, b2.z);

            VERTEX3D v[6] = {};
            v[0].pos = t1; v[0].norm = norm; v[0].dif = tCol; v[0].spc = spc; v[0].u = 0.0f; v[0].v = 0.0f;
            v[1].pos = t2; v[1].norm = norm; v[1].dif = tCol; v[1].spc = spc; v[1].u = 1.0f; v[1].v = 0.0f;
            v[2].pos = b1; v[2].norm = norm; v[2].dif = bCol; v[2].spc = spc; v[2].u = 0.0f; v[2].v = 1.0f;

            v[3].pos = b1; v[3].norm = norm; v[3].dif = bCol; v[3].spc = spc; v[3].u = 0.0f; v[3].v = 1.0f;
            v[4].pos = t2; v[4].norm = norm; v[4].dif = tCol; v[4].spc = spc; v[4].u = 1.0f; v[4].v = 0.0f;
            v[5].pos = b2; v[5].norm = norm; v[5].dif = bCol; v[5].spc = spc; v[5].u = 1.0f; v[5].v = 1.0f;

            for (int k = 0; k < 6; ++k)
            {
                verts.push_back(v[k]);
            }
        }
        DrawPolygon3D(verts.data(), RING_SEGS * 2, DX_NONE_GRAPH, TRUE);
    }

    // 3. 3D炎パーティクル・火柱・スパーク・地面えぐり土煙の描画
    for (const auto& p : m_particles)
    {
        if (p.type == FlameParticleType::BodySmoke)
        {
            continue;
        }

        float lifeRatio = p.age / p.maxLife;

        if (p.type == FlameParticleType::RunDust)
        {
            float dustEnv = (lifeRatio < 0.12f) ? (lifeRatio / 0.12f) : (1.0f - (lifeRatio - 0.12f) / 0.88f);
            unsigned char dustAlpha = static_cast<unsigned char>(MathHelper::Clamp(dustEnv * 135.0f, 0.0f, 255.0f));
            if (dustAlpha <= 2)
            {
                continue;
            }

            // 白い床でも見える淡いサンドベージュ〜グレージュの土煙（主張しすぎず自然）
            COLOR_U8 dustBottom = (p.colorVariant == 0) ? GetColorU8(145, 125, 98, dustAlpha) : GetColorU8(132, 118, 102, dustAlpha);
            COLOR_U8 dustTop    = GetColorU8(180, 165, 142, static_cast<unsigned char>(dustAlpha * 50 / 100));

            float expand = 0.70f + lifeRatio * 1.35f;
            VECTOR upDir = VGet(0.0f, 1.0f, 0.0f);
            DrawFlameQuad3D(p.pos, upDir, camPos, p.width * expand, p.width * expand * 1.15f, p.height * expand, dustBottom, dustTop, m_smokeTexHandle);
            continue;
        }

        if (p.type == FlameParticleType::GougeDust)
        {
            float dustEnv = (lifeRatio < 0.15f) ? (lifeRatio / 0.15f) : (1.0f - (lifeRatio - 0.15f) / 0.85f);
            unsigned char dustAlpha = static_cast<unsigned char>(MathHelper::Clamp(dustEnv * 195.0f, 0.0f, 255.0f));
            if (dustAlpha <= 2)
            {
                continue;
            }

            // 白い床の上でも土煙・砂塵として鮮明に見えるアースブラウン〜サンドベージュ
            COLOR_U8 dustBottom = GetColorU8(122, 98, 72, dustAlpha);
            COLOR_U8 dustTop    = GetColorU8(168, 145, 118, static_cast<unsigned char>(dustAlpha * 70 / 100));

            float expand = 0.75f + lifeRatio * 1.15f;
            VECTOR upDir = VSize(p.vel) > 0.01f ? VNorm(VGet(p.vel.x * 0.35f, 1.0f, p.vel.z * 0.35f)) : VGet(0.0f, 1.0f, 0.0f);
            DrawFlameQuad3D(p.pos, upDir, camPos, p.width * expand * 0.85f, p.width * expand * 1.15f, p.height * expand, dustBottom, dustTop, m_smokeTexHandle);
            continue;
        }

        float env = 1.0f;
        if (lifeRatio < 0.12f)
        {
            env = lifeRatio / 0.12f;
        }
        else
        {
            env = 1.0f - ((lifeRatio - 0.12f) / 0.88f);
        }
        env = MathHelper::Clamp(env, 0.0f, 1.0f);

        VECTOR relXZ = VGet(p.pos.x - m_targetPos.x, 0.0f, p.pos.z - m_targetPos.z);
        float relLen = VSize(relXZ);
        float frontAttenuation = 1.0f;
        if (relLen > 0.001f && p.type != FlameParticleType::EmberSpark)
        {
            float dotFront = (relXZ.x * toCamXZ.x + relXZ.z * toCamXZ.z) / relLen;
            if (dotFront > 0.35f && p.pos.y > m_targetPos.y + 26.0f)
            {
                frontAttenuation = 0.18f;
            }
        }

        unsigned char alphaByte = static_cast<unsigned char>(MathHelper::Clamp(env * frontAttenuation * 252.0f, 0.0f, 255.0f));
        if (alphaByte <= 2)
        {
            continue;
        }

        COLOR_U8 bottomCol, topCol;
        GetFlameColors(lifeRatio, p.colorVariant, p.spawnRepLevel, alphaByte, bottomCol, topCol);

        if (p.type == FlameParticleType::FlameTongue)
        {
            float swayX = std::sin(m_time * 16.0f + p.phase + p.age * 0.35f) * 0.24f;
            float swayZ = std::cos(m_time * 16.0f + p.phase + p.age * 0.35f) * 0.24f;
            VECTOR upDir = VNorm(VGet(swayX, 1.0f, swayZ));

            float scaleGrow = 0.80f + 0.45f * std::sin(lifeRatio * MathHelper::PI);
            float curBottomW = p.width * scaleGrow;
            float curTopW    = curBottomW * 0.38f;
            float curHeight  = p.height * (0.85f + lifeRatio * 0.50f);

            DrawFlameQuad3D(p.pos, upDir, camPos, curBottomW, curTopW, curHeight, bottomCol, topCol, m_flameTexHandle);
        }
        else if (p.type == FlameParticleType::FirePillar)
        {
            float tangentX = -std::sin(p.baseAngle) * 0.18f;
            float tangentZ =  std::cos(p.baseAngle) * 0.18f;
            VECTOR upDir = VNorm(VGet(tangentX, 1.0f, tangentZ));

            float curBottomW = p.width * (1.0f - lifeRatio * 0.20f);
            float curTopW    = curBottomW * 0.45f;
            float curHeight  = p.height * (0.90f + lifeRatio * 0.55f);

            DrawFlameQuad3D(p.pos, upDir, camPos, curBottomW, curTopW, curHeight, bottomCol, topCol, m_streakTexHandle);
        }
        else if (p.type == FlameParticleType::CoreGlow)
        {
            unsigned char glowAlpha = static_cast<unsigned char>(alphaByte * 75 / 100);
            COLOR_U8 glowBottom, glowTop;
            GetFlameColors(0.2f, 0, p.spawnRepLevel, glowAlpha, glowBottom, glowTop);
            VECTOR upDir = VGet(0.0f, 1.0f, 0.0f);
            DrawFlameQuad3D(p.pos, upDir, camPos, p.width, p.width, p.height, glowBottom, glowTop, m_glowTexHandle);
        }
        else if (p.type == FlameParticleType::EmberSpark)
        {
            VECTOR velDir = VSize(p.vel) > 0.01f ? VNorm(p.vel) : VGet(0.0f, 1.0f, 0.0f);
            COLOR_U8 sparkBottom, sparkTop;
            GetFlameColors(0.1f, 1, p.spawnRepLevel, alphaByte, sparkBottom, sparkTop);
            DrawFlameQuad3D(p.pos, velDir, camPos, p.width, p.width * 0.45f, p.height, sparkBottom, sparkTop, m_glowTexHandle);
        }
    }

    // 4. Lv.10超えの体から立ち昇る白い湯気（BodySmoke）の描画
    for (const auto& p : m_particles)
    {
        if (p.type != FlameParticleType::BodySmoke)
        {
            continue;
        }

        float lifeRatio = p.age / p.maxLife;
        float env = 1.0f;
        if (lifeRatio < 0.20f)
        {
            env = lifeRatio / 0.20f;
        }
        else
        {
            env = 1.0f - ((lifeRatio - 0.20f) / 0.80f);
        }
        env = MathHelper::Clamp(env, 0.0f, 1.0f);

        // レベル10から上がるにつれて湯気の量・存在感も増す（透明感のある白い蒸気）
        float steamLvBoost = MathHelper::Clamp(0.72f + static_cast<float>(p.spawnRepLevel - SMOKE_START_REP_LEVEL) * 0.06f, 0.72f, 1.0f);
        unsigned char steamAlpha = static_cast<unsigned char>(MathHelper::Clamp(env * steamLvBoost * 165.0f, 0.0f, 255.0f));
        if (steamAlpha <= 2)
        {
            continue;
        }

        // 湯気の色：体の近く（根元）はほんのり熱気を帯びた純白、上空（先端）は透き通るミストホワイト
        int bR = 255;
        int bG = static_cast<int>(MathHelper::Lerp(250.0f, 254.0f, lifeRatio));
        int bB = static_cast<int>(MathHelper::Lerp(248.0f, 255.0f, lifeRatio));

        int tR = static_cast<int>(MathHelper::Lerp(248.0f, 240.0f, lifeRatio));
        int tG = static_cast<int>(MathHelper::Lerp(252.0f, 246.0f, lifeRatio));
        int tB = 255;

        COLOR_U8 steamBottom = GetColorU8(bR, bG, bB, steamAlpha);
        COLOR_U8 steamTop    = GetColorU8(tR, tG, tB, static_cast<unsigned char>(steamAlpha * 65 / 100));

        // 湯気は上昇するにつれて縦に伸びながらふわっと広がる
        float expandW = 0.68f + lifeRatio * 0.90f;
        float expandH = 0.82f + lifeRatio * 1.25f;
        float curW = p.width * expandW;
        float curH = p.height * expandH;

        float swayX = std::sin(m_time * 6.5f + p.phase + p.age * 0.13f) * 0.22f;
        float swayZ = std::cos(m_time * 6.5f + p.phase + p.age * 0.13f) * 0.22f;
        VECTOR upDir = VNorm(VGet(swayX, 1.0f, swayZ));

        DrawFlameQuad3D(p.pos, upDir, camPos, curW * 0.78f, curW * 1.12f, curH, steamBottom, steamTop, m_smokeTexHandle);
    }

    // 描画ステートを確実に標準へ復元
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
    SetWriteZBuffer3D(TRUE);
}

void CustomEffectManager::SpawnMouseCatchCelebration(const VECTOR& mousePos, bool isFast, int comboCount)
{
    if (!m_effectEnabled)
    {
        return;
    }

    int safeCombo = (std::max)(1, comboCount);
    float comboBoost = 1.0f + static_cast<float>((std::min)(safeCombo, 5)) * 0.12f;

    // 1. 祝祭ゴールド＆レインボー衝撃波リング
    CatchShockwaveRing ring;
    ring.center = VGet(mousePos.x, mousePos.y + 1.0f, mousePos.z);
    ring.radius = 4.0f;
    ring.maxRadius = (isFast ? 75.0f : 60.0f) * comboBoost;
    ring.age = 0.0f;
    ring.maxLife = 24.0f;
    ring.isFast = isFast;
    ring.combo = safeCombo;
    m_catchRings.push_back(ring);

    // コンボ3以上または高速ネズミなら第2衝撃波（少し遅れて大きく展開）
    if (safeCombo >= 3 || isFast)
    {
        CatchShockwaveRing ring2 = ring;
        ring2.maxRadius *= 1.3f;
        ring2.maxLife = 28.0f;
        ring2.age = -3.0f; // 3フレーム遅れて展開
        m_catchRings.push_back(ring2);
    }

    // 2. 3D放射状サンバースト光線（捕獲瞬間の爆発的きらめき）
    int rayCount = 10 + (std::min)(safeCombo, 5) * 2 + (isFast ? 4 : 0);
    for (int i = 0; i < rayCount; ++i)
    {
        CatchSunburstRay ray;
        ray.center = VGet(mousePos.x, mousePos.y + 6.0f, mousePos.z);
        float theta = RandRange(0.0f, MathHelper::PI * 2.0f);
        float phi = RandRange(-0.3f, 1.2f); // 水平〜上空方向
        ray.dir = VNorm(VGet(std::cos(theta) * std::cos(phi), std::sin(phi), std::sin(theta) * std::cos(phi)));
        ray.length = 8.0f;
        ray.maxLength = RandRange(45.0f, 85.0f) * comboBoost;
        ray.age = 0.0f;
        ray.maxLife = RandRange(12.0f, 18.0f);
        ray.colorVariant = (isFast ? 1 : (i % 3));
        m_catchSunbursts.push_back(ray);
    }

    // 3. 3Dキラキラ星・ダイヤ紙吹雪パーティクル（祝祭ファウンテン）
    int starCount = 28 + (std::min)(safeCombo, 5) * 8 + (isFast ? 12 : 0);
    for (int s = 0; s < starCount; ++s)
    {
        CatchStarParticle p;
        p.pos = VGet(mousePos.x + RandRange(-3.0f, 3.0f), mousePos.y + RandRange(3.0f, 10.0f), mousePos.z + RandRange(-3.0f, 3.0f));
        float hAngle = RandRange(0.0f, MathHelper::PI * 2.0f);
        float hSpeed = RandRange(1.8f, 6.5f) * comboBoost;
        float vSpeed = RandRange(4.0f, 10.5f) * (isFast ? 1.2f : 1.0f);
        p.vel = VGet(std::cos(hAngle) * hSpeed, vSpeed, std::sin(hAngle) * hSpeed);
        p.size = RandRange(4.0f, 7.8f) * comboBoost;
        p.rot = RandRange(0.0f, MathHelper::PI * 2.0f);
        p.rotSpeed = RandRange(-0.35f, 0.35f);
        p.age = 0.0f;
        p.maxLife = RandRange(35.0f, 52.0f);
        p.colorVariant = isFast ? ((s % 2 == 0) ? 1 : 0) : (s % 5);
        p.shapeType = (s % 3 == 0) ? 1 : 0;
        m_catchStars.push_back(p);
    }

    // 4. コミカル昇天ソウル（天使ネズミ＆光輪）
    CatchAscendingSoul soul;
    soul.pos = VGet(mousePos.x, mousePos.y + 4.0f, mousePos.z);
    soul.age = 0.0f;
    soul.maxLife = 52.0f;
    soul.swayPhase = RandRange(0.0f, MathHelper::PI * 2.0f);
    soul.isFast = isFast;
    m_catchSouls.push_back(soul);
}

void CustomEffectManager::UpdateCatchCelebration()
{
    // 1. 衝撃波リング
    for (auto it = m_catchRings.begin(); it != m_catchRings.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_catchRings.erase(it);
        }
        else
        {
            if (it->age > 0.0f)
            {
                it->radius = MathHelper::Lerp(it->radius, it->maxRadius, 0.20f);
            }
            ++it;
        }
    }

    // 2. サンバースト光線
    for (auto it = m_catchSunbursts.begin(); it != m_catchSunbursts.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_catchSunbursts.erase(it);
        }
        else
        {
            it->length = MathHelper::Lerp(it->length, it->maxLength, 0.25f);
            ++it;
        }
    }

    // 3. キラキラ星・ダイヤ
    for (auto it = m_catchStars.begin(); it != m_catchStars.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_catchStars.erase(it);
        }
        else
        {
            // 重力
            it->vel.y -= 0.30f;
            it->vel.x *= 0.965f;
            it->vel.z *= 0.965f;
            it->pos = VAdd(it->pos, it->vel);

            // 地面バウンド
            if (it->pos.y < 2.0f && it->vel.y < 0.0f)
            {
                it->pos.y = 2.0f;
                it->vel.y = -it->vel.y * 0.45f;
            }

            it->rot += it->rotSpeed;
            ++it;
        }
    }

    // 4. 昇天ソウル
    for (auto it = m_catchSouls.begin(); it != m_catchSouls.end(); )
    {
        it->age += 1.0f;
        if (it->age >= it->maxLife)
        {
            it = m_catchSouls.erase(it);
        }
        else
        {
            it->pos.y += 1.4f;
            it->swayPhase += 0.16f;
            it->pos.x += std::sin(it->swayPhase) * 0.45f;
            ++it;
        }
    }
}

void CustomEffectManager::DrawCatchCelebration() const
{
    if (m_catchRings.empty() && m_catchSunbursts.empty() && m_catchStars.empty() && m_catchSouls.empty())
    {
        return;
    }

    // 1. 衝撃波リング描画（地面に沿う二重リング）
    for (const auto& ring : m_catchRings)
    {
        if (ring.age < 0.0f) continue;
        float progress = ring.age / ring.maxLife;
        int alpha = static_cast<int>((1.0f - progress) * 235.0f);
        if (alpha <= 0) continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        unsigned int outerCol = ring.isFast ? GetColor(60, 230, 255) : (ring.combo >= 3 ? GetColor(255, 140, 20) : GetColor(255, 225, 40));
        unsigned int innerCol = GetColor(255, 255, 255);

        constexpr int SEGS = 28;
        for (int i = 0; i < SEGS; ++i)
        {
            float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / SEGS);
            float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / SEGS);
            VECTOR p1 = VGet(ring.center.x + std::cos(a1) * ring.radius, ring.center.y, ring.center.z + std::sin(a1) * ring.radius);
            VECTOR p2 = VGet(ring.center.x + std::cos(a2) * ring.radius, ring.center.y, ring.center.z + std::sin(a2) * ring.radius);
            DrawLine3D(p1, p2, outerCol);

            // 内側コアリング
            float rInner = ring.radius * 0.88f;
            VECTOR ip1 = VGet(ring.center.x + std::cos(a1) * rInner, ring.center.y + 0.5f, ring.center.z + std::sin(a1) * rInner);
            VECTOR ip2 = VGet(ring.center.x + std::cos(a2) * rInner, ring.center.y + 0.5f, ring.center.z + std::sin(a2) * rInner);
            DrawLine3D(ip1, ip2, innerCol);
        }
    }

    // 2. サンバースト放射光線描画
    for (const auto& ray : m_catchSunbursts)
    {
        float progress = ray.age / ray.maxLife;
        int alpha = static_cast<int>((1.0f - progress) * 240.0f);
        if (alpha <= 0) continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        unsigned int col = (ray.colorVariant == 1) ? GetColor(80, 240, 255) : ((ray.colorVariant == 2) ? GetColor(255, 120, 200) : GetColor(255, 240, 60));

        VECTOR endP = VAdd(ray.center, VScale(ray.dir, ray.length));
        VECTOR startP = VAdd(ray.center, VScale(ray.dir, ray.length * 0.25f));
        DrawCapsule3D(startP, endP, 2.2f * (1.0f - progress * 0.5f), 6, col, col, TRUE);
    }

    // 3. キラキラ星・ダイヤ紙吹雪描画
    VECTOR camPos = GetCameraPosition();
    for (const auto& star : m_catchStars)
    {
        float progress = star.age / star.maxLife;
        int alpha = static_cast<int>((1.0f - progress * progress) * 255.0f);
        if (alpha <= 0) continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        unsigned int col;
        switch (star.colorVariant)
        {
        case 1: col = GetColor(60, 240, 255); break; // Cyan
        case 2: col = GetColor(255, 90, 190); break; // Pink
        case 3: col = GetColor(255, 255, 255); break; // White
        case 4: col = GetColor(100, 255, 150); break; // Lime
        default: col = GetColor(255, 225, 40); break; // Gold
        }

        float sz = star.size * (1.0f - progress * 0.4f);

        VECTOR toCam = VNorm(VSub(camPos, star.pos));
        VECTOR up = VGet(0.0f, 1.0f, 0.0f);
        VECTOR right = VNorm(VCross(toCam, up));
        up = VNorm(VCross(right, toCam));

        float cr = std::cos(star.rot);
        float sr = std::sin(star.rot);
        VECTOR axisX = VAdd(VScale(right, cr), VScale(up, sr));
        VECTOR axisY = VAdd(VScale(right, -sr), VScale(up, cr));

        if (star.shapeType == 0)
        {
            // 4点クロススター（中心球＋4方向の光針）
            DrawSphere3D(star.pos, sz * 0.45f, 6, col, col, TRUE);
            DrawLine3D(VSub(star.pos, VScale(axisX, sz * 1.5f)), VAdd(star.pos, VScale(axisX, sz * 1.5f)), col);
            DrawLine3D(VSub(star.pos, VScale(axisY, sz * 1.5f)), VAdd(star.pos, VScale(axisY, sz * 1.5f)), col);
        }
        else
        {
            // 3Dダイヤモンド（菱形ビルボード）
            VECTOR dTop    = VAdd(star.pos, VScale(axisY, sz * 1.3f));
            VECTOR dBottom = VSub(star.pos, VScale(axisY, sz * 1.3f));
            VECTOR dLeft   = VSub(star.pos, VScale(axisX, sz * 0.8f));
            VECTOR dRight  = VAdd(star.pos, VScale(axisX, sz * 0.8f));
            DrawTriangle3D(dTop, dRight, dBottom, col, TRUE);
            DrawTriangle3D(dTop, dBottom, dLeft, col, TRUE);
            DrawLine3D(dTop, dRight, GetColor(255, 255, 255));
            DrawLine3D(dRight, dBottom, GetColor(255, 255, 255));
            DrawLine3D(dBottom, dLeft, GetColor(255, 255, 255));
            DrawLine3D(dLeft, dTop, GetColor(255, 255, 255));
        }
    }

    // 4. コミカル昇天ソウル（天使ネズミ光輪＆スター）描画
    for (const auto& soul : m_catchSouls)
    {
        float progress = soul.age / soul.maxLife;
        int alpha = static_cast<int>((1.0f - progress) * 220.0f);
        if (alpha <= 0) continue;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        unsigned int haloCol = soul.isFast ? GetColor(80, 240, 255) : GetColor(255, 230, 80);
        unsigned int soulCol = GetColor(255, 255, 255);

        float haloY = soul.pos.y + 12.0f;
        float hRadius = 7.5f;
        constexpr int HSEGS = 16;
        for (int i = 0; i < HSEGS; ++i)
        {
            float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / HSEGS);
            float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / HSEGS);
            VECTOR p1 = VGet(soul.pos.x + std::cos(a1) * hRadius, haloY, soul.pos.z + std::sin(a1) * hRadius);
            VECTOR p2 = VGet(soul.pos.x + std::cos(a2) * hRadius, haloY, soul.pos.z + std::sin(a2) * hRadius);
            DrawLine3D(p1, p2, haloCol);
        }

        DrawSphere3D(soul.pos, 5.0f, 8, soulCol, soulCol, TRUE);

        float wingFlap = std::sin(soul.swayPhase * 3.5f) * 4.0f;
        VECTOR leftWingTip  = VGet(soul.pos.x - 7.5f, soul.pos.y + wingFlap, soul.pos.z);
        VECTOR rightWingTip = VGet(soul.pos.x + 7.5f, soul.pos.y + wingFlap, soul.pos.z);
        DrawLine3D(soul.pos, leftWingTip, GetColor(255, 255, 255));
        DrawLine3D(soul.pos, rightWingTip, GetColor(255, 255, 255));
    }
}
