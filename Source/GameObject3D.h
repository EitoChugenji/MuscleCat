#pragma once
#include "Common.h"
#include "DxLib.h"

// 基底クラス：GameObject3D
// 3D空間内の全オブジェクト共通の抽象基底クラス
class GameObject3D
{
protected:
    VECTOR     m_pos;       // 3D座標 (X, Y, Z)
    float      m_rotY;      // Y軸回転（向き、ラジアン）
    float      m_radius;    // 当たり判定の球・円柱半径
    bool       m_isAlive;   // 生存フラグ
    ObjectType m_type;      // 種別

public:
    GameObject3D(const VECTOR& pos, float radius, ObjectType type)
        : m_pos(pos)
        , m_rotY(0.0f)
        , m_radius(radius)
        , m_isAlive(true)
        , m_type(type)
    {
    }

    virtual ~GameObject3D() = default;

    // 毎フレームの更新処理
    virtual void Update() = 0;

    // 3D空間での描画処理
    virtual void Draw3D() = 0;

    // 2Dスクリーン上でのオーバーレイ描画（UI、頭上マーカー等）
    virtual void Draw2D()
    {
    }

    // ゲッター & セッター
    VECTOR GetPos() const
    {
        return m_pos;
    }

    void SetPos(const VECTOR& pos)
    {
        m_pos = pos;
    }

    float GetRotY() const
    {
        return m_rotY;
    }

    void SetRotY(float rotY)
    {
        m_rotY = rotY;
    }

    virtual float GetRadius() const
    {
        return m_radius;
    }

    void SetRadius(float r)
    {
        m_radius = r;
    }

    bool IsAlive() const
    {
        return m_isAlive;
    }

    void SetAlive(bool alive)
    {
        m_isAlive = alive;
    }

    ObjectType GetType() const
    {
        return m_type;
    }

    bool IsMouse() const
    {
        return m_type == ObjectType::NormalMouse || m_type == ObjectType::FastMouse;
    }
};

