#pragma once
#include "DxLib.h"
#include <string>
#include <vector>

// 壁ポリゴン情報
struct WallPolygon
{
    VECTOR v0, v1, v2;      // 三角形頂点（ワールド座標）
    VECTOR normal;          // 単位法線ベクトル
    VECTOR minPos, maxPos;  // AABBバウンディングボックス
};

// 3Dステージ管理クラス
class Stage3D
{
private:
    float m_halfWidth;
    float m_halfDepth;
    float m_wallHeight;
    std::string m_modelPath;

    int    m_modelHandle = -1;
    bool   m_hasCollision = false;
    VECTOR m_minBounds;
    VECTOR m_maxBounds;
    std::vector<WallPolygon> m_wallPolygons;

public:
    Stage3D();
    ~Stage3D();

    void Init();
    void Draw3D();

    // プレイヤーやネズミがステージ壁を越えないよう押し戻す（3Dメッシュコリジョン対応）
    bool ResolveWallCollision(VECTOR& outPos, float radius) const;

    // 従来の境界ボックス（セーフティネット用）
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
        return m_minBounds;
    }

    const VECTOR& GetMaxBounds() const
    {
        return m_maxBounds;
    }

    const std::vector<WallPolygon>& GetWallPolygons() const
    {
        return m_wallPolygons;
    }
};