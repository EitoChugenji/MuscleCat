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

    // 1. タックル突進エフェクト（進行方向に射出されるアタックエフェクト: スケールを調整して猫のPush動作を見えやすく）
    m_tackleResHandle = LoadEffekseerEffect("Resource/Effects/Attack/01_NextSoft01/RightAttack.efk", 6.5f);

    // 2. 筋トレ成功エフェクト（モデル周囲の光バリア: 猫を覆い隠さない適切なスケールに調整）
    m_pumpSuccessResHandle = LoadEffekseerEffect("Resource/Effects/Shadow/Barrior01.efkefc", 6.0f);

    // 3. マッスルオーラエフェクト（足元から立ち昇るオーラ: 猫の全身が黒煙に覆われないよう足元に調整）
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

    if (m_pumpSuccessResHandle != -1)
    {
        DeleteEffekseerEffect(m_pumpSuccessResHandle);
        m_pumpSuccessResHandle = -1;
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
        // エフェクト無効時は現在再生中のオーラを停止
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

void EffectManager::PlayPumpSuccessEffect(const VECTOR& pos, int repCount)
{
    // 筋トレ時のエフェクト無効化
    (void)pos;
    (void)repCount;
    return;
}

void EffectManager::UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount)
{
    // 筋トレ・マッスルオーラ無効化
    (void)pos;
    (void)isActive;
    (void)repCount;
    StopMuscleAura();
    return;
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