// ============================================================================
// @file   EffectManager.cpp
// @brief  EffectManager クラスの実装（Effekseerの初期化・リソース管理・再生制御）
// ============================================================================
#include "EffectManager.h"
#include <EffekseerForDXLib.h>
#include <cmath>

// ============================================================================
// Effekseerの初期化および各エフェクトファイル(.efk / .efkefc)の読み込み
// ============================================================================
void EffectManager::Init()
{
    // Effekseerランタイムの初期化（同時最大パーティクル数: 8000）
    if (Effekseer_Init(8000) == -1)
    {
        return;
    }

    // フルスクリーン/ウィンドウ切り替え時のグラフィックシステムリセットを防止
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

    // DirectXデバイスロスト時のコールバック関数を設定
    Effekseer_SetGraphicsDeviceLostCallbackFunctions();

    // 空間歪み（ディストーション）エフェクトの初期化
    Effekseer_InitDistortion();

    // 1. タックル突進エフェクト（進行方向へ射出されるアタック衝撃波）
    m_tackleResHandle = LoadEffekseerEffect("Resource/Effects/Attack/01_NextSoft01/RightAttack.efk", 6.5f);

    // 2. ネズミ捕獲エフェクト
    // ① 捕獲瞬間のヒットフラッシュ（LightAttackEffect）
    m_catchHitResHandle = LoadEffekseerEffect("Resource/Effects/Attack/01_NextSoft01/LightAttackEffect.efk", 4.0f);

    // ② ユーザー自作捕獲エフェクト用スロット（配置されていれば最優先で重ねて再生）
    m_customCatchResHandle = LoadEffekseerEffect("Resource/Effects/Custom/Catch.efkefc", 4.0f);
    if (m_customCatchResHandle == -1)
    {
        m_customCatchResHandle = LoadEffekseerEffect("Resource/Effects/Custom/Catch.efk", 4.0f);
    }

    // 3. 筋トレ成功エフェクトおよび常駐マッスルオーラエフェクトの読み込み
    m_pumpSuccessResHandle = LoadEffekseerEffect("Resource/Effects/Shadow/Barrior01.efkefc", 6.0f);
    m_muscleAuraResHandle  = LoadEffekseerEffect("Resource/Effects/Shadow/Aura01.efkefc", 1.2f);
}

// ============================================================================
// 全エフェクトリソースの解放およびEffekseerの終了処理
// ============================================================================
void EffectManager::Release()
{
    // 再生中の常駐オーラがあれば先に停止する
    StopMuscleAura();

    // 読み込み済みエフェクトリソースを個別に解放
    if (m_tackleResHandle != -1)        { DeleteEffekseerEffect(m_tackleResHandle);        m_tackleResHandle = -1; }
    if (m_catchHitResHandle != -1)      { DeleteEffekseerEffect(m_catchHitResHandle);      m_catchHitResHandle = -1; }
    if (m_customCatchResHandle != -1)   { DeleteEffekseerEffect(m_customCatchResHandle);   m_customCatchResHandle = -1; }
    if (m_pumpSuccessResHandle != -1)   { DeleteEffekseerEffect(m_pumpSuccessResHandle);   m_pumpSuccessResHandle = -1; }
    if (m_pumpRingResHandle != -1)      { DeleteEffekseerEffect(m_pumpRingResHandle);      m_pumpRingResHandle = -1; }
    if (m_pumpSparkResHandle != -1)     { DeleteEffekseerEffect(m_pumpSparkResHandle);     m_pumpSparkResHandle = -1; }
    if (m_customLevelUpResHandle != -1) { DeleteEffekseerEffect(m_customLevelUpResHandle); m_customLevelUpResHandle = -1; }
    if (m_muscleAuraResHandle != -1)    { DeleteEffekseerEffect(m_muscleAuraResHandle);    m_muscleAuraResHandle = -1; }

    // Effekseerランタイムの終了処理
    Effkseer_End();
}

// ============================================================================
// 毎フレームのEffekseer時間進行更新
// ============================================================================
void EffectManager::Update()
{
    UpdateEffekseer3D();
}

// ============================================================================
// DxLibの3Dカメラ行列（ビュー・プロジェクション）をEffekseerへ同期する
// ============================================================================
void EffectManager::SyncCamera()
{
    Effekseer_Sync3DSetting();
}

// ============================================================================
// エフェクト表示の有効・無効切り替え
// ============================================================================
void EffectManager::SetEffectEnabled(bool enabled)
{
    m_effectEnabled = enabled;

    if (!m_effectEnabled)
    {
        StopMuscleAura();
    }
}

bool EffectManager::IsEffectEnabled() const
{
    return m_effectEnabled;
}

void EffectManager::ToggleEffectEnabled()
{
    SetEffectEnabled(!m_effectEnabled);
}

// ============================================================================
// 3D空間上のEffekseerエフェクト一括描画
// ============================================================================
void EffectManager::Draw3D()
{
    if (!m_effectEnabled)
    {
        return;
    }

    DrawEffekseer3D();
}

// ============================================================================
// タックル突進エフェクトの再生
// 猫の突進モーションが隠れないよう、進行方向の前方（+14.0f）にオフセットして発生させる
// ============================================================================
void EffectManager::PlayTackleEffect(const VECTOR& pos, const VECTOR& dir)
{
    if (!m_effectEnabled || m_tackleResHandle == -1)
    {
        return;
    }

    // 進行方向ベクトルからY軸回転角（ラジアン）を算出
    float rotY = std::atan2(dir.x, dir.z);

    int playHandle = PlayEffekseer3DEffect(m_tackleResHandle);
    if (playHandle != -1)
    {
        float frontX = pos.x + dir.x * 14.0f;
        float frontZ = pos.z + dir.z * 14.0f;
        SetPosPlayingEffekseer3DEffect(playHandle, frontX, pos.y + 7.0f, frontZ);
        SetRotationPlayingEffekseer3DEffect(playHandle, 0.0f, rotY, 0.0f);
    }
}

// ============================================================================
// ネズミ捕獲時のヒットフラッシュエフェクト再生
// 高速ネズミ（isFast == true）捕獲時はスケールを1.35倍に拡大して達成感を強調する
// ============================================================================
void EffectManager::PlayCatchEffect(const VECTOR& pos, bool isFast)
{
    if (!m_effectEnabled)
    {
        return;
    }

    float fastMultiplier = isFast ? 1.35f : 1.0f;

    // ユーザー自作捕獲エフェクトが存在する場合は同時に再生
    if (m_customCatchResHandle != -1)
    {
        int customPlay = PlayEffekseer3DEffect(m_customCatchResHandle);
        if (customPlay != -1)
        {
            float scale = 3.5f * fastMultiplier;
            SetPosPlayingEffekseer3DEffect(customPlay, pos.x, pos.y + 6.0f, pos.z);
            SetScalePlayingEffekseer3DEffect(customPlay, scale, scale, scale);
        }
    }

    // 捕獲瞬間のヒット閃光をネズミの中心高さ（pos.y + 8.0f）で再生
    if (m_catchHitResHandle != -1)
    {
        int hitPlay = PlayEffekseer3DEffect(m_catchHitResHandle);
        if (hitPlay != -1)
        {
            float scale = 3.2f * fastMultiplier;
            SetPosPlayingEffekseer3DEffect(hitPlay, pos.x, pos.y + 8.0f, pos.z);
            SetScalePlayingEffekseer3DEffect(hitPlay, scale, scale, scale);
        }
    }
}

// ============================================================================
// 筋トレ成功時の発光バリアエフェクト再生（筋トレレベルに応じて拡大）
// ============================================================================
void EffectManager::PlayPumpSuccessEffect(const VECTOR& pos, int repCount)
{
    if (!m_effectEnabled || m_pumpSuccessResHandle == -1)
    {
        return;
    }

    int playHandle = PlayEffekseer3DEffect(m_pumpSuccessResHandle);
    if (playHandle != -1)
    {
        float scale = 4.5f + static_cast<float>(repCount) * 0.25f;
        SetPosPlayingEffekseer3DEffect(playHandle, pos.x, pos.y + 10.0f, pos.z);
        SetScalePlayingEffekseer3DEffect(playHandle, scale, scale, scale);
    }
}

// ============================================================================
// 筋トレ中・パンプアップ状態中に纏うマッスルオーラの追従更新
// ============================================================================
void EffectManager::UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount)
{
    if (!m_effectEnabled || !isActive || m_muscleAuraResHandle == -1)
    {
        StopMuscleAura();
        return;
    }

    // 未再生または再生終了していたらループ再生し直す
    if (m_activeAuraHandle == -1 || IsEffekseer3DEffectPlaying(m_activeAuraHandle) == -1)
    {
        m_activeAuraHandle = PlayEffekseer3DEffect(m_muscleAuraResHandle);
    }

    // プレイヤーの足元座標および現在の筋トレレベルに合わせたスケールを反映
    if (m_activeAuraHandle != -1)
    {
        float auraScale = 1.0f + static_cast<float>(repCount) * 0.1f;
        SetPosPlayingEffekseer3DEffect(m_activeAuraHandle, pos.x, pos.y + 2.0f, pos.z);
        SetScalePlayingEffekseer3DEffect(m_activeAuraHandle, auraScale, auraScale, auraScale);
    }
}

// ============================================================================
// 再生中のマッスルオーラを強制停止する
// ============================================================================
void EffectManager::StopMuscleAura()
{
    if (m_activeAuraHandle != -1)
    {
        if (IsEffekseer3DEffectPlaying(m_activeAuraHandle) != -1)
        {
            StopEffekseer3DEffect(m_activeAuraHandle);
        }

        m_activeAuraHandle = -1;
    }
}
