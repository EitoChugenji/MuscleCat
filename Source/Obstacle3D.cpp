#include "Obstacle3D.h"
#include "ModelManager.h"
#include "FontManager.h"
#include "Common.h"
#include <algorithm>

Obstacle3D::Obstacle3D(
    const VECTOR& pos,
    float width,
    float height,
    float depth,
    const std::string& name,
    const std::string& modelPath,
    unsigned int mainColor,
    unsigned int frameColor
)
    : GameObject3D(pos, (width + depth) * 0.25f, ObjectType::Obstacle)
    , m_width(width)
    , m_height(height)
    , m_depth(depth)
    , m_name(name)
    , m_modelPath(modelPath)
    , m_mainColor(mainColor)
    , m_frameColor(frameColor)
{
}

void Obstacle3D::Draw3D()
{
    // 3Dモデルがあれば描画、無ければフォールバック直方体描画
    if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, m_pos, m_rotY, 1.0f))
    {
        ModelManager::GetInstance().DrawFallbackObstacle(m_pos, m_width, m_height, m_depth, m_mainColor, m_frameColor, m_name);
    }
}

void Obstacle3D::Draw2D()
{
    // 器具の頭上に3D->2D投影で器具名を表示
    VECTOR headPos = VGet(m_pos.x, m_pos.y + m_height + 14.0f, m_pos.z);
    VECTOR screenPos = ConvWorldPosToScreenPos(headPos);

    // 画面内かつ前方にある場合のみ描画
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

bool Obstacle3D::ResolveCollision(VECTOR& outPos, float radius) const
{
    float minX = m_pos.x - m_width * 0.5f;
    float maxX = m_pos.x + m_width * 0.5f;
    float minZ = m_pos.z - m_depth * 0.5f;
    float maxZ = m_pos.z + m_depth * 0.5f;

    // 最近接点クランプ
    float closestX = MathHelper::Clamp(outPos.x, minX, maxX);
    float closestZ = MathHelper::Clamp(outPos.z, minZ, maxZ);

    float dx = outPos.x - closestX;
    float dz = outPos.z - closestZ;
    float distSq = dx * dx + dz * dz;

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
            // 中心に入り込んでしまった場合の脱出押し出し
            float pushLeft  = std::abs(outPos.x - (minX - radius));
            float pushRight = std::abs((maxX + radius) - outPos.x);
            float pushBack  = std::abs(outPos.z - (minZ - radius));
            float pushFront = std::abs((maxZ + radius) - outPos.z);

            float minPush = (std::min)({pushLeft, pushRight, pushBack, pushFront});

            if (minPush == pushLeft)
            {
                outPos.x = minX - radius;
            }
            else if (minPush == pushRight)
            {
                outPos.x = maxX + radius;
            }
            else if (minPush == pushBack)
            {
                outPos.z = minZ - radius;
            }
            else
            {
                outPos.z = maxZ + radius;
            }
        }

        return true;
    }

    return false;
}

