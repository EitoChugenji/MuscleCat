// ============================================================================
// @file   Skybox3D.h
// @brief  3D森のスカイボックス・スカイドーム描画および森林環境パーティクル管理
// ============================================================================
#pragma once
#include "DxLib.h"
#include <vector>

// ============================================================================
// 木の葉パーティクル構造体（森の中を舞い落ちる風情演出用）
// ============================================================================
struct ForestLeafParticle
{
    VECTOR pos;          // 現在のワールド座標
    VECTOR velocity;     // 移動速度
    VECTOR rot;          // 3軸回転角度
    VECTOR rotSpeed;     // 回転速度
    float  size;         // 葉の大きさ
    float  swaySpeed;    // 左右揺らぎの速度
    float  swayPhase;    // 左右揺らぎの初期位相
    unsigned int color;  // 葉の色（新緑・深緑・黄緑など）
};

// ============================================================================
// Skybox3D クラス
// カメラを中心に描画される巨大な森のスカイドーム（抜けるような青空・太陽の木漏れ日・
// 遠く連なる山並みと森林パノラマ・頭上の樹冠キャノピー）および舞い散る木の葉を管理する
// ============================================================================
class Skybox3D
{
private:
    int m_textureHandle = -1;  // 生成した森のパノラマテクスチャハンドル
    int m_sliceCount    = 48;  // 水平方向（経度）分割数
    int m_ringCount     = 16;  // 垂直方向（緯度）分割数
    float m_radius      = 45000.0f; // スカイドーム半径

    // 原点基準の基本メッシュデータ
    std::vector<VERTEX3D>       m_baseVertices;
    std::vector<VERTEX3D>       m_drawVertices;
    std::vector<unsigned short> m_indices;

    // 森林環境パーティクル（舞い落ちる木の葉）
    std::vector<ForestLeafParticle> m_leaves;
    int m_leafCount = 40;

    void GenerateForestTexture();
    void BuildDomeMesh();
    void InitLeaves();
    void UpdateLeaves(const VECTOR& centerPos);
    void DrawLeaves();

public:
    Skybox3D();
    ~Skybox3D();

    void Init();
    void Update(const VECTOR& centerPos);
    void Draw(const VECTOR& cameraPos);
};
