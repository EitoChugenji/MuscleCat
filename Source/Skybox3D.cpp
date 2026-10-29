// ============================================================================
// @file   Skybox3D.cpp
// @brief  3D森のスカイボックス・スカイドーム描画および森林環境パーティクル実装
// ============================================================================
#include "Skybox3D.h"
#include <cmath>
#include <cstdlib>

namespace
{
    constexpr float PI_F = 3.1415926535f;

    // 線形補間ヘルパー
    inline int LerpColor(int c0, int c1, float t)
    {
        if (t <= 0.0f) return c0;
        if (t >= 1.0f) return c1;
        int r0 = (c0 >> 16) & 0xFF, g0 = (c0 >> 8) & 0xFF, b0 = c0 & 0xFF;
        int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
        int r = static_cast<int>(r0 + (r1 - r0) * t);
        int g = static_cast<int>(g0 + (g1 - g0) * t);
        int b = static_cast<int>(b0 + (b1 - b0) * t);
        return GetColor(r, g, b);
    }
}

Skybox3D::Skybox3D()
{
}

Skybox3D::~Skybox3D()
{
    if (m_textureHandle != -1)
    {
        DeleteGraph(m_textureHandle);
        m_textureHandle = -1;
    }
}

void Skybox3D::Init()
{
    if (m_textureHandle == -1)
    {
        GenerateForestTexture();
        BuildDomeMesh();
        InitLeaves();
    }
}

// ============================================================================
// 高精細・森のパノラマテクスチャ生成 (2048 x 1024)
// ============================================================================
void Skybox3D::GenerateForestTexture()
{
    const int texW = 2048;
    const int texH = 1024;

    m_textureHandle = MakeScreen(texW, texH, FALSE);
    if (m_textureHandle == -1)
    {
        return;
    }

    int prevScreen = GetDrawScreen();
    SetDrawScreen(m_textureHandle);
    ClearDrawScreen();

    // ------------------------------------------------------------------------
    // 1. 背景空・大気グラデーション (天頂 〜 地平線 〜 大地)
    // ------------------------------------------------------------------------
    int colZenith   = GetColor(18, 55, 135);   // 天頂: 深く澄んだコバルトブルー
    int colMidSky   = GetColor(45, 120, 215);  // 中空: 爽快なスカイブルー
    int colLowerSky = GetColor(105, 185, 235); // 下空: 明るいシアンブルー
    int colHorizon  = GetColor(165, 222, 215); // 地平線: 朝霧と光が交わる淡いミントシアン
    int colMist     = GetColor(135, 195, 170); // 森林大気層: 木漏れ日に煙るエメラルドミント
    int colGround   = GetColor(16, 38, 22);    // 最下部: 深い森の大地・下草

    for (int y = 0; y < texH; y += 2)
    {
        int col = 0;
        if (y < 220)
        {
            float t = static_cast<float>(y) / 220.0f;
            col = LerpColor(colZenith, colMidSky, t);
        }
        else if (y < 420)
        {
            float t = static_cast<float>(y - 220) / 200.0f;
            col = LerpColor(colMidSky, colLowerSky, t);
        }
        else if (y < 530)
        {
            float t = static_cast<float>(y - 420) / 110.0f;
            col = LerpColor(colLowerSky, colHorizon, t);
        }
        else if (y < 680)
        {
            float t = static_cast<float>(y - 530) / 150.0f;
            col = LerpColor(colHorizon, colMist, t);
        }
        else
        {
            float t = static_cast<float>(y - 680) / 344.0f;
            col = LerpColor(colMist, colGround, t);
        }
        DrawBox(0, y, texW, y + 2, col, TRUE);
    }

    // ------------------------------------------------------------------------
    // 2. 太陽と木漏れ日の光芒（サンシャフト / God Rays）
    // ------------------------------------------------------------------------
    int sunX = 1380;
    int sunY = 280;

    // 太陽の外部光彩・ブルーム
    SetDrawBlendMode(DX_BLENDMODE_ADD, 35);
    DrawCircle(sunX, sunY, 320, GetColor(255, 230, 160), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 65);
    DrawCircle(sunX, sunY, 180, GetColor(255, 240, 180), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
    DrawCircle(sunX, sunY, 90,  GetColor(255, 250, 210), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
    DrawCircle(sunX, sunY, 40,  GetColor(255, 255, 245), TRUE);

    // 木漏れ日の光条（太陽から斜め下に降り注ぐ光の帯）
    for (int i = 0; i < 18; ++i)
    {
        float angle = (45.0f + i * 4.2f + (i % 3) * 1.5f) * (PI_F / 180.0f);
        float len = 700.0f + (i % 5) * 80.0f;
        int alpha = 16 + (i % 4) * 8;
        SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);

        int ex1 = sunX + static_cast<int>(std::cos(angle - 0.025f) * len);
        int ey1 = sunY + static_cast<int>(std::sin(angle - 0.025f) * len);
        int ex2 = sunX + static_cast<int>(std::cos(angle + 0.025f) * len);
        int ey2 = sunY + static_cast<int>(std::sin(angle + 0.025f) * len);

        DrawTriangle(sunX, sunY, ex1, ey1, ex2, ey2, GetColor(255, 240, 180), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ------------------------------------------------------------------------
    // 3. 遠くに浮かぶ白い雲
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
    int cloudPositions[][2] = {
        { 250, 310 }, { 600, 240 }, { 950, 330 }, { 1750, 270 }, { 1980, 320 }
    };
    for (const auto& cp : cloudPositions)
    {
        int cx = cp[0], cy = cp[1];
        int colCloud = GetColor(255, 255, 255);
        DrawCircle(cx, cy, 45, colCloud, TRUE);
        DrawCircle(cx - 35, cy + 8, 32, colCloud, TRUE);
        DrawCircle(cx + 38, cy + 6, 35, colCloud, TRUE);
        DrawCircle(cx + 65, cy + 12, 24, colCloud, TRUE);
        DrawCircle(cx - 58, cy + 14, 20, colCloud, TRUE);
        DrawBox(cx - 55, cy + 10, cx + 65, cy + 32, colCloud, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ------------------------------------------------------------------------
    // 4. 最遠景の山並み（青緑の山影）
    // ------------------------------------------------------------------------
    int colMnt = GetColor(80, 145, 155);
    for (int x = 0; x < texW; ++x)
    {
        float fx = static_cast<float>(x);
        float h = std::sin(fx * 0.004f) * 45.0f
                + std::sin(fx * 0.012f + 1.2f) * 22.0f
                + std::sin(fx * 0.025f + 2.5f) * 12.0f;
        int mntY = 475 + static_cast<int>(h);
        DrawBox(x, mntY, x + 1, 620, colMnt, TRUE);
    }

    // ------------------------------------------------------------------------
    // 5. 遠景の針葉樹林（モミ・スギの稜線）
    // ------------------------------------------------------------------------
    int colConifer = GetColor(38, 92, 65);
    for (int x = 0; x < texW; x += 14)
    {
        float fx = static_cast<float>(x);
        float baseH = std::sin(fx * 0.006f + 0.8f) * 30.0f;
        int treeH = 50 + (x * 17) % 35;
        int tipY = 490 + static_cast<int>(baseH) - treeH;
        int bottomY = 620;

        DrawTriangle(x, tipY, x - 12, bottomY, x + 12, bottomY, colConifer, TRUE);
    }

    // ------------------------------------------------------------------------
    // 6. 中景の豊かな広葉樹林（大樹・豊かな樹冠の重なり）
    // ------------------------------------------------------------------------
    unsigned int forestColors[] = {
        GetColor(22, 68, 40),
        GetColor(32, 88, 52),
        GetColor(45, 115, 66),
        GetColor(58, 138, 78),
        GetColor(28, 76, 44)
    };
    int colorCount = sizeof(forestColors) / sizeof(forestColors[0]);

    // 木々の幹を描画
    for (int x = 10; x < texW; x += 38)
    {
        int trunkW = 6 + (x % 5);
        int trunkTopY = 560 + (x * 11) % 40;
        DrawBox(x - trunkW / 2, trunkTopY, x + trunkW / 2, 780, GetColor(45, 34, 24), TRUE);
    }

    // 樹冠（丸みのある豊かな葉の塊）を重層配置
    for (int layer = 0; layer < 3; ++layer)
    {
        int step = (layer == 0) ? 32 : ((layer == 1) ? 26 : 20);
        int yOffset = (layer == 0) ? 555 : ((layer == 1) ? 590 : 635);

        for (int x = -20; x < texW + 40; x += step)
        {
            int seed = x * 37 + layer * 101;
            int radiusX = 28 + (seed % 28);
            int radiusY = 22 + (seed % 20);
            int cy = yOffset + (seed % 35);
            int cIndex = (seed % colorCount);

            DrawOval(x, cy, radiusX, radiusY, forestColors[cIndex], TRUE);
            // 木漏れ日のハイライト
            SetDrawBlendMode(DX_BLENDMODE_ADD, 30);
            DrawOval(x - radiusX / 4, cy - radiusY / 4, radiusX / 2, radiusY / 2, GetColor(120, 220, 100), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // ------------------------------------------------------------------------
    // 7. 近中景の鬱蒼とした手前の木立・下草
    // ------------------------------------------------------------------------
    int colForeground = GetColor(15, 48, 28);
    for (int x = -10; x < texW + 20; x += 18)
    {
        int r = 24 + (x * 13) % 22;
        int cy = 680 + (x * 7) % 25;
        DrawCircle(x, cy, r, colForeground, TRUE);
    }
    // 地表の塗りつぶし
    DrawBox(0, 710, texW, texH, colGround, TRUE);

    // ------------------------------------------------------------------------
    // 8. 木々の足元に漂う朝霧（フォグ・ミストレイヤー）
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
    for (int y = 580; y < 690; y += 4)
    {
        float t = static_cast<float>(y - 580) / 110.0f;
        int alpha = static_cast<int>(std::sin(t * PI_F) * 75.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, y, texW, y + 4, GetColor(180, 230, 210), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ------------------------------------------------------------------------
    // 9. 天頂の樹冠キャノピー（上を見上げたときに包み込む木々の梢）
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
    int colCanopy = GetColor(12, 42, 24);
    for (int x = 0; x < texW; x += 40)
    {
        int branchLen = 35 + (x * 23) % 65;
        DrawOval(x, 0, 38 + (x % 20), branchLen, colCanopy, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    SetDrawScreen(prevScreen);
}

// ============================================================================
// スカイドームメッシュの構築
// ============================================================================
void Skybox3D::BuildDomeMesh()
{
    m_baseVertices.clear();
    m_drawVertices.clear();
    m_indices.clear();

    const int slices = m_sliceCount; // 48
    const int rings  = m_ringCount;  // 16
    const float R    = m_radius;     // 45000.0f

    // 頂点配列の生成
    // 緯度: 天頂 (phi = +PI/2) 〜 地平線 (phi = 0) 〜 地下 (phi = -PI/4)
    for (int j = 0; j <= rings; ++j)
    {
        float v = static_cast<float>(j) / static_cast<float>(rings);
        // v = 0.0 -> phi = +PI/2 (天頂)
        // v = 0.55 -> phi = 0 (地平線)
        // v = 1.0 -> phi = -PI/4 (地下)
        float phi = 0.0f;
        if (v <= 0.55f)
        {
            float t = v / 0.55f;
            phi = (PI_F * 0.5f) * (1.0f - t);
        }
        else
        {
            float t = (v - 0.55f) / 0.45f;
            phi = -(PI_F * 0.28f) * t;
        }

        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);
        float y = R * sinPhi;

        for (int i = 0; i <= slices; ++i)
        {
            float u = static_cast<float>(i) / static_cast<float>(slices);
            float theta = u * (PI_F * 2.0f);

            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float x = R * cosPhi * sinTheta;
            float z = R * cosPhi * cosTheta;

            VERTEX3D vert{};
            vert.pos = VGet(x, y, z);
            // 法線はドーム内部（原点）を向くように設定
            vert.norm = VGet(-cosPhi * sinTheta, -sinPhi, -cosPhi * cosTheta);
            vert.dif = GetColorU8(255, 255, 255, 255);
            vert.spc = GetColorU8(0, 0, 0, 0);
            vert.u = u;
            vert.v = v;

            m_baseVertices.push_back(vert);
        }
    }

    m_drawVertices = m_baseVertices;

    // インデックス配列の生成（内向きに見えるよう頂点順序を調整）
    for (int j = 0; j < rings; ++j)
    {
        for (int i = 0; i < slices; ++i)
        {
            int row1 = j * (slices + 1);
            int row2 = (j + 1) * (slices + 1);

            int p0 = row1 + i;
            int p1 = row1 + i + 1;
            int p2 = row2 + i + 1;
            int p3 = row2 + i;

            // 三角形 1: p0 -> p1 -> p2 (内側から見て反時計回り)
            m_indices.push_back(static_cast<unsigned short>(p0));
            m_indices.push_back(static_cast<unsigned short>(p1));
            m_indices.push_back(static_cast<unsigned short>(p2));

            // 三角形 2: p0 -> p2 -> p3
            m_indices.push_back(static_cast<unsigned short>(p0));
            m_indices.push_back(static_cast<unsigned short>(p2));
            m_indices.push_back(static_cast<unsigned short>(p3));
        }
    }
}

// ============================================================================
// 木の葉パーティクル初期化
// ============================================================================
void Skybox3D::InitLeaves()
{
    m_leaves.resize(m_leafCount);
    unsigned int leafColors[] = {
        GetColor(90, 200, 70),   // 明るい黄緑
        GetColor(50, 165, 55),   // 鮮やかな新緑
        GetColor(35, 125, 45),   // 深緑
        GetColor(170, 215, 60),  // 若葉色
        GetColor(115, 185, 80)   // オリーブグリーン
    };

    for (size_t i = 0; i < m_leaves.size(); ++i)
    {
        auto& leaf = m_leaves[i];
        float angle = static_cast<float>(rand() % 360) * (PI_F / 180.0f);
        float dist = 80.0f + static_cast<float>(rand() % 520);
        leaf.pos = VGet(
            std::cos(angle) * dist,
            40.0f + static_cast<float>(rand() % 350),
            std::sin(angle) * dist
        );
        leaf.velocity = VGet(
            0.6f + static_cast<float>(rand() % 100) * 0.008f,
            -(0.45f + static_cast<float>(rand() % 100) * 0.005f),
            0.3f + static_cast<float>(rand() % 100) * 0.006f
        );
        leaf.rot = VGet(
            static_cast<float>(rand() % 360) * (PI_F / 180.0f),
            static_cast<float>(rand() % 360) * (PI_F / 180.0f),
            static_cast<float>(rand() % 360) * (PI_F / 180.0f)
        );
        leaf.rotSpeed = VGet(
            0.02f + static_cast<float>(rand() % 40) * 0.001f,
            0.03f + static_cast<float>(rand() % 50) * 0.001f,
            0.015f + static_cast<float>(rand() % 30) * 0.001f
        );
        leaf.size = 2.4f + static_cast<float>(rand() % 20) * 0.1f;
        leaf.swaySpeed = 0.035f + static_cast<float>(rand() % 30) * 0.001f;
        leaf.swayPhase = static_cast<float>(rand() % 360) * (PI_F / 180.0f);
        leaf.color = leafColors[rand() % 5];
    }
}

// ============================================================================
// 木の葉パーティクル更新
// ============================================================================
void Skybox3D::UpdateLeaves(const VECTOR& centerPos)
{
    for (auto& leaf : m_leaves)
    {
        leaf.swayPhase += leaf.swaySpeed;
        float swayX = std::sin(leaf.swayPhase) * 0.55f;
        float swayZ = std::cos(leaf.swayPhase * 0.7f) * 0.35f;

        leaf.pos.x += leaf.velocity.x + swayX;
        leaf.pos.y += leaf.velocity.y;
        leaf.pos.z += leaf.velocity.z + swayZ;

        leaf.rot.x += leaf.rotSpeed.x;
        leaf.rot.y += leaf.rotSpeed.y;
        leaf.rot.z += leaf.rotSpeed.z;

        // プレイヤー周辺の一定範囲外に出たら再配置
        float dx = leaf.pos.x - centerPos.x;
        float dz = leaf.pos.z - centerPos.z;
        float distSq = dx * dx + dz * dz;

        if (leaf.pos.y < centerPos.y - 10.0f || distSq > (650.0f * 650.0f))
        {
            float angle = static_cast<float>(rand() % 360) * (PI_F / 180.0f);
            float dist = 60.0f + static_cast<float>(rand() % 450);
            leaf.pos.x = centerPos.x + std::cos(angle) * dist;
            leaf.pos.y = centerPos.y + 120.0f + static_cast<float>(rand() % 200);
            leaf.pos.z = centerPos.z + std::sin(angle) * dist;
        }
    }
}

void Skybox3D::Update(const VECTOR& centerPos)
{
    UpdateLeaves(centerPos);
}

// ============================================================================
// 木の葉パーティクル描画
// ============================================================================
void Skybox3D::DrawLeaves()
{
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);

    for (const auto& leaf : m_leaves)
    {
        float s = leaf.size;
        // 葉の形状（小さな菱形ポリゴン）
        VECTOR localPts[4] = {
            VGet(0.0f,  0.0f,  s * 1.5f),  // 先端
            VGet(-s * 0.7f, 0.0f,  0.0f),  // 左
            VGet(0.0f,  0.0f, -s * 0.9f),  // 根元
            VGet( s * 0.7f, 0.0f,  0.0f)   // 右
        };

        // 3軸回転マトリクス
        MATRIX matRot = MGetRotY(leaf.rot.y);
        matRot = MMult(matRot, MGetRotX(leaf.rot.x));
        matRot = MMult(matRot, MGetRotZ(leaf.rot.z));

        VECTOR worldPts[4];
        for (int i = 0; i < 4; ++i)
        {
            VECTOR r = VTransform(localPts[i], matRot);
            worldPts[i] = VAdd(leaf.pos, r);
        }

        // 葉の2枚の三角形を描画
        DrawTriangle3D(worldPts[0], worldPts[1], worldPts[2], leaf.color, TRUE);
        DrawTriangle3D(worldPts[0], worldPts[2], worldPts[3], leaf.color, TRUE);
    }

    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}

// ============================================================================
// スカイドーム描画（カメラ追従・最奥描画）
// ============================================================================
void Skybox3D::Draw(const VECTOR& cameraPos)
{
    if (m_textureHandle == -1 || m_baseVertices.empty())
    {
        return;
    }

    // 1. レンダリングステート設定
    int prevBlendMode, prevBlendParam;
    GetDrawBlendMode(&prevBlendMode, &prevBlendParam);

    SetUseLighting(FALSE);         // 自発光（ライトに左右されない）
    SetWriteZBuffer3D(FALSE);      // Zバッファ書き込み無効（最背面に描画）
    SetUseBackCulling(FALSE);      // 両面描画
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    // 2. カメラ座標を中心とするように頂点座標をオフセット
    size_t vertCount = m_baseVertices.size();
    for (size_t i = 0; i < vertCount; ++i)
    {
        m_drawVertices[i].pos.x = m_baseVertices[i].pos.x + cameraPos.x;
        m_drawVertices[i].pos.y = m_baseVertices[i].pos.y + cameraPos.y;
        m_drawVertices[i].pos.z = m_baseVertices[i].pos.z + cameraPos.z;
    }

    // 3. スカイドームメッシュ描画
    int polyCount = static_cast<int>(m_indices.size() / 3);
    DrawPolygonIndexed3D(
        m_drawVertices.data(),
        static_cast<int>(m_drawVertices.size()),
        m_indices.data(),
        polyCount,
        m_textureHandle,
        FALSE
    );

    // 4. レンダリングステート復元
    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetDrawBlendMode(prevBlendMode, prevBlendParam);

    // 5. 森林環境演出：舞い落ちる木の葉を描画
    DrawLeaves();
}
