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
{
}

Stage3D::~Stage3D()
{
    if (m_modelHandle != -1)
    {
        MV1DeleteModel(m_modelHandle);
        m_modelHandle = -1;
        m_hasCollision = false;
        m_wallPolygons.clear();
    }
}

void Stage3D::Init()
{
    // 画面クリア背景色（アニメ調の明るいパステルブルー・空色）
    SetBackgroundColor(175, 218, 252);

    // 3Dライティング設定（アニメ・セルルック風：明るい環境光と高コントラスト）
    SetUseLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.86f, 0.86f, 0.88f, 1.0f));
    ChangeLightTypeDir(VGet(0.35f, -0.85f, 0.4f));
    SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    SetLightSpcColor(GetColorF(0.7f, 0.7f, 0.7f, 1.0f));

    // フォグ設定（アニメ調の澄んだ空気感）
    SetFogEnable(TRUE);
    SetFogColor(175, 218, 252);
    SetFogStartEnd(1800.0f, 4200.0f);

    // 既存モデルハンドルの破棄
    if (m_modelHandle != -1)
    {
        MV1DeleteModel(m_modelHandle);
        m_modelHandle = -1;
        m_hasCollision = false;
        m_wallPolygons.clear();
    }

    // ステージ3Dモデル読み込みとコリジョン情報の構築
    int baseHandle = ModelManager::GetInstance().LoadModelHandle(m_modelPath);
    if (baseHandle != -1)
    {
        m_modelHandle = MV1DuplicateModel(baseHandle);
        if (m_modelHandle != -1)
        {
            float scale = ModelConfig::STAGE_MODEL_SCALE;
            MV1SetPosition(m_modelHandle, VGet(0.0f, 0.0f, 0.0f));
            MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, 0.0f, 0.0f));
            MV1SetScale(m_modelHandle, VGet(scale, scale, scale));

            // コリジョン情報の構築（空間分割: 32 x 16 x 32）
            MV1SetupCollInfo(m_modelHandle, -1, 32, 16, 32);
            m_hasCollision = true;

            int meshNum = MV1GetMeshNum(m_modelHandle);
            VECTOR totalMin = VGet(1e9f, 1e9f, 1e9f);
            VECTOR totalMax = VGet(-1e9f, -1e9f, -1e9f);
            for (int m = 0; m < meshNum; ++m)
            {
                VECTOR bMin = MV1GetMeshMinPosition(m_modelHandle, m);
                VECTOR bMax = MV1GetMeshMaxPosition(m_modelHandle, m);
                if (bMin.x < totalMin.x) totalMin.x = bMin.x;
                if (bMin.y < totalMin.y) totalMin.y = bMin.y;
                if (bMin.z < totalMin.z) totalMin.z = bMin.z;
                if (bMax.x > totalMax.x) totalMax.x = bMax.x;
                if (bMax.y > totalMax.y) totalMax.y = bMax.y;
                if (bMax.z > totalMax.z) totalMax.z = bMax.z;
            }
            m_minBounds = VGet(totalMin.x * scale, totalMin.y * scale, totalMin.z * scale);
            m_maxBounds = VGet(totalMax.x * scale, totalMax.y * scale, totalMax.z * scale);

            AppLogAdd("=== STAGE MODEL LOADED: %s ===\n", m_modelPath.c_str());
            AppLogAdd("Meshes: %d, Scale: %.1f\n", meshNum, scale);
            AppLogAdd("Bounds Min: (%.1f, %.1f, %.1f), Max: (%.1f, %.1f, %.1f)\n",
                m_minBounds.x, m_minBounds.y, m_minBounds.z,
                m_maxBounds.x, m_maxBounds.y, m_maxBounds.z);

            for (int m = 0; m < meshNum; ++m)
            {
                VECTOR bMin = MV1GetMeshMinPosition(m_modelHandle, m);
                VECTOR bMax = MV1GetMeshMaxPosition(m_modelHandle, m);
                int polyNum = MV1GetMeshTriangleNum(m_modelHandle, m);
                AppLogAdd("  Mesh[%d]: polyNum=%d, Min=(%.1f, %.1f, %.1f), Max=(%.1f, %.1f, %.1f)\n",
                    m, polyNum,
                    bMin.x * scale, bMin.y * scale, bMin.z * scale,
                    bMax.x * scale, bMax.y * scale, bMax.z * scale);
            }

            // モデルから直接参照ポリゴンを取得し、面法線による壁ポリゴンリストを構築
            m_wallPolygons.clear();

            auto CalcMin3 = [](float a, float b, float c) -> float {
                float m = (a < b) ? a : b;
                return (m < c) ? m : c;
            };
            auto CalcMax3 = [](float a, float b, float c) -> float {
                float m = (a > b) ? a : b;
                return (m > c) ? m : c;
            };

            std::vector<int> meshWallCount(meshNum, 0);

            auto ExtractPolygons = [&](const MV1_REF_POLYGONLIST& ref) {
                if (ref.PolygonNum <= 0 || ref.Polygons == nullptr || ref.Vertexs == nullptr)
                {
                    return;
                }

                for (int i = 0; i < ref.PolygonNum; ++i)
                {
                    const auto& poly = ref.Polygons[i];

                    // 【床メッシュの除外】
                    // 親メッシュの上端が低いもの（床メッシュ: Max.Y=5.0f）や高さが小さいものは、
                    // 床ブロックの側面や分け目なので壁として抽出しない
                    if (poly.MeshIndex < meshNum)
                    {
                        VECTOR bMin = MV1GetMeshMinPosition(m_modelHandle, poly.MeshIndex);
                        VECTOR bMax = MV1GetMeshMaxPosition(m_modelHandle, poly.MeshIndex);
                        float meshMaxY = bMax.y * scale;
                        float meshHeight = (bMax.y - bMin.y) * scale;
                        if (meshMaxY < 100.0f || meshHeight < 50.0f)
                        {
                            continue; // 床メッシュ（分け目の垂直面）を除外
                        }
                    }

                    VECTOR v0 = ref.Vertexs[poly.VIndex[0]].Position;
                    VECTOR v1 = ref.Vertexs[poly.VIndex[1]].Position;
                    VECTOR v2 = ref.Vertexs[poly.VIndex[2]].Position;

                    float polyMinY = CalcMin3(v0.y, v1.y, v2.y);
                    float polyMaxY = CalcMax3(v0.y, v1.y, v2.y);
                    float polyHeight = polyMaxY - polyMinY;

                    // 【ポリゴン単位の床分け目除外】
                    // 本物の壁は Y=0 〜 Y=200 まで伸びている。
                    // 床の分け目や段差（上端が50未満、または高さが40未満）は除外
                    if (polyMaxY < 50.0f || polyHeight < 40.0f)
                    {
                        continue;
                    }

                    // 面法線の計算 (v1 - v0) x (v2 - v0)
                    VECTOR e1 = VSub(v1, v0);
                    VECTOR e2 = VSub(v2, v0);
                    VECTOR cross = VCross(e1, e2);
                    float lenSq = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;
                    if (lenSq < 0.000001f)
                    {
                        continue; // 面積が0の縮退ポリゴンは除外
                    }

                    float invLen = 1.0f / std::sqrt(lenSq);
                    VECTOR normal = VGet(cross.x * invLen, cross.y * invLen, cross.z * invLen);

                    // 壁ポリゴンの判定:
                    // 法線のY成分の絶対値が 0.65 未満のものを壁（垂直壁・急勾配）とする
                    if (std::abs(normal.y) < 0.65f)
                    {
                        WallPolygon wp;
                        wp.v0 = v0;
                        wp.v1 = v1;
                        wp.v2 = v2;
                        wp.normal = normal;
                        wp.minPos = VGet(CalcMin3(v0.x, v1.x, v2.x), polyMinY, CalcMin3(v0.z, v1.z, v2.z));
                        wp.maxPos = VGet(CalcMax3(v0.x, v1.x, v2.x), polyMaxY, CalcMax3(v0.z, v1.z, v2.z));
                        m_wallPolygons.push_back(wp);

                        if (poly.MeshIndex < meshNum)
                        {
                            meshWallCount[poly.MeshIndex]++;
                        }
                    }
                }
            };

            // 1. モデル全体の参照メッシュを取得（IsTransform = TRUE でスケール反映済み）
            if (MV1SetupReferenceMesh(m_modelHandle, -1, TRUE) >= 0)
            {
                MV1_REF_POLYGONLIST refAll = MV1GetReferenceMesh(m_modelHandle, -1, TRUE);
                ExtractPolygons(refAll);
                MV1TerminateReferenceMesh(m_modelHandle, -1, TRUE);
            }

            for (int m = 0; m < meshNum; ++m)
            {
                if (meshWallCount[m] > 0)
                {
                    AppLogAdd("  Mesh[%d] produced %d wall polygons\n", m, meshWallCount[m]);
                }
            }

            int lowPolyCount = 0;
            int highPolyCount = 0;
            int midPolyCount = 0;
            for (const auto& wp : m_wallPolygons)
            {
                if (wp.maxPos.y <= 4.0f) lowPolyCount++;
                else if (wp.minPos.y >= 35.0f) highPolyCount++;
                else midPolyCount++;
            }

            AppLogAdd("Stage wall polygons: total=%d (floor-level<=4: %d, player-level: %d, ceiling-level>=35: %d)\n",
                static_cast<int>(m_wallPolygons.size()), lowPolyCount, midPolyCount, highPolyCount);
            if (!m_wallPolygons.empty())
            {
                AppLogAdd("Sample Wall Polygon 0: v0=(%.1f, %.1f, %.1f), v1=(%.1f, %.1f, %.1f), normal=(%.2f, %.2f, %.2f)\n",
                    m_wallPolygons[0].v0.x, m_wallPolygons[0].v0.y, m_wallPolygons[0].v0.z,
                    m_wallPolygons[0].v1.x, m_wallPolygons[0].v1.y, m_wallPolygons[0].v1.z,
                    m_wallPolygons[0].normal.x, m_wallPolygons[0].normal.y, m_wallPolygons[0].normal.z);
            }
        }
    }
}

void Stage3D::Draw3D()
{
    if (m_modelHandle != -1)
    {
        MV1DrawModel(m_modelHandle);
    }
    else if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, VGet(0.0f, 0.0f, 0.0f), 0.0f, ModelConfig::STAGE_MODEL_SCALE))
    {
        ModelManager::GetInstance().DrawFallbackStage(m_halfWidth, m_halfDepth, m_wallHeight);
    }
}

bool Stage3D::ResolveWallCollision(VECTOR& outPos, float radius, VECTOR* outPushNormal) const
{
    bool anyCollided = false;
    if (outPushNormal)
    {
        *outPushNormal = VGet(0.0f, 0.0f, 0.0f);
    }

    if (!m_wallPolygons.empty())
    {
        // 押し戻しを最大4回反復（各反復で最もめり込みが大きい壁を正確に押し出し、オーバープッシュやコーナーの挟まりを解消）
        for (int iter = 0; iter < 4; ++iter)
        {
            float maxPenetration = 0.0f;
            float bestPushX = 0.0f;
            float bestPushZ = 0.0f;

            float charHeight = (radius * 2.0f > 24.0f) ? (radius * 2.0f) : 24.0f;
            float catBottomY = outPos.y;
            float catTopY    = outPos.y + charHeight;

            for (const auto& poly : m_wallPolygons)
            {
                // AABB による高速カリング（キャラクター判定範囲外のポリゴンは即座にスキップ）
                if (outPos.x + radius < poly.minPos.x || outPos.x - radius > poly.maxPos.x ||
                    outPos.z + radius < poly.minPos.z || outPos.z - radius > poly.maxPos.z ||
                    catTopY < poly.minPos.y || catBottomY > poly.maxPos.y)
                {
                    continue;
                }

                // 足元より下、または頭より上にある無関係なポリゴン（床の凹凸や天井・梁）は除外
                if (poly.maxPos.y < catBottomY || poly.minPos.y > catTopY)
                {
                    continue;
                }

                // 三角形の高さに合わせてキャラクター中心線上の最適なテスト点Pを決定
                float polyMidY = (poly.v0.y + poly.v1.y + poly.v2.y) * 0.333333f;
                float testY = catBottomY + radius * 0.5f;
                if (testY < poly.minPos.y) testY = poly.minPos.y + 0.1f;
                if (testY > poly.maxPos.y) testY = poly.maxPos.y - 0.1f;

                VECTOR p = VGet(outPos.x, testY, outPos.z);
                VECTOR q = ClosestPointOnTriangle(p, poly.v0, poly.v1, poly.v2);

                // 三角形最近接点 Q とキャラクター位置 P の水平距離
                float dx = outPos.x - q.x;
                float dz = outPos.z - q.z;
                float distSq = dx * dx + dz * dz;

                // 水平判定半径以内で壁と交差しているか判定（垂直壁のため水平距離で判定）
                if (distSq < radius * radius)
                {
                    float dist = std::sqrt(distSq);
                    float penetration = radius - dist;

                    if (penetration > maxPenetration)
                    {
                        // 離反方向ベクトル（三角形の接触点から離れる方向）
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

                        // ポリゴンの水平面法線
                        float nx = poly.normal.x;
                        float nz = poly.normal.z;
                        float nLen = std::sqrt(nx * nx + nz * nz);
                        if (nLen > 0.0001f)
                        {
                            nx /= nLen;
                            nz /= nLen;
                        }

                        // 法線の向きをプレイヤーがいる側（表・裏どちらから来ても自然に離れるよう）に整列
                        float dot = awayX * nx + awayZ * nz;
                        float alignNx = (dot >= 0.0f) ? nx : -nx;
                        float alignNz = (dot >= 0.0f) ? nz : -nz;

                        // 法線と離反ベクトルを滑らかにブレンド
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
                break; // めり込み解消完了
            }
        }
    }
    else if (m_hasCollision && m_modelHandle != -1)
    {
        // フォールバック: MV1CollCheck_Capsule
        for (int iter = 0; iter < 3; ++iter)
        {
            float topH = (radius * 0.9f > 16.0f) ? (radius * 0.9f) : 16.0f;
            float topY = outPos.y + topH;
            VECTOR capBottom = VGet(outPos.x, outPos.y + 4.0f, outPos.z);
            VECTOR capTop    = VGet(outPos.x, topY, outPos.z);

            MV1_COLL_RESULT_POLY_DIM hit = MV1CollCheck_Capsule(m_modelHandle, -1, capBottom, capTop, radius);
            if (hit.HitNum <= 0)
            {
                MV1CollResultPolyDimTerminate(hit);
                break;
            }

            bool iterationPushed = false;

            for (int i = 0; i < hit.HitNum; ++i)
            {
                const auto& poly = hit.Dim[i];

                if (std::abs(poly.Normal.y) < 0.65f)
                {
                    float nx = poly.Normal.x;
                    float nz = poly.Normal.z;
                    float lenSq = nx * nx + nz * nz;
                    if (lenSq > 0.0001f)
                    {
                        float invLen = 1.0f / std::sqrt(lenSq);
                        nx *= invLen;
                        nz *= invLen;

                        float dx = outPos.x - poly.HitPosition.x;
                        float dz = outPos.z - poly.HitPosition.z;
                        float distSq = dx * dx + dz * dz;

                        if (distSq < radius * radius)
                        {
                            float dist = std::sqrt(distSq);
                            float push = radius - dist;

                            float dot = dx * nx + dz * nz;
                            float pushX = (dot >= 0.0f) ? nx : -nx;
                            float pushZ = (dot >= 0.0f) ? nz : -nz;

                            if (dist > 0.001f)
                            {
                                float awayX = dx / dist;
                                float awayZ = dz / dist;
                                pushX = (pushX + awayX) * 0.5f;
                                pushZ = (pushZ + awayZ) * 0.5f;
                                float pLen = std::sqrt(pushX * pushX + pushZ * pushZ);
                                if (pLen > 0.0001f)
                                {
                                    pushX /= pLen;
                                    pushZ /= pLen;
                                }
                            }

                            outPos.x += pushX * push;
                            outPos.z += pushZ * push;
                            iterationPushed = true;
                            anyCollided = true;
                        }
                    }
                }
            }

            MV1CollResultPolyDimTerminate(hit);

            if (!iterationPushed)
            {
                break;
            }
        }
    }

    // セーフティネット：外周境界ボックス
    VECTOR prevPos = outPos;
    if (ClampToBounds(outPos, radius))
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

bool Stage3D::ClampToBounds(VECTOR& outPos, float radius) const
{
    bool collided = false;
    float minX = m_minBounds.x + radius;
    float maxX = m_maxBounds.x - radius;
    float minZ = m_minBounds.z + radius;
    float maxZ = m_maxBounds.z - radius;

    if (outPos.x < minX)
    {
        outPos.x = minX;
        collided = true;
    }

    if (outPos.x > maxX)
    {
        outPos.x = maxX;
        collided = true;
    }

    if (outPos.z < minZ)
    {
        outPos.z = minZ;
        collided = true;
    }

    if (outPos.z > maxZ)
    {
        outPos.z = maxZ;
        collided = true;
    }

    return collided;
}