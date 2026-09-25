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

    // 1. タックル用衝撃波エフェクト（進行方向に射出される衝撃波リング）
    m_tackleResHandle = LoadEffekseerEffect("Resource/Effects/Attack/RightAttack.efk", 20.0f);

    // 2. 筋トレ成功パンプアップエフェクト（上空への激しい光柱）
    m_pumpSuccessResHandle = LoadEffekseerEffect("Resource/Effects/Basic/Laser01.efkefc", 15.0f);

    // 3. マッスルオーラエフェクト（足元から立ち昇るオーラ）
    m_muscleAuraResHandle = LoadEffekseerEffect("Resource/Effects/Shadow/Aura01.efkefc", 12.0f);
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

void EffectManager::Draw3D()
{
    // 3Dエフェクトの描画
    DrawEffekseer3D();
}

void EffectManager::PlayTackleEffect(const VECTOR& pos, const VECTOR& dir)
{
    if (m_tackleResHandle == -1)
    {
        return;
    }

    // タックル突進方向に合わせた角度を計算
    float rotY = std::atan2(dir.x, dir.z);

    // 猫の足元より少し高い位置（胸部付近）に再生
    int playHandle = PlayEffekseer3DEffect(m_tackleResHandle);

    if (playHandle != -1)
    {
        SetPosPlayingEffekseer3DEffect(playHandle, pos.x, pos.y + 12.0f, pos.z);
        SetRotationPlayingEffekseer3DEffect(playHandle, 0.0f, rotY, 0.0f);
    }
}

void EffectManager::PlayPumpSuccessEffect(const VECTOR& pos, int repCount)
{
    if (m_pumpSuccessResHandle == -1)
    {
        return;
    }

    // Rep数に応じて光柱のスケールを強化（バカゲー風演出）
    float scale = (14.0f + static_cast<float>(repCount) * 1.2f) * 0.3f;

    int playHandle = PlayEffekseer3DEffect(m_pumpSuccessResHandle);

    if (playHandle != -1)
    {
        SetPosPlayingEffekseer3DEffect(playHandle, pos.x, pos.y, pos.z);
        SetScalePlayingEffekseer3DEffect(playHandle, scale, scale, scale);
    }
}

void EffectManager::UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount)
{
    if (!isActive || m_muscleAuraResHandle == -1)
    {
        StopMuscleAura();
        return;
    }

    // オーラが再生されていない場合は新規再生
    if (m_activeAuraHandle == -1 || IsEffekseer3DEffectPlaying(m_activeAuraHandle) == -1)
    {
        m_activeAuraHandle = PlayEffekseer3DEffect(m_muscleAuraResHandle);
    }

    // 再生中のオーラ位置とサイズを猫の現在位置・Rep数に追従
    if (m_activeAuraHandle != -1)
    {
        SetPosPlayingEffekseer3DEffect(m_activeAuraHandle, pos.x, pos.y, pos.z);

        float auraScale = 10.0f + static_cast<float>(repCount) * 1.0f;
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
