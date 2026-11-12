// ============================================================================
// @file   Skybox3D.cpp
// @brief  3D???????????????????????????????
// ============================================================================
#include "Skybox3D.h"
#include <cmath>
#include <cstdlib>

namespace
{
    constexpr float PI_F = 3.1415926535f;

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
// ????????????????????????????? (2048 x 1024)
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

    // 1. ????????? (?? y=0 ? ??? y=740 ? ?? y=1024)
    int colZenith   = GetColor(24, 92, 210);   // ??: ??????????????
    int colUpperSky = GetColor(52, 138, 238);  // ??: ???????????
    int colMidSky   = GetColor(98, 178, 248);  // ??: ?????????
    int colLowerSky = GetColor(156, 214, 252); // ??: ?????????????
    int colHorizon  = GetColor(212, 238, 252); // ???: ????????????????????
    int colGround   = GetColor(56, 108, 64);   // ????: ??????

    for (int y = 0; y < texH; y += 2)
    {
        int col = 0;
        if (y < 220)
        {
            float t = static_cast<float>(y) / 220.0f;
            col = LerpColor(colZenith, colUpperSky, t);
        }
        else if (y < 460)
        {
            float t = static_cast<float>(y - 220) / 240.0f;
            col = LerpColor(colUpperSky, colMidSky, t);
        }
        else if (y < 640)
        {
            float t = static_cast<float>(y - 460) / 180.0f;
            col = LerpColor(colMidSky, colLowerSky, t);
        }
        else if (y < 740)
        {
            float t = static_cast<float>(y - 640) / 100.0f;
            col = LerpColor(colLowerSky, colHorizon, t);
        }
        else
        {
            float t = static_cast<float>(y - 740) / 284.0f;
            col = LerpColor(colHorizon, colGround, t);
        }
        DrawBox(0, y, texW, y + 2, col, TRUE);
    }

    // 2. ????????????????????
    int sunX = 1420;
    int sunY = 340;

    SetDrawBlendMode(DX_BLENDMODE_ADD, 40);
    DrawCircle(sunX, sunY, 340, GetColor(255, 235, 175), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 75);
    DrawCircle(sunX, sunY, 190, GetColor(255, 245, 200), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 140);
    DrawCircle(sunX, sunY, 95,  GetColor(255, 252, 225), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 235);
    DrawCircle(sunX, sunY, 44,  GetColor(255, 255, 250), TRUE);

    for (int i = 0; i < 16; ++i)
    {
        float angle = (i * 22.5f) * (PI_F / 180.0f);
        float len = 480.0f + (i % 3) * 90.0f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 22);
        int ex1 = sunX + static_cast<int>(std::cos(angle - 0.03f) * len);
        int ey1 = sunY + static_cast<int>(std::sin(angle - 0.03f) * len);
        int ex2 = sunX + static_cast<int>(std::cos(angle + 0.03f) * len);
        int ey2 = sunY + static_cast<int>(std::sin(angle + 0.03f) * len);
        DrawTriangle(sunX, sunY, ex1, ey1, ex2, ey2, GetColor(255, 248, 210), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 3. ????????????????????TPS???????? y=320?640 ???????
    auto drawFluffyCloud = [](int cx, int cy, float scale)
    {
        int colShadow = GetColor(188, 212, 238);
        int colMain   = GetColor(246, 250, 255);
        int colPure   = GetColor(255, 255, 255);

        int rMain = static_cast<int>(56.0f * scale);
        int rSub1 = static_cast<int>(44.0f * scale);
        int rSub2 = static_cast<int>(48.0f * scale);
        int rSide = static_cast<int>(34.0f * scale);
        int ox1   = static_cast<int>(46.0f * scale);
        int ox2   = static_cast<int>(82.0f * scale);
        int oy    = static_cast<int>(12.0f * scale);

        // ?????????
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
        DrawOval(cx, cy + oy + 8, ox2 + rSide / 2, rSide / 2 + 6, colShadow, TRUE);

        // ???????????????
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 235);
        DrawCircle(cx - ox2, cy + oy + 4, rSide, colMain, TRUE);
        DrawCircle(cx + ox2, cy + oy + 4, rSide, colMain, TRUE);
        DrawCircle(cx - ox1, cy + 4,      rSub1, colMain, TRUE);
        DrawCircle(cx + ox1, cy + 2,      rSub2, colMain, TRUE);
        DrawCircle(cx,       cy - 6,      rMain, colPure, TRUE);

        // ??????????
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 250);
        DrawCircle(cx - ox1 / 2, cy - 12, static_cast<int>(rMain * 0.72f), colPure, TRUE);
        DrawCircle(cx + ox1 / 2, cy - 10, static_cast<int>(rSub2 * 0.72f), colPure, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    };

    struct CloudDef { int x; int y; float s; };
    const CloudDef clouds[] = {
        {  140, 520, 1.25f }, {  380, 410, 0.95f }, {  640, 540, 1.35f },
        {  920, 440, 1.10f }, { 1180, 530, 1.20f }, { 1540, 490, 1.05f },
        { 1790, 420, 1.30f }, { 1960, 535, 1.10f },
        {  260, 290, 0.80f }, {  780, 270, 0.85f }, { 1310, 260, 0.75f }, { 1680, 285, 0.85f }
    };
    for (const auto& c : clouds)
    {
        drawFluffyCloud(c.x, c.y, c.s);
    }

    // 4. ??????y=680?780????????????????
    int colFarMnt  = GetColor(132, 184, 214);
    int colNearMnt = GetColor(96, 156, 148);
    for (int x = 0; x < texW; ++x)
    {
        float fx = static_cast<float>(x);
        float h1 = std::sin(fx * 0.006f) * 28.0f + std::sin(fx * 0.015f + 1.1f) * 14.0f;
        int mntY1 = 695 + static_cast<int>(h1);
        DrawBox(x, mntY1, x + 1, 780, colFarMnt, TRUE);

        float h2 = std::sin(fx * 0.009f + 2.0f) * 20.0f + std::sin(fx * 0.022f + 0.5f) * 10.0f;
        int mntY2 = 725 + static_cast<int>(h2);
        DrawBox(x, mntY2, x + 1, 820, colNearMnt, TRUE);
    }

    // ??????????????
    for (int y = 700; y < 770; y += 2)
    {
        float t = static_cast<float>(y - 700) / 70.0f;
        int alpha = static_cast<int>(std::sin(t * PI_F) * 90.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, y, texW, y + 2, GetColor(215, 240, 252), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    SetDrawScreen(prevScreen);
}

// ============================================================================
// ?????????????
// ============================================================================
void Skybox3D::BuildDomeMesh()
{
    m_baseVertices.clear();
    m_drawVertices.clear();
    m_indices.clear();

    const int slices = m_sliceCount; // 48
    const int rings  = m_ringCount;  // 16
    const float R    = m_radius;     // 12000.0f

    for (int j = 0; j <= rings; ++j)
    {
        float v = static_cast<float>(j) / static_cast<float>(rings);
        // v = 0.0  -> phi = +PI/2 (??)
        // v = 0.72 -> phi = 0     (???: ????? y=737 ??)
        // v = 1.0  -> phi = -PI/5 (????)
        float phi = 0.0f;
        if (v <= 0.72f)
        {
            float t = v / 0.72f;
            phi = (PI_F * 0.5f) * (1.0f - t);
        }
        else
        {
            float t = (v - 0.72f) / 0.28f;
            phi = -(PI_F * 0.22f) * t;
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
            vert.pos  = VGet(x, y, z);
            vert.norm = VGet(-cosPhi * sinTheta, -sinPhi, -cosPhi * cosTheta);
            vert.dif  = GetColorU8(255, 255, 255, 255);
            vert.spc  = GetColorU8(0, 0, 0, 0);
            vert.u    = u;
            vert.v    = v;

            m_baseVertices.push_back(vert);
        }
    }

    m_drawVertices = m_baseVertices;

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

            m_indices.push_back(static_cast<unsigned short>(p0));
            m_indices.push_back(static_cast<unsigned short>(p1));
            m_indices.push_back(static_cast<unsigned short>(p2));

            m_indices.push_back(static_cast<unsigned short>(p0));
            m_indices.push_back(static_cast<unsigned short>(p2));
            m_indices.push_back(static_cast<unsigned short>(p3));
        }
    }
}

void Skybox3D::InitLeaves()
{
    m_leaves.resize(m_leafCount);
    unsigned int leafColors[] = {
        GetColor(90, 200, 70),
        GetColor(50, 165, 55),
        GetColor(35, 125, 45),
        GetColor(170, 215, 60),
        GetColor(115, 185, 80)
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

void Skybox3D::DrawLeaves()
{
    SetUseLighting(FALSE);
    SetUseBackCulling(FALSE);

    for (const auto& leaf : m_leaves)
    {
        float s = leaf.size;
        VECTOR localPts[4] = {
            VGet(0.0f,  0.0f,  s * 1.5f),
            VGet(-s * 0.7f, 0.0f,  0.0f),
            VGet(0.0f,  0.0f, -s * 0.9f),
            VGet( s * 0.7f, 0.0f,  0.0f)
        };

        MATRIX matRot = MGetRotY(leaf.rot.y);
        matRot = MMult(matRot, MGetRotX(leaf.rot.x));
        matRot = MMult(matRot, MGetRotZ(leaf.rot.z));

        VECTOR worldPts[4];
        for (int i = 0; i < 4; ++i)
        {
            VECTOR r = VTransform(localPts[i], matRot);
            worldPts[i] = VAdd(leaf.pos, r);
        }

        DrawTriangle3D(worldPts[0], worldPts[1], worldPts[2], leaf.color, TRUE);
        DrawTriangle3D(worldPts[0], worldPts[2], worldPts[3], leaf.color, TRUE);
    }

    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
}

// ============================================================================
// ???????????????Z???????????????????
// ============================================================================
void Skybox3D::Draw(const VECTOR& cameraPos)
{
    if (m_textureHandle == -1 || m_baseVertices.empty())
    {
        return;
    }

    int prevBlendMode, prevBlendParam;
    GetDrawBlendMode(&prevBlendMode, &prevBlendParam);

    // ????????????????????????Z????????
    SetFogEnable(FALSE);
    SetUseLighting(FALSE);
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);
    SetUseBackCulling(FALSE);
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    size_t vertCount = m_baseVertices.size();
    for (size_t i = 0; i < vertCount; ++i)
    {
        m_drawVertices[i].pos.x = m_baseVertices[i].pos.x + cameraPos.x;
        m_drawVertices[i].pos.y = m_baseVertices[i].pos.y + cameraPos.y;
        m_drawVertices[i].pos.z = m_baseVertices[i].pos.z + cameraPos.z;
    }

    int polyCount = static_cast<int>(m_indices.size() / 3);
    DrawPolygonIndexed3D(
        m_drawVertices.data(),
        static_cast<int>(m_drawVertices.size()),
        m_indices.data(),
        polyCount,
        m_textureHandle,
        FALSE
    );

    SetUseBackCulling(TRUE);
    SetUseLighting(TRUE);
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetFogEnable(TRUE);
    SetDrawBlendMode(prevBlendMode, prevBlendParam);

    DrawLeaves();
}
