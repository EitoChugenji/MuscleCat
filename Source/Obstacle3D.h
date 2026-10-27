// ============================================================================
// @file   Obstacle3D.h
// @brief  ステージ内に配置される静的障害物（家具・トレーニング器具）クラス定義
// ============================================================================
#pragma once
#include "GameObject3D.h"
#include <string>

// ============================================================================
// Obstacle3D クラス（GameObject3D 派生）
// 直方体（AABB）の当たり判定を持ち、3Dモデル描画またはフォールバック描画を行う
// ============================================================================
class Obstacle3D : public GameObject3D
{
private:
    float        m_width;       // X軸方向の横幅
    float        m_height;      // Y軸方向の高さ
    float        m_depth;       // Z軸方向の奥行き
    std::string  m_name;        // 頭上に表示する器具・家具名（空文字列なら非表示）
    std::string  m_modelPath;   // 3Dモデル(.mv1)のファイルパス
    unsigned int m_mainColor;   // フォールバック描画時の本体カラー
    unsigned int m_frameColor;  // フォールバック描画時の枠線・文字カラー
    float        m_scale;       // 3Dモデルの表示スケール
    VECTOR       m_modelOffset; // モデル原点補正用の座標オフセット

public:
    Obstacle3D(
        const VECTOR& pos,
        float width,
        float height,
        float depth,
        const std::string& name = "",
        const std::string& modelPath = "",
        unsigned int mainColor = 0,
        unsigned int frameColor = 0,
        float scale = 1.0f,
        float rotY = 0.0f,
        const VECTOR& modelOffset = VGet(0.0f, 0.0f, 0.0f)
    );
    virtual ~Obstacle3D() = default;

    // 静的オブジェクトのため毎フレームの移動更新は行わない
    void Update() override {}

    // 3Dモデル（またはフォールバック直方体）の描画
    void Draw3D() override;

    // 頭上ネームプレートの2D投影描画
    void Draw2D() override;

    // 寸法・名称ゲッター
    float GetWidth() const            { return m_width; }
    float GetHeight() const           { return m_height; }
    float GetDepth() const            { return m_depth; }
    const std::string& GetName() const { return m_name; }

    // 球・円柱オブジェクト（猫やネズミ）がこの直方体と重なっている場合、外周へ押し出す
    bool ResolveCollision(VECTOR& outPos, float radius) const;
};
