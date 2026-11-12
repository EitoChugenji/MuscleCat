// ============================================================================
// @file   Common.h
// @brief  プロジェクト全体で共有する基本型・列挙型・数学ヘルパー関数の定義
// ============================================================================
#pragma once
#include "DxLib.h"
#include <cmath>

// ============================================================================
// オブジェクト種別定義
// 3D空間上に配置される各ゲームオブジェクトの識別に使用する
// ============================================================================
enum class ObjectType
{
    Player,       // プレイヤー操作キャラクター（マッスルにゃんこ）
    NormalMouse,  // 通常ネズミ（一定距離内でプレイヤーから逃走）
    FastMouse,    // 高速逃走ネズミ（感知範囲が広く、逃走速度が高い）
    Obstacle      // ステージ上の静的障害物（家具・トレーニング器具など）
};

// ============================================================================
// 数学・ベクトル補助関数群
// 角度変換、クランプ、線形補間、XZ平面距離計算などをインライン提供する
// ============================================================================
namespace MathHelper
{
    // 円周率および角度変換定数（度 <-> ラジアン）
    constexpr float PI         = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // ------------------------------------------------------------------------
    // 値 v を [minVal, maxVal] の範囲内に制限（クランプ）して返す
    // ------------------------------------------------------------------------
    inline float Clamp(float v, float minVal, float maxVal)
    {
        if (v < minVal)
        {
            return minVal;
        }

        if (v > maxVal)
        {
            return maxVal;
        }

        return v;
    }

    // ------------------------------------------------------------------------
    // 2つの値 a, b 間を割合 t (0.0f 〜 1.0f) で線形補間して返す
    // ------------------------------------------------------------------------
    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    // ------------------------------------------------------------------------
    // 3D空間上の2点 a, b 間の水平面（XZ平面）上の直線距離を算出する
    // 高低差（Y座標）を無視した距離判定やAIの視野距離計算に使用する
    // ------------------------------------------------------------------------
    inline float DistanceXZ(const VECTOR& a, const VECTOR& b)
    {
        float dx = a.x - b.x;
        float dz = a.z - b.z;

        return std::sqrt(dx * dx + dz * dz);
    }

    // ------------------------------------------------------------------------
    // 角度 from から角度 to（ラジアン）へ最短回転経路で線形補間する
    // -π 〜 +π の境界をまたぐ際にも反対回りにならないよう正規化して補間する
    // ------------------------------------------------------------------------
    inline float LerpAngle(float from, float to, float t)
    {
        // 角度差を [-π, +π] の範囲に正規化
        float diff = std::fmod(to - from + PI, PI * 2.0f);

        if (diff < 0.0f)
        {
            diff += PI * 2.0f;
        }

        diff -= PI;

        return from + diff * t;
    }

    // ------------------------------------------------------------------------
    // ??????????????? / ???????????3D???????
    // ?????????????????????????????????2?????????
    // ------------------------------------------------------------------------
    inline void DrawFakeShadow3D(
        const VECTOR& groundPos,
        float radiusX,
        float radiusZ,
        float rotY = 0.0f,
        int centerAlpha = 145,
        const VECTOR& groundNormal = VGet(0.0f, 1.0f, 0.0f)
    )
    {
        if (centerAlpha <= 0 || radiusX <= 0.1f || radiusZ <= 0.1f)
        {
            return;
        }
        if (centerAlpha > 230) centerAlpha = 230;

        constexpr int SLICES = 16;
        constexpr int VERT_COUNT = 1 + SLICES * 2;
        constexpr int IDX_COUNT  = SLICES * 3 + SLICES * 6;

        VERTEX3D verts[VERT_COUNT];
        unsigned short indices[IDX_COUNT];

        // Z???????????????????????????? Y+0.35f ??????
        VECTOR basePos = VAdd(groundPos, VScale(groundNormal, 0.45f));

        MATRIX mRotY = MGetRotY(rotY);
        VECTOR up = VGet(0.0f, 1.0f, 0.0f);
        VECTOR axis = VCross(up, groundNormal);
        float axisLen = VSize(axis);
        MATRIX mAlign = MGetIdent();
        if (axisLen > 0.001f)
        {
            axis = VScale(axis, 1.0f / axisLen);
            float dot = Clamp(VDot(up, groundNormal), -1.0f, 1.0f);
            mAlign = MGetRotAxis(axis, std::acos(dot));
        }
        MATRIX mTransform = MMult(mRotY, mAlign);

        COLOR_U8 colCenter = GetColorU8(10, 12, 18, static_cast<unsigned char>(centerAlpha));
        COLOR_U8 colMid    = GetColorU8(10, 12, 18, static_cast<unsigned char>(centerAlpha * 0.68f));
        COLOR_U8 colOuter  = GetColorU8(10, 12, 18, 0);
        COLOR_U8 colSpc    = GetColorU8(0, 0, 0, 0);

        verts[0].pos  = basePos;
        verts[0].norm = groundNormal;
        verts[0].dif  = colCenter;
        verts[0].spc  = colSpc;
        verts[0].u    = 0.5f;
        verts[0].v    = 0.5f;
        verts[0].su   = 0.0f;
        verts[0].sv   = 0.0f;

        for (int i = 0; i < SLICES; ++i)
        {
            float angle = (static_cast<float>(i) / static_cast<float>(SLICES)) * (PI * 2.0f);
            float s = std::sin(angle);
            float c = std::cos(angle);

            VECTOR localMid   = VGet(s * radiusX * 0.62f, 0.0f, c * radiusZ * 0.62f);
            VECTOR localOuter = VGet(s * radiusX,         0.0f, c * radiusZ);

            VERTEX3D& vMid = verts[1 + i];
            vMid.pos  = VAdd(basePos, VTransform(localMid, mTransform));
            vMid.norm = groundNormal;
            vMid.dif  = colMid;
            vMid.spc  = colSpc;
            vMid.u    = 0.5f;
            vMid.v    = 0.5f;
            vMid.su   = 0.0f;
            vMid.sv   = 0.0f;

            VERTEX3D& vOut = verts[1 + SLICES + i];
            vOut.pos  = VAdd(basePos, VTransform(localOuter, mTransform));
            vOut.norm = groundNormal;
            vOut.dif  = colOuter;
            vOut.spc  = colSpc;
            vOut.u    = 0.5f;
            vOut.v    = 0.5f;
            vOut.su   = 0.0f;
            vOut.sv   = 0.0f;
        }

        int idx = 0;
        for (int i = 0; i < SLICES; ++i)
        {
            int next = (i + 1) % SLICES;
            // ??????
            indices[idx++] = 0;
            indices[idx++] = static_cast<unsigned short>(1 + i);
            indices[idx++] = static_cast<unsigned short>(1 + next);

            // ??????????
            int m0 = 1 + i;
            int m1 = 1 + next;
            int o0 = 1 + SLICES + i;
            int o1 = 1 + SLICES + next;

            indices[idx++] = static_cast<unsigned short>(m0);
            indices[idx++] = static_cast<unsigned short>(o0);
            indices[idx++] = static_cast<unsigned short>(m1);

            indices[idx++] = static_cast<unsigned short>(m1);
            indices[idx++] = static_cast<unsigned short>(o0);
            indices[idx++] = static_cast<unsigned short>(o1);
        }

        int prevBlendMode, prevBlendParam;
        GetDrawBlendMode(&prevBlendMode, &prevBlendParam);

        SetUseLighting(FALSE);
        SetWriteZBuffer3D(FALSE);
        SetUseBackCulling(FALSE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);

        DrawPolygonIndexed3D(verts, VERT_COUNT, indices, IDX_COUNT / 3, DX_NONE_GRAPH, TRUE);

        SetUseBackCulling(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetUseLighting(TRUE);
        SetDrawBlendMode(prevBlendMode, prevBlendParam);
    }
}
