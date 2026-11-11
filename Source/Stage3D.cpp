// ============================================================================
// @file   Stage3D.cpp
// @brief  Stage3D クラスの実装（マップ別メッシュ解析・壁押し戻し・坂道接地判定）
// ============================================================================
#include "Stage3D.h"
#include "Config.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "Common.h"
#include <cmath>
#include <algorithm>

namespace
{
    // 点 P から三角形 (A, B, C) への最近接点 Q を計算（Ericson's Real-Time Collision Detection）
    VECTOR ClosestPointOnTriangle(const VECTOR& p, const VECTOR& a, const VECTOR& b, const VECTOR& c)
    {
        VECTOR ab = VSub(b, a);
        VECTOR ac = VSub(c, a);
        VECTOR ap = VSub(p, a);

        float d1 = VDot(ab, ap);
        float d2 = VDot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f) return a; // 頂点 A のボロノイ領域

        VECTOR bp = VSub(p, b);
        float d3 = VDot(ab, bp);
        float d4 = VDot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) return b; // 頂点 B のボロノイ領域

        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float v = d1 / (d1 - d3);
            return VAdd(a, VScale(ab, v)); // 辺 AB 上
        }

        VECTOR cp = VSub(p, c);
        float d5 = VDot(ab, cp);
        float d6 = VDot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return c; // 頂点 C のボロノイ領域

        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float w = d2 / (d2 - d6);
            return VAdd(a, VScale(ac, w)); // 辺 AC 上
        }

        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return VAdd(b, VScale(VSub(c, b), w)); // 辺 BC 上
        }

        // 面の内部
        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return VAdd(a, VAdd(VScale(ab, v), VScale(ac, w)));
    }

    float CalcMin3(float a, float b, float c)
    {
        float m = (a < b) ? a : b;
        return (m < c) ? m : c;
    }

    float CalcMax3(float a, float b, float c)
    {
        float m = (a > b) ? a : b;
        return (m > c) ? m : c;
    }
}

Stage3D::Stage3D()
    : m_halfWidth(Config::STAGE_HALF_WIDTH)
    , m_halfDepth(Config::STAGE_HALF_DEPTH)
    , m_wallHeight(Config::WALL_HEIGHT)
    , m_modelPath(ModelConfig::STAGE_MODEL_PATH)
    , m_modelHandle(-1)
    , m_hasCollision(false)
    , m_minBounds(VGet(-Config::STAGE_HALF_WIDTH, -100.0f, -Config::STAGE_HALF_DEPTH))
    , m_maxBounds(VGet(Config::STAGE_HALF_WIDTH, 500.0f, Config::STAGE_HALF_DEPTH))
    , m_houseMinBounds(VGet(-305.0f, -5.0f, -855.0f))
    , m_houseMaxBounds(VGet(905.0f, 200.0f, 655.0f))
    , m_newMapMinBounds(VGet(-2310.0f, -5.0f, -1837.5f))
    , m_newMapMaxBounds(VGet(2610.0f, 200.0f, 1460.0f))
{
}

Stage3D::~Stage3D()
{
    if (m_modelHandle != -1)
    {
        MV1DeleteModel(m_modelHandle);
        m_modelHandle = -1;
        m_hasCollision = false;
        m_houseWallPolygons.clear();
        m_newMapWallPolygons.clear();
        if (m_leafTextureHandle != -1) { DeleteGraph(m_leafTextureHandle); m_leafTextureHandle = -1; }
    }
}

void Stage3D::Init(ModelConfig::MapType mapType)
{
    // 画面クリア背景色（アニメ調の明るいパステルブルー・空色）
    SetBackgroundColor(140, 195, 220);

    // 3Dライティング設定（アニメ・セルルック風：明るい環境光と高コントラスト）
    SetUseLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.50f, 0.52f, 0.55f, 1.0f));
    ChangeLightTypeDir(VGet(0.45f, -0.75f, 0.45f));
    SetLightDifColor(GetColorF(0.95f, 0.93f, 0.88f, 1.0f));
    SetLightSpcColor(GetColorF(0.20f, 0.20f, 0.20f, 1.0f));

    m_skybox.Init();
    LoadStage(mapType);
}

void Stage3D::LoadStage(ModelConfig::MapType mapType)
{
    m_mapType = mapType;

    // 既存モデルハンドルおよび各マップコリジョンデータのクリア
    if (m_modelHandle != -1)
    {
        MV1DeleteModel(m_modelHandle);
        m_modelHandle = -1;
        m_hasCollision = false;
    }
    m_houseWallPolygons.clear();
    m_newMapWallPolygons.clear();
    if (m_leafTextureHandle != -1) { DeleteGraph(m_leafTextureHandle); m_leafTextureHandle = -1; }

    if (m_mapType == ModelConfig::MapType::House)
    {
        m_modelPath = ModelConfig::MAP1_HOUSE_PATH;
        m_scale = ModelConfig::MAP1_HOUSE_SCALE; // 100.0f
        m_halfWidth = 850.0f;
        m_halfDepth = 850.0f;
        m_wallHeight = 250.0f;

        // フォグ設定（室内用）
        SetFogEnable(TRUE);
        SetFogColor(188, 224, 248);
        SetFogStartEnd(4500.0f, 13000.0f);
    }
    else // NewMap (newmap.mv1, scale: 100.0f)
    {
        m_modelPath = ModelConfig::MAP2_SLOPE_PATH;
        m_scale = ModelConfig::MAP2_SLOPE_SCALE; // 100.0f
        m_halfWidth = 2700.0f;
        m_halfDepth = 2200.0f;
        m_wallHeight = 250.0f;

        // フォグ設定（広めの室内空間用）
        SetFogEnable(TRUE);
        SetFogColor(188, 224, 248);
        SetFogStartEnd(5000.0f, 15000.0f);
    }

    int baseHandle = ModelManager::GetInstance().LoadModelHandle(m_modelPath);
    if (baseHandle == -1)
    {
        return;
    }

    m_modelHandle = MV1DuplicateModel(baseHandle);
    if (m_modelHandle == -1)
    {
        return;
    }

    MV1SetPosition(m_modelHandle, VGet(0.0f, 0.0f, 0.0f));
    MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, 0.0f, 0.0f));
    MV1SetScale(m_modelHandle, VGet(m_scale, m_scale, m_scale));

    // コリジョン情報の構築（空間分割: 32 x 16 x 32）
    MV1SetupCollInfo(m_modelHandle, -1, 32, 16, 32);
    m_hasCollision = true;

    // マップ1（House）とマップ2（NewMap）で完全に独立したコリジョン構築処理を実行
    if (m_mapType == ModelConfig::MapType::House)
    {
        BuildHouseCollision();
    }
    else
    {
        BuildNewMapCollision();
    }
    SetupForestTrees();
    // ステージモデルのマテリアルカラー設定（脱・真っ白化：マップに応じた視認性の高い配色）
    if (m_modelHandle != -1)
    {
        int matNum = MV1GetMaterialNum(m_modelHandle);
        COLOR_F difCol, ambCol;
        if (m_mapType == ModelConfig::MapType::House)
        {
            // マップ1（House）: 室内ナチュラルウッド・フローリング調（温かみのある木目カラー）
            difCol = GetColorF(0.70f, 0.54f, 0.38f, 1.0f);
            ambCol = GetColorF(0.38f, 0.28f, 0.18f, 1.0f);
        }
        else
        {
            // マップ2（NewMap）: 屋外パーク＆トレーニングフィールド（目に優しい爽やかな芝生グリーン）
            // 白猫や茶ネズミ、ダンベルがくっきりと浮かび上がり、スロープの立体感が鮮明に見える
            difCol = GetColorF(0.32f, 0.58f, 0.30f, 1.0f);
            ambCol = GetColorF(0.22f, 0.38f, 0.20f, 1.0f);
        }
        for (int i = 0; i < matNum; ++i)
        {
            MV1SetMaterialDifColor(m_modelHandle, i, difCol);
            MV1SetMaterialAmbColor(m_modelHandle, i, ambCol);
            MV1SetMaterialSpcColor(m_modelHandle, i, GetColorF(0.04f, 0.04f, 0.04f, 1.0f));
        }
    }
    BuildStageVisualMesh();
    SetupGroundLeaves();
}

// ============================================================================
// マップ1（House: firstmap.mv1）専用のコリジョン構築
// ============================================================================
void Stage3D::BuildHouseCollision()
{
    m_houseWallPolygons.clear();
    int meshNum = MV1GetMeshNum(m_modelHandle);

    // 1. 床メッシュ（全高35未満）および不要な原点キューブ（Mesh[10]: pCube55）の検出・除外
    std::vector<bool> isExcludedMesh(meshNum, false);
    for (int m = 0; m < meshNum; ++m)
    {
        VECTOR bMin = MV1GetMeshMinPosition(m_modelHandle, m);
        VECTOR bMax = MV1GetMeshMaxPosition(m_modelHandle, m);
        float meshH = (bMax.y - bMin.y) * m_scale;
        if (meshH < 35.0f)
        {
            isExcludedMesh[m] = true;
        }
    }

    if (meshNum > 10)
    {
        VECTOR bMin = MV1GetMeshMinPosition(m_modelHandle, 10);
        VECTOR bMax = MV1GetMeshMaxPosition(m_modelHandle, 10);
        if (std::abs(bMin.x - (-0.5f)) < 0.1f && std::abs(bMax.x - 0.5f) < 0.1f)
        {
            MV1SetMeshVisible(m_modelHandle, 10, FALSE);
            isExcludedMesh[10] = true;
        }
    }

    // 2. 参照メッシュからマップ1の境界と垂直壁ポリゴン（高さ35以上・上端Y>=30）を抽出
    if (MV1SetupReferenceMesh(m_modelHandle, -1, TRUE) >= 0)
    {
        MV1_REF_POLYGONLIST refAll = MV1GetReferenceMesh(m_modelHandle, -1, TRUE);

        VECTOR refMin = VGet(1e9f, 1e9f, 1e9f);
        VECTOR refMax = VGet(-1e9f, -1e9f, -1e9f);

        for (int i = 0; i < refAll.PolygonNum; ++i)
        {
            const auto& poly = refAll.Polygons[i];

            VECTOR v0 = refAll.Vertexs[poly.VIndex[0]].Position;
            VECTOR v1 = refAll.Vertexs[poly.VIndex[1]].Position;
            VECTOR v2 = refAll.Vertexs[poly.VIndex[2]].Position;

            // 不要な pCube55 (Mesh[10]) 以外の頂点でマップ1の正確なバウンディングボックスを算出
            if (!(poly.MeshIndex == 10 && poly.MeshIndex < meshNum && isExcludedMesh[10]))
            {
                const VECTOR verts[3] = { v0, v1, v2 };
                for (const auto& pos : verts)
                {
                    if (pos.x < refMin.x) refMin.x = pos.x;
                    if (pos.y < refMin.y) refMin.y = pos.y;
                    if (pos.z < refMin.z) refMin.z = pos.z;
                    if (pos.x > refMax.x) refMax.x = pos.x;
                    if (pos.y > refMax.y) refMax.y = pos.y;
                    if (pos.z > refMax.z) refMax.z = pos.z;
                }
            }

            // 床メッシュや除外メッシュは壁判定からスキップ
            if (poly.MeshIndex < meshNum && isExcludedMesh[poly.MeshIndex])
            {
                continue;
            }

            float polyMinY = CalcMin3(v0.y, v1.y, v2.y);
            float polyMaxY = CalcMax3(v0.y, v1.y, v2.y);
            float polyHeight = polyMaxY - polyMinY;

            // House マップ専用フィルタ: 床の分け目や段差（高さ35未満、または上端が床上30未満）は除外
            if (polyHeight < 35.0f || polyMaxY < 30.0f)
            {
                continue;
            }

            VECTOR e1 = VSub(v1, v0);
            VECTOR e2 = VSub(v2, v0);
            VECTOR cross = VCross(e1, e2);
            float lenSq = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;
            if (lenSq < 0.000001f)
            {
                continue;
            }

            float invLen = 1.0f / std::sqrt(lenSq);
            VECTOR normal = VGet(cross.x * invLen, cross.y * invLen, cross.z * invLen);

            // ほぼ垂直な面（|normal.y| < 0.35）のみをマップ1の壁として登録
            if (std::abs(normal.y) < 0.35f)
            {
                WallPolygon wp;
                wp.v0 = v0;
                wp.v1 = v1;
                wp.v2 = v2;
                wp.normal = normal;
                wp.minPos = VGet(CalcMin3(v0.x, v1.x, v2.x), polyMinY, CalcMin3(v0.z, v1.z, v2.z));
                wp.maxPos = VGet(CalcMax3(v0.x, v1.x, v2.x), polyMaxY, CalcMax3(v0.z, v1.z, v2.z));
                wp.isInvisibleWall = false;
                m_houseWallPolygons.push_back(wp);
            }
        }

        if (refMin.x <= refMax.x)
        {
            m_houseMinBounds = refMin;
            m_houseMaxBounds = refMax;
        }
        m_minBounds = m_houseMinBounds;
        m_maxBounds = m_houseMaxBounds;

        MV1TerminateReferenceMesh(m_modelHandle, -1, TRUE);
    }

    AppLogAdd("=== MAP 1 (House) COLLISION BUILT ===\n");
    AppLogAdd("House Bounds: Min=(%.1f, %.1f, %.1f), Max=(%.1f, %.1f, %.1f), WallPolygons=%d\n",
        m_houseMinBounds.x, m_houseMinBounds.y, m_houseMinBounds.z,
        m_houseMaxBounds.x, m_houseMaxBounds.y, m_houseMaxBounds.z,
        static_cast<int>(m_houseWallPolygons.size()));
}

// ============================================================================
// マップ2（NewMap: newmap.mv1）専用のコリジョン構築（不要なAddWallQuad透明壁を完全撤去）
// ============================================================================
void Stage3D::BuildNewMapCollision()
{
    m_newMapWallPolygons.clear();

    if (MV1SetupReferenceMesh(m_modelHandle, -1, TRUE) >= 0)
    {
        MV1_REF_POLYGONLIST refAll = MV1GetReferenceMesh(m_modelHandle, -1, TRUE);

        VECTOR refMin = VGet(1e9f, 1e9f, 1e9f);
        VECTOR refMax = VGet(-1e9f, -1e9f, -1e9f);

        for (int i = 0; i < refAll.PolygonNum; ++i)
        {
            const auto& poly = refAll.Polygons[i];

            VECTOR v0 = refAll.Vertexs[poly.VIndex[0]].Position;
            VECTOR v1 = refAll.Vertexs[poly.VIndex[1]].Position;
            VECTOR v2 = refAll.Vertexs[poly.VIndex[2]].Position;

            float polyMinY = CalcMin3(v0.y, v1.y, v2.y);
            float polyMaxY = CalcMax3(v0.y, v1.y, v2.y);
            float polyHeight = polyMaxY - polyMinY;

            // 原点下の不要なデフォルトキューブ（Y = -50 付近）は除外
            if (polyMinY < -20.0f)
            {
                continue;
            }

            const VECTOR verts[3] = { v0, v1, v2 };
            for (const auto& pos : verts)
            {
                if (pos.x < refMin.x) refMin.x = pos.x;
                if (pos.y < refMin.y) refMin.y = pos.y;
                if (pos.z < refMin.z) refMin.z = pos.z;
                if (pos.x > refMax.x) refMax.x = pos.x;
                if (pos.y > refMax.y) refMax.y = pos.y;
                if (pos.z > refMax.z) refMax.z = pos.z;
            }

            // NewMap 専用フィルタ:
            // 通路の側壁・ガードレール（高さ20）および部屋の外壁（高さ200）を正確に抽出し、
            // 高さ8未満の床の厚み側面のみを除外（人為的な AddWallQuad 透明壁は一切追加しない）
            if (polyHeight < 8.0f)
            {
                continue;
            }

            VECTOR e1 = VSub(v1, v0);
            VECTOR e2 = VSub(v2, v0);
            VECTOR cross = VCross(e1, e2);
            float lenSq = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;
            if (lenSq < 0.000001f)
            {
                continue;
            }

            float invLen = 1.0f / std::sqrt(lenSq);
            VECTOR normal = VGet(cross.x * invLen, cross.y * invLen, cross.z * invLen);

            // 法線のY成分の絶対値が 0.35 未満（傾き約70度以上のほぼ垂直な面）を壁とする
            // （坂道・スロープは normal.y >= 0.89 のため壁として誤判定されない）
            if (std::abs(normal.y) < 0.35f)
            {
                WallPolygon wp;
                wp.v0 = v0;
                wp.v1 = v1;
                wp.v2 = v2;
                wp.normal = normal;
                wp.minPos = VGet(CalcMin3(v0.x, v1.x, v2.x), polyMinY, CalcMin3(v0.z, v1.z, v2.z));
                wp.maxPos = VGet(CalcMax3(v0.x, v1.x, v2.x), polyMaxY, CalcMax3(v0.z, v1.z, v2.z));
                wp.isInvisibleWall = false;
                m_newMapWallPolygons.push_back(wp);
            }
        }

        if (refMin.x <= refMax.x)
        {
            m_newMapMinBounds = refMin;
            m_newMapMaxBounds = refMax;
        }
        m_minBounds = m_newMapMinBounds;
        m_maxBounds = m_newMapMaxBounds;

        MV1TerminateReferenceMesh(m_modelHandle, -1, TRUE);
    }

    AppLogAdd("=== MAP 2 (NewMap) COLLISION BUILT ===\n");
    AppLogAdd("NewMap Bounds: Min=(%.1f, %.1f, %.1f), Max=(%.1f, %.1f, %.1f), WallPolygons=%d\n",
        m_newMapMinBounds.x, m_newMapMinBounds.y, m_newMapMinBounds.z,
        m_newMapMaxBounds.x, m_newMapMaxBounds.y, m_newMapMaxBounds.z,
        static_cast<int>(m_newMapWallPolygons.size()));
}

// ============================================================================
// Stage Forest Trees Setup (Fixed non-overlapping tree placement outside stage)
// ============================================================================
void Stage3D::SetupForestTrees()
{
    m_forestTrees.clear();

    struct FixedTreePlacement
    {
        const char* modelPath;
        float       x;
        float       y;
        float       z;
        float       rotY;
        float       scale;
    };

    if (m_mapType == ModelConfig::MapType::NewMap)
    {
        static const FixedTreePlacement kNewMapTrees[] = {
            { "Resource/Sousyoku/ABigTree1.mv1",  -2310.0f,   -8.0f,   2060.0f,  0.44f, 38.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",  -1299.8f,   -8.0f,   2060.0f,  1.61f, 38.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",   -289.5f,   -8.0f,   2060.0f,  2.78f, 38.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",    720.7f,   -8.0f,   2060.0f,  3.94f, 38.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",   1731.0f,   -8.0f,   2060.0f,  5.11f, 38.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",   2740.2f,   -8.0f,   2045.7f,  0.00f, 38.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",   3210.0f,   -8.0f,   1261.0f,  1.17f, 38.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",   3210.0f,   -8.0f,    250.8f,  2.34f, 38.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   3210.0f,   -8.0f,   -759.5f,  3.51f, 38.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",   3210.0f,   -8.0f,  -1769.7f,  4.68f, 38.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",   2610.0f,   -8.0f,  -2437.5f,  5.85f, 38.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   1599.8f,   -8.0f,  -2437.5f,  0.73f, 38.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",    589.5f,   -8.0f,  -2437.5f,  1.90f, 38.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   -420.7f,   -8.0f,  -2437.5f,  3.07f, 38.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",  -1431.0f,   -8.0f,  -2437.5f,  4.24f, 38.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",  -2440.2f,   -8.0f,  -2423.2f,  5.41f, 38.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",  -2910.0f,   -8.0f,  -1638.5f,  0.30f, 38.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",  -2910.0f,   -8.0f,   -628.3f,  1.47f, 38.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",  -2910.0f,   -8.0f,    382.0f,  2.64f, 38.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",  -2910.0f,   -8.0f,   1392.2f,  3.80f, 38.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",  -1784.3f,  -10.0f,   2860.0f,  4.97f, 46.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   -733.0f,  -10.0f,   2860.0f,  6.14f, 46.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",    318.3f,  -10.0f,   2860.0f,  1.03f, 46.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   1369.6f,  -10.0f,   2860.0f,  2.20f, 46.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",   2420.9f,  -10.0f,   2860.0f,  3.37f, 46.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",   3418.7f,  -10.0f,   2602.8f,  4.54f, 46.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",   3981.0f,  -10.0f,   1743.6f,  5.71f, 46.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",   4010.0f,  -10.0f,    694.3f,  0.59f, 46.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   4010.0f,  -10.0f,   -357.0f,  1.76f, 46.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",   4010.0f,  -10.0f,  -1408.3f,  2.93f, 46.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",   3874.0f,  -10.0f,  -2439.4f,  4.10f, 46.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   3123.4f,  -10.0f,  -3140.0f,  5.27f, 46.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",   2084.3f,  -10.0f,  -3237.5f,  0.16f, 46.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   1033.0f,  -10.0f,  -3237.5f,  1.33f, 46.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",    -18.3f,  -10.0f,  -3237.5f,  2.50f, 46.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",  -1069.6f,  -10.0f,  -3237.5f,  3.67f, 46.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",  -2120.9f,  -10.0f,  -3237.5f,  4.83f, 46.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",  -3118.7f,  -10.0f,  -2980.3f,  6.00f, 46.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",  -3681.0f,  -10.0f,  -2121.1f,  0.89f, 46.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",  -3710.0f,  -10.0f,  -1071.8f,  2.06f, 46.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",  -3710.0f,  -10.0f,    -20.5f,  3.23f, 46.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",  -3710.0f,  -10.0f,   1030.8f,  4.40f, 46.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",  -3574.0f,  -10.0f,   2061.9f,  5.57f, 46.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",  -2823.4f,  -10.0f,   2762.5f,  0.45f, 46.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",  -2034.2f,  -12.0f,   3760.0f,  1.62f, 54.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",   -931.1f,  -12.0f,   3760.0f,  2.79f, 54.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",    171.9f,  -12.0f,   3760.0f,  3.96f, 54.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",   1275.0f,  -12.0f,   3760.0f,  5.13f, 54.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   2378.1f,  -12.0f,   3760.0f,  0.02f, 54.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",   3460.5f,  -12.0f,   3597.0f,  1.19f, 54.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",   4350.6f,  -12.0f,   2963.4f,  2.36f, 54.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   4847.9f,  -12.0f,   1990.7f,  3.53f, 54.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",   4910.0f,  -12.0f,    892.4f,  4.69f, 54.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   4910.0f,  -12.0f,   -210.7f,  5.86f, 54.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",   4910.0f,  -12.0f,  -1313.8f,  0.75f, 54.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",   4837.4f,  -12.0f,  -2410.7f,  1.92f, 54.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",   4321.6f,  -12.0f,  -3373.9f,  3.09f, 54.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",   3419.6f,  -12.0f,  -3990.3f,  4.26f, 54.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   2334.2f,  -12.0f,  -4137.5f,  5.43f, 54.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",   1231.1f,  -12.0f,  -4137.5f,  0.31f, 54.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",    128.1f,  -12.0f,  -4137.5f,  1.48f, 54.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   -975.0f,  -12.0f,  -4137.5f,  2.65f, 54.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",  -2078.1f,  -12.0f,  -4137.5f,  3.82f, 54.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",  -3160.5f,  -12.0f,  -3974.5f,  4.99f, 54.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",  -4050.6f,  -12.0f,  -3340.9f,  6.16f, 54.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",  -4547.9f,  -12.0f,  -2368.2f,  1.05f, 54.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",  -4610.0f,  -12.0f,  -1269.9f,  2.22f, 54.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",  -4610.0f,  -12.0f,   -166.8f,  3.39f, 54.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",  -4610.0f,  -12.0f,    936.3f,  4.56f, 54.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",  -4537.4f,  -12.0f,   2033.2f,  5.72f, 54.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",  -4021.6f,  -12.0f,   2996.4f,  0.61f, 54.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",  -3119.6f,  -12.0f,   3612.8f,  1.78f, 54.0f },
        };
        const size_t count = sizeof(kNewMapTrees) / sizeof(kNewMapTrees[0]);
        m_forestTrees.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            const auto& src = kNewMapTrees[i];
            ForestTreeInstance tree;
            tree.modelPath = src.modelPath;
            tree.pos       = VGet(src.x, src.y, src.z);
            tree.rotY      = src.rotY;
            tree.scale     = src.scale;
            m_forestTrees.push_back(tree);
        }
    }
    else
    {
        static const FixedTreePlacement kHouseTrees[] = {
            { "Resource/Sousyoku/ABigTree1.mv1",   -305.0f,   -5.0f,   1155.0f,  0.44f, 34.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",    553.2f,   -5.0f,   1155.0f,  1.61f, 34.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",   1329.1f,   -5.0f,    919.8f,  2.78f, 34.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   1405.0f,   -5.0f,     75.9f,  3.94f, 34.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",   1405.0f,   -5.0f,   -782.2f,  5.11f, 34.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",    905.0f,   -5.0f,  -1355.0f,  0.00f, 34.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",     46.8f,   -5.0f,  -1355.0f,  1.17f, 34.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",   -729.1f,   -5.0f,  -1119.8f,  2.34f, 34.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   -805.0f,   -5.0f,   -275.9f,  3.51f, 34.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",   -805.0f,   -5.0f,    582.2f,  4.68f, 34.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",    169.8f,   -7.0f,   1905.0f,  5.85f, 42.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",   1118.3f,   -7.0f,   1886.7f,  0.73f, 42.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",   1907.9f,   -7.0f,   1401.1f,  1.90f, 42.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   2155.0f,   -7.0f,    505.0f,  3.07f, 42.0f },
            { "Resource/Sousyoku/ABigTree3.mv1",   2155.0f,   -7.0f,   -444.6f,  4.24f, 42.0f },
            { "Resource/Sousyoku/ASmallTree4.mv1",   2040.5f,   -7.0f,  -1377.6f,  5.41f, 42.0f },
            { "Resource/Sousyoku/ASmallTree3.mv1",   1368.5f,   -7.0f,  -2015.9f,  0.30f, 42.0f },
            { "Resource/Sousyoku/ABigTree4.mv1",    430.2f,   -7.0f,  -2105.0f,  1.47f, 42.0f },
            { "Resource/Sousyoku/ASmallTree5.mv1",   -518.3f,   -7.0f,  -2086.7f,  2.64f, 42.0f },
            { "Resource/Sousyoku/ASmallTree6.mv1",  -1307.9f,   -7.0f,  -1601.1f,  3.80f, 42.0f },
            { "Resource/Sousyoku/ABigTree1.mv1",  -1555.0f,   -7.0f,   -705.0f,  4.97f, 42.0f },
            { "Resource/Sousyoku/ASmallTree1.mv1",  -1555.0f,   -7.0f,    244.6f,  6.14f, 42.0f },
            { "Resource/Sousyoku/ABigTree2.mv1",  -1440.5f,   -7.0f,   1177.6f,  1.03f, 42.0f },
            { "Resource/Sousyoku/ASmallTree2.mv1",   -768.5f,   -7.0f,   1815.9f,  2.20f, 42.0f },
        };
        const size_t count = sizeof(kHouseTrees) / sizeof(kHouseTrees[0]);
        m_forestTrees.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            const auto& src = kHouseTrees[i];
            ForestTreeInstance tree;
            tree.modelPath = src.modelPath;
            tree.pos       = VGet(src.x, src.y, src.z);
            tree.rotY      = src.rotY;
            tree.scale     = src.scale;
            m_forestTrees.push_back(tree);
        }
    }
}
// ============================================================================
// Ground Fallen Leaves Setup
// ============================================================================

// ============================================================================
// Build custom shaded stage mesh (1?: ??????????? / 2?: ???????????????)
// ============================================================================
void Stage3D::BuildStageVisualMesh()
{
    m_stageVertices.clear();
    if (m_modelHandle == -1)
    {
        return;
    }

    int meshNum = MV1GetMeshNum(m_modelHandle);
    if (MV1SetupReferenceMesh(m_modelHandle, -1, TRUE) < 0)
    {
        return;
    }

    MV1_REF_POLYGONLIST refAll = MV1GetReferenceMesh(m_modelHandle, -1, TRUE);
    m_stageVertices.reserve(static_cast<size_t>(refAll.PolygonNum) * 3 + 6);

    // 1. ???????3D????????????????????
    {
        float groundY = (m_mapType == ModelConfig::MapType::House) ? -5.5f : -8.5f;
        float ext = 8500.0f;
        COLOR_U8 grassCol = GetColorU8(96, 158, 78, 255);
        COLOR_U8 zeroSpc  = GetColorU8(0, 0, 0, 0);
        VECTOR upNorm = VGet(0.0f, 1.0f, 0.0f);

        VECTOR gp[4] = {
            VGet(-ext, groundY,  ext),
            VGet( ext, groundY,  ext),
            VGet( ext, groundY, -ext),
            VGet(-ext, groundY, -ext)
        };
        const int triIdx[6] = { 0, 1, 2, 0, 2, 3 };
        for (int k = 0; k < 6; ++k)
        {
            VERTEX3D v{};
            v.pos  = gp[triIdx[k]];
            v.norm = upNorm;
            v.dif  = grassCol;
            v.spc  = zeroSpc;
            m_stageVertices.push_back(v);
        }
    }

    // ?????????????????????????
    VECTOR sunDir = VNorm(VGet(-0.48f, 0.76f, -0.44f));
    COLOR_U8 zeroSpc = GetColorU8(0, 0, 0, 0);

    for (int i = 0; i < refAll.PolygonNum; ++i)
    {
        const auto& poly = refAll.Polygons[i];
        VECTOR v0 = refAll.Vertexs[poly.VIndex[0]].Position;
        VECTOR v1 = refAll.Vertexs[poly.VIndex[1]].Position;
        VECTOR v2 = refAll.Vertexs[poly.VIndex[2]].Position;

        if (m_mapType == ModelConfig::MapType::House)
        {
            // ????????? Mesh[10] (pCube55) ???????
            if (poly.MeshIndex == 10)
            {
                continue;
            }

            // 1??House?: ??????Mesh[2]?????????2????????????(Y?27)???? Y=17.5f ?????
            if (poly.MeshIndex != 2)
            {
                auto lowerWallY = [](VECTOR& v)
                {
                    if (v.y > 5.0f)
                    {
                        v.y = 5.0f + (v.y - 5.0f) * (12.5f / 195.0f);
                    }
                };
                lowerWallY(v0);
                lowerWallY(v1);
                lowerWallY(v2);
            }
        }
        else
        {
            float polyMinY = CalcMin3(v0.y, v1.y, v2.y);
            if (polyMinY < -20.0f)
            {
                continue;
            }
        }

        VECTOR e1 = VSub(v1, v0);
        VECTOR e2 = VSub(v2, v0);
        VECTOR cross = VCross(e1, e2);
        float lenSq = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;
        VECTOR normal = (lenSq > 0.000001f) ? VScale(cross, 1.0f / std::sqrt(lenSq)) : VGet(0.0f, 1.0f, 0.0f);

        float polyMinY = CalcMin3(v0.y, v1.y, v2.y);
        float polyMaxY = CalcMax3(v0.y, v1.y, v2.y);
        float cx = (v0.x + v1.x + v2.x) * (1.0f / 3.0f);
        float cz = (v0.z + v1.z + v2.z) * (1.0f / 3.0f);

        float baseR = 200.0f, baseG = 200.0f, baseB = 200.0f;

        if (m_mapType == ModelConfig::MapType::House)
        {
            if (poly.MeshIndex == 2)
            {
                // 1???: ??????????????????????????????????????
                int plankIdx = static_cast<int>(std::floor((cx + 2000.0f) / 90.0f));
                float stripe = (plankIdx % 2 == 0) ? 1.0f : 0.95f;
                baseR = 222.0f * stripe;
                baseG = 182.0f * stripe;
                baseB = 136.0f * stripe;
            }
            else
            {
                // 1????????17.5f?: ???????????????????????????????
                if (normal.y >= 0.75f)
                {
                    baseR = 240.0f;
                    baseG = 230.0f;
                    baseB = 212.0f;
                }
                else
                {
                    baseR = 164.0f;
                    baseG = 118.0f;
                    baseB = 82.0f;
                }
            }
        }
        else
        {
            // 2??NewMap?: ??????????????????????????????????
            int tileX = static_cast<int>(std::floor((cx + 5000.0f) / 180.0f));
            int tileZ = static_cast<int>(std::floor((cz + 5000.0f) / 180.0f));
            float checker = ((tileX + tileZ) % 2 == 0) ? 1.0f : 0.96f;

            if (normal.y >= 0.96f)
            {
                if (polyMaxY >= 95.0f)
                {
                    // ????????????: ???????????
                    baseR = 238.0f * checker;
                    baseG = 226.0f * checker;
                    baseB = 206.0f * checker;
                }
                else if (polyMaxY > 12.0f && polyMaxY < 35.0f)
                {
                    // ????????????????????: ?????????
                    baseR = 220.0f;
                    baseG = 198.0f;
                    baseB = 172.0f;
                }
                else
                {
                    // ???????????: ??????????????????????????????
                    baseR = 196.0f * checker;
                    baseG = 162.0f * checker;
                    baseB = 124.0f * checker;
                }
            }
            else if (normal.y >= 0.35f)
            {
                // ???????: ??????????????????????????
                int stepStripe = static_cast<int>(std::floor((cx + cz + 5000.0f) / 70.0f));
                float sFactor = (stepStripe % 2 == 0) ? 1.0f : 0.94f;
                baseR = 216.0f * sFactor;
                baseG = 152.0f * sFactor;
                baseB = 108.0f * sFactor;
            }
            else
            {
                if (polyMaxY < 60.0f)
                {
                    // ???????????????????: ??????????????????
                    baseR = 152.0f;
                    baseG = 122.0f;
                    baseB = 94.0f;
                }
                else
                {
                    // ??????????: ????????????????????????????????
                    baseR = 166.0f;
                    baseG = 176.0f;
                    baseB = 188.0f;
                }
            }
        }

        // ????????????????????????????????????????????
        float ndotl = VDot(normal, sunDir);
        float shade = 0.80f + 0.22f * ndotl;
        if (std::abs(normal.y) < 0.35f)
        {
            // ??????????????????????????????????????
            shade = 0.72f + 0.26f * ((ndotl + 1.0f) * 0.5f);
        }

        int r = static_cast<int>(MathHelper::Clamp(baseR * shade, 0.0f, 255.0f));
        int g = static_cast<int>(MathHelper::Clamp(baseG * shade, 0.0f, 255.0f));
        int b = static_cast<int>(MathHelper::Clamp(baseB * shade, 0.0f, 255.0f));
        COLOR_U8 col = GetColorU8(r, g, b, 255);

        const VECTOR pts[3] = { v0, v1, v2 };
        for (int k = 0; k < 3; ++k)
        {
            VERTEX3D vt{};
            vt.pos  = pts[k];
            vt.norm = normal;
            vt.dif  = col;
            vt.spc  = zeroSpc;
            m_stageVertices.push_back(vt);
        }
    }

    MV1TerminateReferenceMesh(m_modelHandle, -1, TRUE);
}

void Stage3D::SetupGroundLeaves()
{
    m_groundLeaves.clear();

    unsigned int leafColors[] = {
        GetColor(76, 166, 52),
        GetColor(92, 184, 62),
        GetColor(120, 198, 48),
        GetColor(48, 122, 38),
        GetColor(180, 142, 42),
        GetColor(196, 158, 48),
        GetColor(158, 88, 36)
    };
    const int colorCount = sizeof(leafColors) / sizeof(leafColors[0]);

    float spawnRangeX = (m_mapType == ModelConfig::MapType::NewMap) ? 2500.0f : 800.0f;
    float spawnRangeZ = (m_mapType == ModelConfig::MapType::NewMap) ? 2100.0f : 800.0f;
    size_t targetCount = (m_mapType == ModelConfig::MapType::NewMap) ? 180 : 80;

    unsigned int leafSeed = 98765u;
    auto nextLeafRand = [&leafSeed]() -> int {
        leafSeed = leafSeed * 1664525u + 1013904223u;
        return static_cast<int>((leafSeed >> 1) & 0x7FFF);
    };

    for (int attempt = 0; attempt < 3500 && m_groundLeaves.size() < targetCount; ++attempt)
    {
        float rx = ((nextLeafRand() % 2000) / 1000.0f - 1.0f) * spawnRangeX;
        float rz = ((nextLeafRand() % 2000) / 1000.0f - 1.0f) * spawnRangeZ;

        float groundY = 0.0f;
        VECTOR groundNorm = VGet(0.0f, 1.0f, 0.0f);

        if (GetGroundHeight(VGet(rx, 100.0f, rz), groundY, &groundNorm))
        {
            if (groundNorm.y > 0.45f)
            {
                GroundFallenLeaf leaf;
                leaf.pos    = VGet(rx, groundY + 0.35f, rz);
                leaf.normal = groundNorm;
                leaf.rotY   = static_cast<float>(nextLeafRand() % 360) * (3.14159265f / 180.0f);
                leaf.size   = 3.8f + static_cast<float>(nextLeafRand() % 35) * 0.1f;
                leaf.color  = leafColors[nextLeafRand() % colorCount];
                m_groundLeaves.push_back(leaf);
            }
        }
    }
}

void Stage3D::Update(const VECTOR& centerPos)
{
    m_skybox.Update(centerPos);
}

// ============================================================================
// Stage 3D Drawing
// ============================================================================
void Stage3D::Draw3D()
{
    // 1. Forest Skybox / Skydome drawing (most distant background)
    m_skybox.Draw(GetCameraPosition());

    // 2. Stage model (???????????????????????????)
    if (!m_stageVertices.empty())
    {
        SetUseLighting(FALSE);
        DrawPolygon3D(m_stageVertices.data(), static_cast<int>(m_stageVertices.size() / 3), DX_NONE_GRAPH, FALSE);
        SetUseLighting(TRUE);
    }
    else if (m_modelHandle != -1)
    {
        MV1DrawModel(m_modelHandle);
    }
    else if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, VGet(0.0f, 0.0f, 0.0f), 0.0f, m_scale))
    {
        ModelManager::GetInstance().DrawFallbackStage(m_halfWidth, m_halfDepth, m_wallHeight);
    }

    // 2.5. 地面に散らばる落ち葉・草の描画（地面に接地）
    SetUseLighting(TRUE);
    SetUseBackCulling(FALSE);

    for (const auto& leaf : m_groundLeaves)
    {
        float s = leaf.size;
        VECTOR localPts[4] = {
            VGet(0.0f,       0.0f,  s * 1.4f),
            VGet(-s * 0.65f, 0.0f,  0.0f),
            VGet(0.0f,       0.0f, -s * 0.9f),
            VGet( s * 0.65f, 0.0f,  0.0f)
        };

        MATRIX mRotY = MGetRotY(leaf.rotY);

        VECTOR up = VGet(0.0f, 1.0f, 0.0f);
        VECTOR axis = VCross(up, leaf.normal);
        float axisLen = VSize(axis);
        MATRIX mAlign = MGetIdent();
        if (axisLen > 0.001f)
        {
            axis = VScale(axis, 1.0f / axisLen);
            float dot = VDot(up, leaf.normal);
            if (dot > 1.0f) dot = 1.0f;
            if (dot < -1.0f) dot = -1.0f;
            float angle = std::acos(dot);
            mAlign = MGetRotAxis(axis, angle);
        }

        MATRIX mCombined = MMult(mRotY, mAlign);

        VECTOR wPts[4];
        for (int k = 0; k < 4; ++k)
        {
            VECTOR r = VTransform(localPts[k], mCombined);
            wPts[k] = VAdd(leaf.pos, r);
        }

        DrawTriangle3D(wPts[0], wPts[1], wPts[2], leaf.color, TRUE);
        DrawTriangle3D(wPts[0], wPts[2], wPts[3], leaf.color, TRUE);
    }
    SetUseBackCulling(TRUE);

    // 3. 3D Trees surrounding the stage
    for (const auto& tree : m_forestTrees)
    {
        ModelManager::GetInstance().DrawModelIfLoaded(
            tree.modelPath.c_str(),
            tree.pos,
            tree.rotY,
            tree.scale
        );
    }
}

// ============================================================================
// ポリゴンリストに対する共通押し戻しソルバ（マップ1・マップ2それぞれの専用ポリゴンで実行）
// ============================================================================
bool Stage3D::ResolvePolygonWallCollision(const std::vector<WallPolygon>& polygons, VECTOR& outPos, float radius, VECTOR* outPushNormal) const
{
    bool anyCollided = false;
    if (outPushNormal)
    {
        *outPushNormal = VGet(0.0f, 0.0f, 0.0f);
    }

    if (polygons.empty())
    {
        return false;
    }

    // 押し戻しを最大4回反復（コーナーでの挟まりや複数壁との同時接触を滑らかに解決）
    for (int iter = 0; iter < 4; ++iter)
    {
        float maxPenetration = 0.0f;
        float bestPushX = 0.0f;
        float bestPushZ = 0.0f;

        float charHeight = (radius * 2.5f > 60.0f) ? (radius * 2.5f) : 60.0f;
        float catBottomY = outPos.y;
        float catTopY    = outPos.y + charHeight;

        for (const auto& poly : polygons)
        {
            // AABB カリング
            if (outPos.x + radius < poly.minPos.x || outPos.x - radius > poly.maxPos.x ||
                outPos.z + radius < poly.minPos.z || outPos.z - radius > poly.maxPos.z ||
                catTopY < poly.minPos.y || (catBottomY + 1.0f) > poly.maxPos.y)
            {
                continue;
            }

            if (poly.maxPos.y < catBottomY + 1.0f || poly.minPos.y > catTopY)
            {
                continue;
            }

            float testY = catBottomY + radius * 0.5f;
            if (testY < poly.minPos.y) testY = poly.minPos.y + 0.1f;
            if (testY > poly.maxPos.y) testY = poly.maxPos.y - 0.1f;

            VECTOR p = VGet(outPos.x, testY, outPos.z);
            VECTOR q = ClosestPointOnTriangle(p, poly.v0, poly.v1, poly.v2);

            float dx = outPos.x - q.x;
            float dz = outPos.z - q.z;
            float distSq = dx * dx + dz * dz;

            if (distSq < radius * radius)
            {
                float dist = std::sqrt(distSq);
                float penetration = radius - dist;

                if (penetration > maxPenetration)
                {
                    float awayX = 0.0f;
                    float awayZ = 0.0f;
                    if (dist > 0.001f)
                    {
                        awayX = dx / dist;
                        awayZ = dz / dist;
                    }
                    else
                    {
                        awayX = poly.normal.x;
                        awayZ = poly.normal.z;
                    }

                    float nx = poly.normal.x;
                    float nz = poly.normal.z;
                    float nLen = std::sqrt(nx * nx + nz * nz);
                    if (nLen > 0.0001f)
                    {
                        nx /= nLen;
                        nz /= nLen;
                    }

                    float dot = awayX * nx + awayZ * nz;
                    float alignNx = (dot >= 0.0f) ? nx : -nx;
                    float alignNz = (dot >= 0.0f) ? nz : -nz;

                    float pushX = alignNx * 0.65f + awayX * 0.35f;
                    float pushZ = alignNz * 0.65f + awayZ * 0.35f;
                    float pLen = std::sqrt(pushX * pushX + pushZ * pushZ);
                    if (pLen > 0.0001f)
                    {
                        pushX /= pLen;
                        pushZ /= pLen;
                    }

                    maxPenetration = penetration;
                    bestPushX = pushX;
                    bestPushZ = pushZ;
                }
            }
        }

        if (maxPenetration > 0.001f)
        {
            outPos.x += bestPushX * maxPenetration;
            outPos.z += bestPushZ * maxPenetration;
            anyCollided = true;
            if (outPushNormal)
            {
                outPushNormal->x = bestPushX;
                outPushNormal->z = bestPushZ;
            }
        }
        else
        {
            break;
        }
    }

    return anyCollided;
}

// ============================================================================
// マップ1（House）専用の壁衝突解決
// ============================================================================
bool Stage3D::ResolveHouseWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const
{
    bool anyCollided = ResolvePolygonWallCollision(m_houseWallPolygons, outPos, radius, outPushNormal);

    VECTOR prevPos = outPos;
    if (ClampToHouseBounds(outPos, radius))
    {
        anyCollided = true;
        if (outPushNormal && outPushNormal->x == 0.0f && outPushNormal->z == 0.0f)
        {
            float cdx = outPos.x - prevPos.x;
            float cdz = outPos.z - prevPos.z;
            float cLen = std::sqrt(cdx * cdx + cdz * cdz);
            if (cLen > 0.0001f)
            {
                outPushNormal->x = cdx / cLen;
                outPushNormal->z = cdz / cLen;
            }
        }
    }

    return anyCollided;
}

// ============================================================================
// マップ2（NewMap）専用の壁衝突解決
// ============================================================================
bool Stage3D::ResolveNewMapWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const
{
    bool anyCollided = ResolvePolygonWallCollision(m_newMapWallPolygons, outPos, radius, outPushNormal);

    VECTOR prevPos = outPos;
    if (ClampToNewMapBounds(outPos, radius))
    {
        anyCollided = true;
        if (outPushNormal && outPushNormal->x == 0.0f && outPushNormal->z == 0.0f)
        {
            float cdx = outPos.x - prevPos.x;
            float cdz = outPos.z - prevPos.z;
            float cLen = std::sqrt(cdx * cdx + cdz * cdz);
            if (cLen > 0.0001f)
            {
                outPushNormal->x = cdx / cLen;
                outPushNormal->z = cdz / cLen;
            }
        }
    }

    return anyCollided;
}

bool Stage3D::ResolveWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const
{
    if (m_mapType == ModelConfig::MapType::House)
    {
        return ResolveHouseWallCollision(outPos, radius, outPushNormal);
    }
    else
    {
        return ResolveNewMapWallCollision(outPos, radius, outPushNormal);
    }
}

// ============================================================================
// マップ1（House）専用の外周境界クランプ
// ============================================================================
bool Stage3D::ClampToHouseBounds(VECTOR& outPos, float radius) const
{
    bool collided = false;
    float minX = m_houseMinBounds.x + radius;
    float maxX = m_houseMaxBounds.x - radius;
    float minZ = m_houseMinBounds.z + radius;
    float maxZ = m_houseMaxBounds.z - radius;

    if (outPos.x < minX) { outPos.x = minX; collided = true; }
    if (outPos.x > maxX) { outPos.x = maxX; collided = true; }
    if (outPos.z < minZ) { outPos.z = minZ; collided = true; }
    if (outPos.z > maxZ) { outPos.z = maxZ; collided = true; }

    return collided;
}

// ============================================================================
// マップ2（NewMap）専用の外周境界クランプ
// ============================================================================
bool Stage3D::ClampToNewMapBounds(VECTOR& outPos, float radius) const
{
    bool collided = false;
    float minX = m_newMapMinBounds.x + radius;
    float maxX = m_newMapMaxBounds.x - radius;
    float minZ = m_newMapMinBounds.z + radius;
    float maxZ = m_newMapMaxBounds.z - radius;

    if (outPos.x < minX) { outPos.x = minX; collided = true; }
    if (outPos.x > maxX) { outPos.x = maxX; collided = true; }
    if (outPos.z < minZ) { outPos.z = minZ; collided = true; }
    if (outPos.z > maxZ) { outPos.z = maxZ; collided = true; }

    return collided;
}

bool Stage3D::ClampToBounds(VECTOR& outPos, float radius) const
{
    if (m_mapType == ModelConfig::MapType::House)
    {
        return ClampToHouseBounds(outPos, radius);
    }
    else
    {
        return ClampToNewMapBounds(outPos, radius);
    }
}

// ============================================================================
// マップ1（House）専用の接地高さ判定
// （不要なpCube55(Y=50)や壁の上面(Y=200)を誤検出せず、室内床面(Y~5)のみを正確に検出）
// ============================================================================
bool Stage3D::GetHouseGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const
{
    if (m_modelHandle == -1 || !m_hasCollision)
    {
        outGroundY = 0.0f;
        return false;
    }

    // Houseの床（Mesh[2], Y~5.0f）のみを探索するため、Y=30.0f 〜 -30.0f の範囲でレイキャスト
    VECTOR start = VGet(pos.x, 30.0f, pos.z);
    VECTOR end   = VGet(pos.x, -30.0f, pos.z);

    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(m_modelHandle, -1, start, end);
    if (hit.HitFlag == 1 && hit.Normal.y >= 0.35f)
    {
        outGroundY = hit.HitPosition.y;
        if (outGroundNormal)
        {
            *outGroundNormal = hit.Normal;
        }
        return true;
    }

    outGroundY = 0.0f;
    return false;
}

// ============================================================================
// マップ2（NewMap）専用の接地高さ判定
// （坂道・通路床(Y~0〜120)を正確に追跡し、外壁や中央円柱の屋上(Y=200)への誤ワープを防止）
// ============================================================================
bool Stage3D::GetNewMapGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const
{
    if (m_modelHandle == -1 || !m_hasCollision)
    {
        outGroundY = 0.0f;
        return false;
    }

    // 1. 現在の足元付近のレイキャスト（通常移動時は pos.y+120 から探索するため壁屋上Y=200へ誤ワープしない）
    float topSearchY = pos.y + 120.0f;
    float bottomSearchY = pos.y - 180.0f;
    VECTOR start = VGet(pos.x, topSearchY, pos.z);
    VECTOR end   = VGet(pos.x, bottomSearchY, pos.z);

    MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(m_modelHandle, -1, start, end);
    if (hit.HitFlag == 1 && hit.Normal.y >= 0.35f && hit.HitPosition.y > -20.0f)
    {
        outGroundY = hit.HitPosition.y;
        if (outGroundNormal)
        {
            *outGroundNormal = hit.Normal;
        }
        return true;
    }

    // 2. セーフティネット：上空から床下まで全体探索（タイトル画面の原点台座 Y=200 等にも対応）
    float skyY = (m_newMapMaxBounds.y > 500.0f) ? (m_newMapMaxBounds.y + 500.0f) : 500.0f;
    VECTOR wideStart = VGet(pos.x, skyY, pos.z);
    VECTOR wideEnd   = VGet(pos.x, -20.0f, pos.z);
    MV1_COLL_RESULT_POLY wideHit = MV1CollCheck_Line(m_modelHandle, -1, wideStart, wideEnd);
    if (wideHit.HitFlag == 1 && wideHit.Normal.y >= 0.35f && wideHit.HitPosition.y > -20.0f)
    {
        outGroundY = wideHit.HitPosition.y;
        if (outGroundNormal)
        {
            *outGroundNormal = wideHit.Normal;
        }
        return true;
    }

    outGroundY = 0.0f;
    return false;
}

bool Stage3D::GetGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const
{
    if (m_mapType == ModelConfig::MapType::House)
    {
        return GetHouseGroundHeight(pos, outGroundY, outGroundNormal);
    }
    else
    {
        return GetNewMapGroundHeight(pos, outGroundY, outGroundNormal);
    }
}


// ============================================================================
// 指定座標が壁や境界・段差から十分離れた安全なスポーン可能位置か検証する
// ============================================================================
bool Stage3D::IsSafeSpawnLocation(const VECTOR& pos, float minWallDist, float& outGroundY) const
{
    // 1. 床面接地チェック
    if (!GetGroundHeight(pos, outGroundY))
    {
        return false;
    }

    VECTOR testPos = VGet(pos.x, outGroundY, pos.z);

    // 2. 外周境界チェック（外周から minWallDist 以上内側か）
    const VECTOR& minB = GetMinBounds();
    const VECTOR& maxB = GetMaxBounds();
    if (testPos.x < minB.x + minWallDist || testPos.x > maxB.x - minWallDist ||
        testPos.z < minB.z + minWallDist || testPos.z > maxB.z - minWallDist)
    {
        return false;
    }

    // 3. 壁ポリゴンとの距離チェック（minWallDist 半径の球で衝突押し出しがあるか）
    VECTOR pushNormal = VGet(0.0f, 0.0f, 0.0f);
    VECTOR clampedPos = testPos;
    if (ResolveWallCollision(clampedPos, minWallDist, &pushNormal))
    {
        return false; // 壁から minWallDist 以内に近すぎる
    }

    // 4. 周囲4方向（前後左右）も床が存在するか（崖っぷち・急な段差際でないか）
    const float probeDist = minWallDist * 0.6f;
    const VECTOR offsets[4] = {
        VGet( probeDist, 0.0f,  0.0f),
        VGet(-probeDist, 0.0f,  0.0f),
        VGet( 0.0f,      0.0f,  probeDist),
        VGet( 0.0f,      0.0f, -probeDist)
    };
    for (int k = 0; k < 4; ++k)
    {
        float probeGy = 0.0f;
        VECTOR probeP = VAdd(testPos, offsets[k]);
        if (!GetGroundHeight(probeP, probeGy))
        {
            return false; // すぐ近くが床外（崖）
        }
        if (std::abs(probeGy - outGroundY) > 35.0f)
        {
            return false; // 急な段差際
        }
    }

    return true;
}