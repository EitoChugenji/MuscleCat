// ============================================================================
// @file   ModelManager.h
// @brief  3Dモデル(.mv1)のキャッシュ管理およびプロシージャル代替描画クラス
// ============================================================================
#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

// ============================================================================
// ModelManager クラス（シングルトン）
// 3Dモデルファイルの重複ロードを防ぐキャッシュ機構と、モデルファイルが存在しない環境でも
// ゲームが完全に動作するよう3Dプリミティブ（球・カプセル・コーン・立方体）を組み合わせた
// フォールバック描画機能を提供する
// ============================================================================
class ModelManager
{
private:
    // ファイルパスをキー、DxLibのMV1モデルハンドルを値とするキャッシュテーブル
    std::unordered_map<std::string, int> m_modelHandles;

    ModelManager() = default;
    ~ModelManager() = default;

public:
    // シングルトンインスタンスの取得
    static ModelManager& GetInstance()
    {
        static ModelManager instance;
        return instance;
    }

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    // 初期化およびキャッシュ済み全モデルの解放
    void Init();
    void Release();

    // ------------------------------------------------------------------------
    // 指定パスの3Dモデルを読み込み、ハンドルを返す（既に読み込み済みならキャッシュを返す）
    // ファイルが存在しない場合は -1 を返す
    // ------------------------------------------------------------------------
    int LoadModelHandle(const std::string& path);

    // ------------------------------------------------------------------------
    // モデルが存在すれば指定座標・回転・スケールで描画し true を返す
    // 存在しなければ false を返し、呼び出し側でフォールバック描画へ切り替えられる
    // ------------------------------------------------------------------------
    bool DrawModelIfLoaded(
        const std::string& path,
        const VECTOR& pos,
        float rotY,
        float scale,
        float rotX = 0.0f,
        float rotZ = 0.0f
    );

    // ------------------------------------------------------------------------
    // ローカルピッチ・ロール・非一様スケール（Squash & Stretch）を合成した変換行列で
    // 3Dモデルを描画する（ネズミのホッピング歩行や気絶横倒し演出に使用）
    // ------------------------------------------------------------------------
    bool DrawModelWithTransform(
        const std::string& path,
        const VECTOR& pos,
        float rotY,
        const VECTOR& scaleVec,
        float localPitchX = 0.0f,
        float localRollZ = 0.0f
    );

    // ------------------------------------------------------------------------
    // 3Dモデル未配置時のプロシージャル・フォールバック描画メソッド群
    // ------------------------------------------------------------------------

    // プレイヤー（マッスルねこ）のプリミティブ描画
    void DrawFallbackCat(
        const VECTOR& pos,
        float rotY,
        int repCount,
        bool isSoreness,
        bool isPouncing,
        float animTime
    );

    // 通常ネズミのプリミティブ描画
    void DrawFallbackNormalMouse(
        const VECTOR& pos,
        float rotY,
        float animTime,
        float localPitchX = 0.0f,
        float localRollZ = 0.0f,
        bool isStunned = false
    );

    // 高速逃走ネズミのプリミティブ描画
    void DrawFallbackFastMouse(
        const VECTOR& pos,
        float rotY,
        float animTime,
        float localPitchX = 0.0f,
        float localRollZ = 0.0f,
        bool isStunned = false
    );

    // 障害物（家具・器具）のプリミティブ描画
    void DrawFallbackObstacle(
        const VECTOR& pos,
        float width,
        float height,
        float depth,
        unsigned int mainColor,
        unsigned int frameColor,
        const std::string& name
    );

    // ステージ（木目フローリング床・ラグ・壁）のプリミティブ描画
    void DrawFallbackStage(float halfW, float halfD, float wallH);
};

