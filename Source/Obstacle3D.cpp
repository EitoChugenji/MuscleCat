// ============================================================================
// @file   Obstacle3D.cpp
// @brief  Obstacle3D クラスの実装（描画およびAABB対円の押し出し衝突解決）
// ============================================================================
#include "Obstacle3D.h"
#include "ModelManager.h"
#include "FontManager.h"
#include "Common.h"
#include <algorithm>

// ============================================================================
// コンストラクタ
// ============================================================================
Obstacle3D::Obstacle3D(
    const VECTOR& pos,
    float width,
    float height,
    float depth,
    const std::string& name,
    const std::string& modelPath,
    unsigned int mainColor,
    unsigned int frameColor,
    float scale,
    float rotY,
    const VECTOR& modelOffset
)
    : GameObject3D(pos, (width + depth) * 0.25f, ObjectType::Obstacle)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
    , m_name(name)
    , m_modelPath(modelPath)
    , m_mainColor(mainColor)
    , m_frameColor(frameColor)
    , m_scale(scale)
    , m_modelOffset(modelOffset)
{
    m_rotY = rotY;
}

// ============================================================================
// 3D空間での障害物描画（モデルが存在すればモデル描画、無ければプリミティブ描画）
// ============================================================================
void Obstacle3D::Draw3D()
{
    VECTOR drawPos = VAdd(m_pos, m_modelOffset);
    if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, drawPos, m_rotY, m_scale))
    {
        ModelManager::GetInstance().DrawFallbackObstacle(m_pos, m_width, m_height, m_depth, m_mainColor, m_frameColor, m_name);
    }
}

// ============================================================================
// 2D頭上ネームラベル描画（3D座標からスクリーン座標へ投影）
// ============================================================================
void Obstacle3D::Draw2D()
{
    // 名前が設定されていない場合は描画しない
    if (m_name.empty())
    {
        return;
    }

    VECTOR headPos = VGet(m_pos.x, m_pos.y + m_height + 14.0f, m_pos.z);
    VECTOR screenPos = ConvWorldPosToScreenPos(headPos);

    // カメラの前方（0.0 < z < 1.0）にある場合のみ描画
    if (screenPos.z > 0.0f && screenPos.z < 1.0f)
    {
        int font13 = FontManager::GetInstance().GetFont13();
        int strWidth = GetDrawStringWidthToHandle(m_name.c_str(), static_cast<int>(m_name.length()), font13);
        int drawX = static_cast<int>(screenPos.x) - strWidth / 2;
        int drawY = static_cast<int>(screenPos.y);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
        DrawBox(drawX - 4, drawY - 2, drawX + strWidth + 4, drawY + 16, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawStringToHandle(drawX, drawY, m_name.c_str(), m_frameColor, font13);
    }
}

// ============================================================================
// 水平面（XZ平面）における直方体（AABB）と円（半径 radius）の押し出し衝突判定
// ============================================================================
bool Obstacle3D::ResolveCollision(VECTOR& outPos, float radius) const
{
    float minX = m_pos.x - m_width * 0.5f;
    float maxX = m_pos.x + m_width * 0.5f;
    float minZ = m_pos.z - m_depth * 0.5f;
    float maxZ = m_pos.z + m_depth * 0.5f;

    // 対象座標からAABB上の最近接点を求める
    float closestX = MathHelper::Clamp(outPos.x, minX, maxX);
    float closestZ = MathHelper::Clamp(outPos.z, minZ, maxZ);

    float dx = outPos.x - closestX;
    float dz = outPos.z - closestZ;
    float distSq = dx * dx + dz * dz;

    // 最近接点までの距離が半径未満なら重なっているため押し出す
    if (distSq < radius * radius)
    {
        float dist = std::sqrt(distSq);

        if (dist > 0.0001f)
        {
            float overlap = radius - dist;
            outPos.x += (dx / dist) * overlap;
            outPos.z += (dz / dist) * overlap;
        }
        else
        {
            // 万一中心まで完全に入り込んでいた場合は、最も近い辺の方向へ脱出させる
            float pushLeft  = std::abs(outPos.x - (minX - radius));
            float pushRight = std::abs((maxX + radius) - outPos.x);
            float pushBack  = std::abs(outPos.z - (minZ - radius));
            float pushFront = std::abs((maxZ + radius) - outPos.z);

            float minPush = (std::min)({ pushLeft, pushRight, pushBack, pushFront });

            if (minPush == pushLeft)       outPos.x = minX - radius;
            else if (minPush == pushRight) outPos.x = maxX + radius;
            else if (minPush == pushBack)  outPos.z = minZ - radius;
            else                           outPos.z = maxZ + radius;
        }

        return true;
    }

    return false;
}
