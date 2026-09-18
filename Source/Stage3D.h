#pragma once
#include "DxLib.h"
#include <string>

// 3Dステージ管理クラス
class Stage3D
{
private:
    float m_halfWidth;
    float m_halfDepth;
    float m_wallHeight;
    std::string m_modelPath;

public:
    Stage3D();
    ~Stage3D() = default;

    void Init();
    void Draw3D();

    // プレイヤーやネズミがステージ外壁を越えないよう押し戻す
    bool ClampToBounds(VECTOR& outPos, float radius) const;

    float GetHalfWidth() const
    {
        return m_halfWidth;
    }
    
    float GetHalfDepth() const
    {
        return m_halfDepth;
    }
};
