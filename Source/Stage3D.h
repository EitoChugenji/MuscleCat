#pragma once
#include "DxLib.h"
#include <string>
#include <vector>

#include "ModelConfig.h"

// 壁ポリゴン情報
struct WallPolygon
{
    VECTOR v0, v1, v2;              // 三角形頂点（ワールド座標）
    VECTOR normal;                  // 単位法線ベクトル
    VECTOR minPos, maxPos;          // AABBバウンディングボックス
    bool   isInvisibleWall = false; // 透明補助壁フラグ（デバッグ可視化用）
};

// 3Dステージ管理クラス（Map1: House と Map2: NewMap で当たり判定を完全分離）
class Stage3D
{
private:
    float m_halfWidth;
    float m_halfDepth;
    float m_wallHeight;
    std::string m_modelPath;
    float m_scale = 1.0f;
    ModelConfig::MapType m_mapType = ModelConfig::MapType::SlopeHills;

    int    m_modelHandle = -1;
    bool   m_hasCollision = false;
    VECTOR m_minBounds;
    VECTOR m_maxBounds;

    // マップ1（House: firstmap.mv1）専用の壁コリジョン＆境界データ
    std::vector<WallPolygon> m_houseWallPolygons;
    VECTOR m_houseMinBounds;
    VECTOR m_houseMaxBounds;

    // マップ2（NewMap: newmap.mv1）専用の壁コリジョン＆境界データ
    std::vector<WallPolygon> m_newMapWallPolygons;
    VECTOR m_newMapMinBounds;
    VECTOR m_newMapMaxBounds;

    // マップ別のコリジョン構築処理
    void BuildHouseCollision();
    void BuildNewMapCollision();

    // マップ別の壁押し戻し・接地・境界クランプ処理
    bool ResolvePolygonWallCollision(const std::vector<WallPolygon>& polygons, VECTOR& outPos, float radius, VECTOR* outPushNormal) const;
    bool ResolveHouseWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const;
    bool ResolveNewMapWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const;

    bool GetHouseGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const;
    bool GetNewMapGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const;

    bool ClampToHouseBounds(VECTOR& outPos, float radius) const;
    bool ClampToNewMapBounds(VECTOR& outPos, float radius) const;

public:
    Stage3D();
    ~Stage3D();

    void Init(ModelConfig::MapType mapType = ModelConfig::MapType::SlopeHills);
    void LoadStage(ModelConfig::MapType mapType);
    void Draw3D();

    ModelConfig::MapType GetMapType() const
    {
        return m_mapType;
    }

    // プレイヤーやネズミがステージ壁を越えないよう押し戻す（マップ1・マップ2別専用判定）
    bool ResolveWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal = nullptr) const;

    // キャラクターの現在位置に対応する床・坂道（スロープ）の高さを取得（マップ1・マップ2別専用接地判定）
    bool GetGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal = nullptr) const;

    // 境界ボックス（セーフティネット用：マップ1・マップ2別）
    bool ClampToBounds(VECTOR& outPos, float radius) const;

    float GetHalfWidth() const
    {
        return m_halfWidth;
    }

    float GetHalfDepth() const
    {
        return m_halfDepth;
    }

    int GetModelHandle() const
    {
        return m_modelHandle;
    }

    bool HasCollision() const
    {
        return m_hasCollision;
    }

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
