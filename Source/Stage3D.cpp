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
    }
}

void Stage3D::Init(ModelConfig::MapType mapType)
{
    // 画面クリア背景色（アニメ調の明るいパステルブルー・空色）
    SetBackgroundColor(175, 218, 252);

    // 3Dライティング設定（アニメ・セルルック風：明るい環境光と高コントラスト）
    SetUseLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.86f, 0.86f, 0.88f, 1.0f));
    ChangeLightTypeDir(VGet(0.35f, -0.85f, 0.4f));
    SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    SetLightSpcColor(GetColorF(0.7f, 0.7f, 0.7f, 1.0f));

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

    if (m_mapType == ModelConfig::MapType::House)
    {
        m_modelPath = ModelConfig::MAP1_HOUSE_PATH;
        m_scale = ModelConfig::MAP1_HOUSE_SCALE; // 100.0f
        m_halfWidth = 850.0f;
        m_halfDepth = 850.0f;
        m_wallHeight = 250.0f;

        // フォグ設定（室内用）
        SetFogEnable(TRUE);
        SetFogColor(175, 218, 252);
        SetFogStartEnd(2500.0f, 5500.0f);
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
        SetFogColor(175, 218, 252);
        SetFogStartEnd(3500.0f, 7500.0f);
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

void Stage3D::Draw3D()
{
    if (m_modelHandle != -1)
    {
        MV1DrawModel(m_modelHandle);
    }
    else if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, VGet(0.0f, 0.0f, 0.0f), 0.0f, m_scale))
    {
        ModelManager::GetInstance().DrawFallbackStage(m_halfWidth, m_halfDepth, m_wallHeight);
    }

    //// 必要なのはDraw3D()の中でこれを呼ぶだけです（当たり判定は発生しません）
    //ModelManager::GetInstance().DrawModelIfLoaded(
    //    "Resource/Sousyoku/ABigTree3.mv1", // 3Dモデルのファイルパス
    //    VGet(300.0f, 0.0f, 500.0f),       // 配置する座標 (X, Y, Z)
    //    0.0f,                             // Y軸の回転角度（ラジアン）
    //    10.0f                            // スケール（大きさ）
    //);

    //ModelManager::GetInstance().DrawModelIfLoaded(
    //    "Resource/Sousyoku/ABigTree3.mv1", // 3Dモデルのファイルパス
    //    VGet(300.0f, 0.0f, -500.0f),       // 配置する座標 (X, Y, Z)
    //    0.0f,                             // Y軸の回転角度（ラジアン）
    //    10.0f                            // スケール（大きさ）
    //);

     // 使いたいモデルのリスト（何個でも追加OK）
    const char* models[] = {
        "Resource/Sousyoku/ABigTree1.mv1",
        "Resource/Sousyoku/ABigTree2.mv1",
        "Resource/Sousyoku/ABigTree3.mv1",
        "Resource/Sousyoku/ASmallTree1.mv1",
    };

    int modelCount = sizeof(models) / sizeof(models[0]);

    static std::vector<VECTOR> treePos;
    if (m_hasCollision && treePos.empty()) {
        for (int i = 0; i < 300 && treePos.size() < 100; ++i) {
            VECTOR p = VGet((rand() % 5000) - 2500.0f, 0.0f, (rand() % 4000) - 2000.0f);
            float dummyY = 0.0f;
            if (!GetGroundHeight(p, dummyY)) {
                treePos.push_back(p);
            }
        }
    }
    // 順番に違う種類のモデルを描画
    for (size_t i = 0; i < treePos.size(); ++i) {
        ModelManager::GetInstance().DrawModelIfLoaded(models[i % modelCount], treePos[i], 0.0f, 40.0f);
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
// （不要なpCube55(Y=50)や壁の上面(Y=200)を誤検出せず、室内床面(Y≈5)のみを正確に検出）
// ============================================================================
bool Stage3D::GetHouseGroundHeight(const VECTOR& pos, float& outGroundY, VECTOR* outGroundNormal) const
{
    if (m_modelHandle == -1 || !m_hasCollision)
    {
        outGroundY = 0.0f;
        return false;
    }

    // Houseの床（Mesh[2], Y≈5.0f）のみを探索するため、Y=30.0f 〜 -30.0f の範囲でレイキャスト
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
// （坂道・通路床(Y≈0〜120)を正確に追跡し、外壁や中央円柱の屋上(Y=200)への誤ワープを防止）
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
