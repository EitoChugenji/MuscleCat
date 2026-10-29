// ============================================================================
// @file   Stage3D.h
// @brief  3Dステージモデルの読み込み・描画・壁押し戻し・接地高さ判定クラス定義
// ============================================================================
#pragma once
#include "DxLib.h"
#include "ModelConfig.h"
#include "Skybox3D.h"
#include <string>
#include <vector>

// ============================================================================
// 壁ポリゴン情報構造体
// ステージモデルから抽出した垂直壁ポリゴン（三角形）の頂点・法線・AABBを保持する
// ============================================================================
struct WallPolygon
{
    VECTOR v0, v1, v2;              // 三角形の3頂点（ワールド座標）
    VECTOR normal;                  // 単位法線ベクトル
    VECTOR minPos, maxPos;          // 高速カリング用のAABBバウンディングボックス
    bool   isInvisibleWall = false; // 透明補助壁フラグ（F1デバッグ可視化用）
};

// ============================================================================
// 森の3D木々・装飾オブジェクト配置構造体
// ============================================================================
struct ForestTreeInstance
{
    std::string modelPath;  // 3Dモデルパス
    VECTOR      pos;        // 配置座標
    float       rotY;       // Y軸回転
    float       scale;      // スケール
};

// ============================================================================
// 蠎翫↓謨｣繧峨・繧玖誠縺｡闡峨・闕芽痩讒矩菴難ｼ域｣ｮ縺ｮ蝨ｰ髱｢貍泌・逕ｨ・・// ============================================================================
struct GroundFallenLeaf
{
    VECTOR       pos;    // 謗･蝨ｰ蠎ｧ讓・(蠎企擇 Y + 0.35f)
    VECTOR       normal; // 蝨ｰ髱｢縺ｮ豕慕ｷ壹・繧ｯ繝医Ν
    float        rotY;   // 蝗櫁ｻ｢隗貞ｺｦ
    float        size;   // 闡峨・螟ｧ縺阪＆
    unsigned int color;  // 闡峨・濶ｲ
};

// ============================================================================
// Stage3D クラス
// マップ1（House: firstmap.mv1）とマップ2（NewMap: newmap.mv1）それぞれのメッシュ特性に
// 合わせた専用コリジョン構築・壁押し戻し・スロープ接地判定・境界クランプを提供する
// ============================================================================
class Stage3D
{
private:
    float                m_halfWidth;                                    // ステージX方向の基準半径
    float                m_halfDepth;                                    // ステージZ方向の基準半径
    float                m_wallHeight;                                   // 壁の基準高さ
    std::string          m_modelPath;                                    // 現在読み込んでいるマップのモデルパス
    float                m_scale        = 1.0f;                          // マップモデルの表示スケール
    ModelConfig::MapType m_mapType      = ModelConfig::MapType::SlopeHills; // 現在のマップ種別

    int                  m_modelHandle  = -1;                            // DxLib複製モデルハンドル
    bool                 m_hasCollision = false;                         // MV1コリジョン構築済みフラグ
    VECTOR               m_minBounds;                                    // 現在のマップの最小境界座標
    VECTOR               m_maxBounds;                                    // 現在のマップの最大境界座標

    // マップ1（House: firstmap.mv1）専用の壁ポリゴン配列＆境界データ
    std::vector<WallPolygon> m_houseWallPolygons;
    VECTOR                   m_houseMinBounds;
    VECTOR                   m_houseMaxBounds;

    // マップ2（NewMap: newmap.mv1）専用の壁ポリゴン配列＆境界データ
    std::vector<WallPolygon> m_newMapWallPolygons;
    VECTOR                   m_newMapMinBounds;
    VECTOR                   m_newMapMaxBounds;
    Skybox3D                 m_skybox;        // 森のスカイボックス・スカイドーム描画
    std::vector<ForestTreeInstance> m_forestTrees;   // ステージ外周の3D森（木々・切り株・自然物）
    std::vector<GroundFallenLeaf>   m_groundLeaves;  // 床面に散らばる落ち葉・草葉群
    int                             m_leafTextureHandle = -1; // 森の床・葉っぱテクスチャ

    // マップ別の壁コリジョン抽出処理
    void BuildHouseCollision();
    void BuildNewMapCollision();
    void SetupForestTrees();
    void SetupGroundLeaves();

    // ポリゴン群に対する球/円柱の反復押し戻しソルバおよびマップ別衝突解決
    bool ResolvePolygonWallCollision(const std::vector<WallPolygon>& polygons, VECTOR& outPos, float radius, VECTOR* outPushNormal) const;
    bool ResolveHouseWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const;
    bool ResolveNewMapWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const;

    // マップ別の床・坂道（スロープ）高さレイキャスト取得
    bool GetHouseGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const;
    bool GetNewMapGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const;

    // マップ別の外周バウンディングボックス制限（セーフティネット）
    bool ClampToHouseBounds(VECTOR& outPos, float radius) const;
    bool ClampToNewMapBounds(VECTOR& outPos, float radius) const;

public:
    Stage3D();
    ~Stage3D();

    // ライティング初期化および指定マップのロード
    void Init(ModelConfig::MapType mapType = ModelConfig::MapType::SlopeHills);

    // マップモデルの切り替え・コリジョン再構築
    void LoadStage(ModelConfig::MapType mapType);
    void Update(const VECTOR& centerPos);

    // 3Dステージモデルの描画
    void Draw3D();

    ModelConfig::MapType GetMapType() const { return m_mapType; }

    // キャラクター（猫・ネズミ）が壁を貫通しないよう押し戻す（押し戻し法線も取得可能）
    bool ResolveWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal = nullptr) const;

    // キャラクターの現在XZ座標における床・坂道のY高さを取得する
    bool GetGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal = nullptr) const;

    // ステージ外周境界ボックス内に座標をクランプする
    bool ClampToBounds(VECTOR& outPos, float radius) const;

    // 各種ゲッター
    float GetHalfWidth() const  { return m_halfWidth; }
    float GetHalfDepth() const  { return m_halfDepth; }
    int   GetModelHandle() const { return m_modelHandle; }
    bool  HasCollision() const  { return m_hasCollision; }

    const VECTOR& GetMinBounds() const
    {
        return (m_mapType == ModelConfig::MapType::House) ? m_houseMinBounds : m_newMapMinBounds;
    }

    const VECTOR& GetMaxBounds() const
    {
        return (m_mapType == ModelConfig::MapType::House) ? m_houseMaxBounds : m_newMapMaxBounds;
    }

    const std::vector<WallPolygon>& GetWallPolygons() const
    {
        return (m_mapType == ModelConfig::MapType::House) ? m_houseWallPolygons : m_newMapWallPolygons;
    }
};

