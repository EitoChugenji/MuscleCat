// ============================================================================
// @file   EffectManager.h
// @brief  Effekseer for DXLib を用いた3Dパーティクルエフェクト管理クラス
// ============================================================================
#pragma once
#include "DxLib.h"
#include <string>

// ============================================================================
// EffectManager クラス（シングルトン）
// Effekseerランタイムの初期化・更新・カメラ同期・3D描画・各エフェクト再生を一括管理する
// ============================================================================
class EffectManager
{
private:
    // ------------------------------------------------------------------------
    // Effekseer エフェクトリソースハンドル（LoadEffekseerEffect で取得）
    // ------------------------------------------------------------------------
    int m_tackleResHandle        = -1; // タックル（スタン攻撃）突進エフェクト
    int m_pumpSuccessResHandle   = -1; // 筋トレ成功エフェクト（バリア発光）
    int m_pumpRingResHandle      = -1; // 筋トレ成功・足元リング衝撃波（拡張用）
    int m_pumpSparkResHandle     = -1; // 筋トレ成功・舞い上がるスパーク粒子（拡張用）
    int m_customLevelUpResHandle = -1; // ユーザー自作レベルアップエフェクト（拡張用）
    int m_muscleAuraResHandle    = -1; // 筋トレ中・パンプアップ中に纏うマッスルオーラ

    // ------------------------------------------------------------------------
    // ネズミ捕獲時エフェクトリソースハンドル
    // ------------------------------------------------------------------------
    int m_catchHitResHandle      = -1; // 捕獲瞬間のヒット閃光（LightAttackEffect）
    int m_catchRingResHandle     = -1; // 捕獲リング衝撃波（拡張用）
    int m_catchSparkResHandle    = -1; // 捕獲スパーク星（拡張用）
    int m_customCatchResHandle   = -1; // ユーザー自作捕獲エフェクト（Custom/Catch.efkefc）

    // ------------------------------------------------------------------------
    // 現在再生中のエフェクトインスタンスハンドル（追従座標更新・停止用）
    // ------------------------------------------------------------------------
    int m_activeAuraHandle       = -1; // プレイヤーに追従する常駐オーラの再生ハンドル

    // エフェクト描画の有効/無効フラグ（F1デバッグメニューの[6]キーで切替可能）
    bool m_effectEnabled         = true;

    EffectManager() = default;
    ~EffectManager() = default;

public:
    // シングルトンインスタンスの取得
    static EffectManager& GetInstance()
    {
        static EffectManager instance;
        return instance;
    }

    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    // 初期化・リソース解放・毎フレーム更新・カメラ同期・3D描画
    void Init();
    void Release();
    void Update();
    void SyncCamera();
    void Draw3D();

    // エフェクト表示の有効/無効制御（デバッグ・パフォーマンス計測用）
    void SetEffectEnabled(bool enabled);
    bool IsEffectEnabled() const;
    void ToggleEffectEnabled();

    // タックル（スタン攻撃）発動時の前方衝撃波エフェクトを再生する
    void PlayTackleEffect(const VECTOR& pos, const VECTOR& dir);

    // ネズミ捕獲時の閃光エフェクトを再生する（高速ネズミ捕獲時は拡大）
    void PlayCatchEffect(const VECTOR& pos, bool isFast = false);

    // 筋トレQTE成功時の発光エフェクトを再生する
    void PlayPumpSuccessEffect(const VECTOR& pos, int repCount);

    // 筋トレ中・マッチョ状態中のオーラエフェクトをプレイヤー座標へ追従更新する
    void UpdateMuscleAura(const VECTOR& pos, bool isActive, int repCount);

    // 再生中のマッスルオーラを停止する
    void StopMuscleAura();
};
