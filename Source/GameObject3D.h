// ============================================================================
// @file   GameObject3D.h
// @brief  3D空間内に存在する全ゲームオブジェクトの抽象基底クラス定義
// ============================================================================
#pragma once
#include "Common.h"
#include "DxLib.h"

// ============================================================================
// GameObject3D クラス（抽象基底クラス）
// プレイヤー（Player3D）、ネズミ（MouseBase3D派生）、障害物（Obstacle3D）の
// 共通インターフェース（座標・回転・当たり判定半径・生死状態・描画）を定義する
// ============================================================================
class GameObject3D
{
protected:
    VECTOR     m_pos;       // 3Dワールド座標 (X, Y, Z)
    float      m_rotY;      // Y軸周りの回転角（キャラクターの向き、単位:ラジアン）
    float      m_radius;    // 水平面（XZ平面）での当たり判定半径
    bool       m_isAlive;   // 生存フラグ（falseになったオブジェクトはObjectManager3Dで回収される）
    ObjectType m_type;      // オブジェクト種別（Player / NormalMouse / FastMouse / Obstacle）

public:
    // ------------------------------------------------------------------------
    // コンストラクタ
    // @param pos    初期ワールド座標
    // @param radius 基本当たり判定半径
    // @param type   オブジェクト種別
    // ------------------------------------------------------------------------
    GameObject3D(const VECTOR& pos, float radius, ObjectType type)
        : m_pos(pos)
        , m_rotY(0.0f)
        , m_radius(radius)
        , m_isAlive(true)
        , m_type(type)
    {
    }

    // 仮想デストラクタ（派生クラスの安全な破棄を保証）
    virtual ~GameObject3D() = default;

    // ------------------------------------------------------------------------
    // 毎フレームの更新処理（純粋仮想関数：各派生クラスで実装必須）
    // ------------------------------------------------------------------------
    virtual void Update() = 0;

    // ------------------------------------------------------------------------
    // 3D空間でのモデル・プリミティブ描画処理（純粋仮想関数）
    // ------------------------------------------------------------------------
    virtual void Draw3D() = 0;

    // ------------------------------------------------------------------------
    // 2Dスクリーン上でのオーバーレイ描画処理（頭上マーカー、QTEゲージ等）
    // 不要なオブジェクトもあるためデフォルトは空実装とする
    // ------------------------------------------------------------------------
    virtual void Draw2D()
    {
    }

    // ------------------------------------------------------------------------
    // 各種ゲッター・セッター
    // ------------------------------------------------------------------------
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

    // ネズミ系オブジェクト（通常ネズミ または 高速ネズミ）かどうかを判定する
    bool IsMouse() const
    {
        return m_type == ObjectType::NormalMouse || m_type == ObjectType::FastMouse;
    }
};
