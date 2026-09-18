#pragma once
#include <vector>
#include <memory>
#include "Stage3D.h"

class GameObject3D;
class Player3D;
class Obstacle3D;
class Camera3D;

// ObjectManager3D クラス
// 3Dオブジェクトの一括更新・描画・衝突解決
class ObjectManager3D
{
private:
    std::vector<std::shared_ptr<GameObject3D>> m_objects;
    std::vector<std::shared_ptr<Obstacle3D>>   m_obstacles;
    std::shared_ptr<Player3D>                  m_player;
    Stage3D                                    m_stage;
    int                                        m_caughtCount = 0;
    bool                                       m_isTimeFrozen = false;

public:
    ObjectManager3D() = default;
    ~ObjectManager3D() = default;

    void Clear();
    void InitStage();

    void SetPlayer(std::shared_ptr<Player3D> player);
    
    std::shared_ptr<Player3D> GetPlayer() const
    {
        return m_player;
    }

    void AddObject(std::shared_ptr<GameObject3D> obj);
    void AddObstacle(std::shared_ptr<Obstacle3D> obs);

    // デバッグチート用メソッド
    void SetTimeFrozen(bool frozen)
    {
        m_isTimeFrozen = frozen;
    }
    
    bool IsTimeFrozen() const
    {
        return m_isTimeFrozen;
    }
    
    void ToggleTimeFrozen()
    {
        m_isTimeFrozen = !m_isTimeFrozen;
    }
    
    void SpawnNormalMouse();
    void SpawnFastMouse();

    void Update(const Camera3D& camera);
    void Draw3D();
    void Draw2D();

    void CheckCollisions();
    void CheckObstacleAndStageCollisions();

    int GetRemainingMouseCount() const;
    
    int GetCaughtCount() const
    {
        return m_caughtCount;
    }
    
    float GetCurrentMouseSpeed() const
    {
        float s = 2.4f + static_cast<float>(m_caughtCount) * 0.15f;
        return (s > 5.3f) ? 5.3f : s;
    }
    
    void ApplyMouseSpeedBonus();
};
