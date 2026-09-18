#pragma once
#include "GameObject3D.h"
#include <string>

// 障害物クラス（ジムのトレーニング器具：3D直方体・モデル対応）
class Obstacle3D : public GameObject3D
{
private:
    float m_width;
    float m_height;
    float m_depth;
    std::string m_name;
    std::string m_modelPath;
    unsigned int m_mainColor;
    unsigned int m_frameColor;

public:
    Obstacle3D(
        const VECTOR& pos,
        float width,
        float height,
        float depth,
        const std::string& name,
        const std::string& modelPath,
        unsigned int mainColor,
        unsigned int frameColor
    );
    virtual ~Obstacle3D() = default;

    void Update() override
    {
    } // 静的オブジェクト

    void Draw3D() override;
    void Draw2D() override;

    float GetWidth() const
    {
        return m_width;
    }

    float GetHeight() const
    {
        return m_height;
    }

    float GetDepth() const
    {
        return m_depth;
    }

    const std::string& GetName() const
    {
        return m_name;
    }

    // 円/球オブジェクトとの押し出し判定
    bool ResolveCollision(VECTOR& outPos, float radius) const;
};

