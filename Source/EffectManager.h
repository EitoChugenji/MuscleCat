#pragma once

#include "DxLib.h"
#include <string>

// EffectManager クラス (シングルトン)
// Effekseer 3Dエフェクトの初期化・更新・描画・再生を一括管理
class EffectManager
{
private:
    // エフェクトリソースハンドル
    int m_tackleResHandle = -1;       // タックル突進エフェクト
    int m_pumpSuccessResHandle = -1;   // 筋トレ成功エフェクト（変身オーラ・全身発光）
    int m_pumpRingResHandle = -1;      // 筋トレ成功・足元リング衝撃波
    int m_pumpSparkResHandle = -1;     // 筋トレ成功・舞い上がる花火スパーク粒子
    int m_customLevelUpResHandle = -1; // ユーザー自作レベルアップエフェクト（Custom/LevelUp.efkefc）
    int m_muscleAuraResHandle = -1;   // 筋トレ・マッスルオーラ

    // 捕獲エフェクトリソースハンドル
    int m_catchHitResHandle = -1;      // 捕獲ヒット閃光
    int m_catchRingResHandle = -1;     // 捕獲リング衝撃波
    int m_catchSparkResHandle = -1;    // 捕獲スパーク星
    int m_customCatchResHandle = -1;   // 自作捕獲エフェクト（Custom/Catch.efkefc）

    // 再生中ハンドル（位置更新や停止用）
    int m_activeAuraHandle = -1;

    // エフェクト有効/無効フラグ（デバッグチート切替用）
    bool m_effectEnabled = true;

    EffectManager() = default;
    ~EffectManager() = default;

public:
    static EffectManager& GetInstance()
    {
        static EffectManager instance;
        return instance;
    }

    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    void Init();
    void Release();
    void Update();
    void SyncCamera();
    void Draw3D();

    // エフェクト表示の有効/無効設定
    void SetEffectEnabled(bool enabled);
    bool IsEffectEnabled() const;
    void ToggleEffectEnabled();

    // タックル突進エフェクト再生
    void PlayTackleEffect(const VECTOR& pos, const VECTOR& dir);

    // 筋トレ成功（モデル周囲発光）エフェクト再生
    void PlayCatchEffect(const VECTOR& pos, bool isFast = false);
    void PlayPumpSuccessEffect(const VECTOR& pos, int repCount);

    // 筋トレQTE中・マッチョ中のオーラエフェクト再生/追従
    void UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount);
    void StopMuscleAura();
};