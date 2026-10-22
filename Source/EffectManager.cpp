#include "EffectManager.h"
#include <EffekseerForDXLib.h>
#include <cmath>

void EffectManager::Init()
{
    // Effekseerの初期化（最大パーティクル数8000）
    if (Effekseer_Init(8000) == -1)
    {
        return;
    }

    // 画面モード変更時のグラフィックシステムリセット防止
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

    // デバイスロスト時のコールバック設定
    Effekseer_SetGraphicsDeviceLostCallbackFunctions();

    // 歪みエフェクトの初期化
    Effekseer_InitDistortion();

    // 1. タックル突進エフェクト（進行方向に射出されるアタックエフェクト）
    m_tackleResHandle = LoadEffekseerEffect("Resource/Effects/Attack/01_NextSoft01/RightAttack.efk", 6.5f);

    // 2. ネズミ捕獲エフェクト
    // ① 捕獲ヒット閃光（LightAttackEffect: 捕獲瞬間のクッキリしたヒットフラッシュ）
    m_catchHitResHandle = LoadEffekseerEffect("Resource/Effects/Attack/01_NextSoft01/LightAttackEffect.efk", 4.0f);

    // ② ユーザー自作捕獲エフェクト用スロット（Resource/Effects/Custom/Catch.efkefc または Catch.efk があれば最優先再生）
    m_customCatchResHandle = LoadEffekseerEffect("Resource/Effects/Custom/Catch.efkefc", 4.0f);
    if (m_customCatchResHandle == -1)
    {
        m_customCatchResHandle = LoadEffekseerEffect("Resource/Effects/Custom/Catch.efk", 4.0f);
    }

    // 筋トレ成功エフェクト＆マッスルオーラエフェクトのロードを追加
    m_pumpSuccessResHandle = LoadEffekseerEffect("Resource/Effects/Shadow/Barrior01.efkefc", 6.0f);
    m_muscleAuraResHandle = LoadEffekseerEffect("Resource/Effects/Shadow/Aura01.efkefc", 1.2f);
}

void EffectManager::Release()
{
    // 再生中オーラの停止
    StopMuscleAura();

    // エフェクトリソースの解放
    if (m_tackleResHandle != -1)
    {
        DeleteEffekseerEffect(m_tackleResHandle);
        m_tackleResHandle = -1;
    }

    if (m_catchHitResHandle != -1)
    {
        DeleteEffekseerEffect(m_catchHitResHandle);
        m_catchHitResHandle = -1;
    }



    if (m_customCatchResHandle != -1)
    {
        DeleteEffekseerEffect(m_customCatchResHandle);
        m_customCatchResHandle = -1;
    }

    if (m_pumpSuccessResHandle != -1)
    {
        DeleteEffekseerEffect(m_pumpSuccessResHandle);
        m_pumpSuccessResHandle = -1;
    }

    if (m_pumpRingResHandle != -1)
    {
        DeleteEffekseerEffect(m_pumpRingResHandle);
        m_pumpRingResHandle = -1;
    }

    if (m_pumpSparkResHandle != -1)
    {
        DeleteEffekseerEffect(m_pumpSparkResHandle);
        m_pumpSparkResHandle = -1;
    }

    if (m_customLevelUpResHandle != -1)
    {
        DeleteEffekseerEffect(m_customLevelUpResHandle);
        m_customLevelUpResHandle = -1;
    }

    if (m_muscleAuraResHandle != -1)
    {
        DeleteEffekseerEffect(m_muscleAuraResHandle);
        m_muscleAuraResHandle = -1;
    }

    // Effekseer終了処理
    Effkseer_End();
}

void EffectManager::Update()
{
    // Effekseerエフェクトの内部時間更新
    UpdateEffekseer3D();
}

void EffectManager::SyncCamera()
{
    // DxLibの3Dカメラ設定とEffekseerのカメラ設定を同期
    Effekseer_Sync3DSetting();
}

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

void EffectManager::Draw3D()
{
    if (!m_effectEnabled)
    {
        return;
    }

    // 3Dエフェクトの描画
    DrawEffekseer3D();
}

void EffectManager::PlayTackleEffect(const VECTOR& pos, const VECTOR& dir)
{
    if (!m_effectEnabled || m_tackleResHandle == -1)
    {
        return;
    }

    // タックル突進方向に合わせた角度を計算
    float rotY = std::atan2(dir.x, dir.z);

    // 猫自身（Pushモーション）を覆い隠さないよう、猫の手の前方（突進方向）に少し離して再生
    int playHandle = PlayEffekseer3DEffect(m_tackleResHandle);

    if (playHandle != -1)
    {
        float frontX = pos.x + dir.x * 14.0f;
        float frontZ = pos.z + dir.z * 14.0f;
        SetPosPlayingEffekseer3DEffect(playHandle, frontX, pos.y + 7.0f, frontZ);
        SetRotationPlayingEffekseer3DEffect(playHandle, 0.0f, rotY, 0.0f);
    }
}

void EffectManager::PlayCatchEffect(const VECTOR& pos, bool isFast)
{
    if (!m_effectEnabled)
    {
        return;
    }

    // 高速ネズミ捕獲時は少しエフェクトを大きめにしてメリハリをつける
    float fastMultiplier = isFast ? 1.35f : 1.0f;

    // 自作捕獲エフェクトがある場合はそれを最優先再生
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

    // ① 捕獲ヒット閃光（ネズミの体中心でバシュッと光る）
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

void EffectManager::UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount)
{
    if (!m_effectEnabled || !isActive || m_muscleAuraResHandle == -1)
    {
        StopMuscleAura();
        return;
    }
    if (m_activeAuraHandle == -1 || IsEffekseer3DEffectPlaying(m_activeAuraHandle) == -1)
    {
        m_activeAuraHandle = PlayEffekseer3DEffect(m_muscleAuraResHandle);
    }
    if (m_activeAuraHandle != -1)
    {
        float auraScale = 1.0f + static_cast<float>(repCount) * 0.1f;
        SetPosPlayingEffekseer3DEffect(m_activeAuraHandle, pos.x, pos.y + 2.0f, pos.z);
        SetScalePlayingEffekseer3DEffect(m_activeAuraHandle, auraScale, auraScale, auraScale);
    }
}

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
