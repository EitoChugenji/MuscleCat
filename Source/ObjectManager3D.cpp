#include "ObjectManager3D.h"
#include "GameObject3D.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "Camera3D.h"
#include "Common.h"
#include "DxLib.h"
#include <algorithm>

void ObjectManager3D::Clear() {
    m_objects.clear();
    m_obstacles.clear();
    m_player.reset();
    m_caughtCount = 0;
    m_isTimeFrozen = false;
}

void ObjectManager3D::InitStage() {
    m_stage.Init();
}

void ObjectManager3D::SetPlayer(std::shared_ptr<Player3D> player) {
    m_player = player;
}

void ObjectManager3D::AddObject(std::shared_ptr<GameObject3D> obj) {
    m_objects.push_back(obj);
}

void ObjectManager3D::AddObstacle(std::shared_ptr<Obstacle3D> obs) {
    m_obstacles.push_back(obs);
}

void ObjectManager3D::SpawnNormalMouse() {
    float x = static_cast<float>(-380 + rand() % 760);
    float z = static_cast<float>(100 + rand() % 220);
    if (rand() % 2 == 0) z = -z;
    AddObject(std::make_shared<NormalMouse3D>(VGet(x, 0.0f, z), m_player));
}

void ObjectManager3D::SpawnFastMouse() {
    float x = static_cast<float>(-380 + rand() % 760);
    float z = static_cast<float>(120 + rand() % 200);
    if (rand() % 2 == 0) z = -z;
    AddObject(std::make_shared<FastMouse3D>(VGet(x, 0.0f, z), m_player));
}

void ObjectManager3D::Update(const Camera3D& camera) {
    // プレイヤー更新（TPSカメラ基準）
    if (m_player && m_player->IsAlive()) {
        m_player->UpdateWithCamera(camera);
    }

    // ネズミ等オブジェクト更新（時間停止中は更新スキップ）
    if (!m_isTimeFrozen) {
        for (auto& obj : m_objects) {
            if (obj->IsAlive()) {
                obj->Update();
            }
        }
    }

    // 障害物・ステージ壁との衝突押し出し
    CheckObstacleAndStageCollisions();

    // ネズミと猫の捕獲当たり判定
    CheckCollisions();

    // 死亡（捕獲された）オブジェクトの削除
    m_objects.erase(
        std::remove_if(m_objects.begin(), m_objects.end(),
            [](const std::shared_ptr<GameObject3D>& obj) {
                return !obj->IsAlive();
            }),
        m_objects.end()
    );
}

void ObjectManager3D::Draw3D() {
    // 1. ステージ描画（床・外壁）
    m_stage.Draw3D();

    // 2. 障害物描画
    for (auto& obs : m_obstacles) {
        obs->Draw3D();
    }

    // 3. ネズミ描画
    for (auto& obj : m_objects) {
        if (obj->IsAlive()) {
            obj->Draw3D();
        }
    }

    // 4. プレイヤー描画
    if (m_player && m_player->IsAlive()) {
        m_player->Draw3D();
    }
}

void ObjectManager3D::Draw2D() {
    // 障害物の頭上UI
    for (auto& obs : m_obstacles) {
        obs->Draw2D();
    }

    // ネズミ等の2D UI（スタン頭上アイコン・残り時間表示等）
    for (auto& obj : m_objects) {
        if (obj->IsAlive()) {
            obj->Draw2D();
        }
    }

    // プレイヤーの2D UI（QTEバー、状態通知等）
    if (m_player && m_player->IsAlive()) {
        m_player->Draw2D();
    }
}

void ObjectManager3D::CheckCollisions() {
    if (!m_player || !m_player->IsAlive()) return;

    VECTOR pPos = m_player->GetPos();
    float pRadius = m_player->GetRadius();
    bool isTackling = m_player->IsTackling();

    for (auto& obj : m_objects) {
        if (!obj->IsAlive()) continue;

        if (obj->IsMouse()) {
            VECTOR mPos = obj->GetPos();
            float mRadius = obj->GetRadius();

            float distSq = (pPos.x - mPos.x) * (pPos.x - mPos.x) +
                           (pPos.y - mPos.y) * (pPos.y - mPos.y) +
                           (pPos.z - mPos.z) * (pPos.z - mPos.z);
            float totalR = pRadius + mRadius;

            if (distSq <= totalR * totalR) {
                if (isTackling) {
                    // ★【タックル時】ネズミは捕まえられないが、スタン状態にする！
                    auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
                    if (mouse) {
                        int stunDuration = m_player->GetMouseStunDuration();
                        mouse->ApplyStun(stunDuration);
                    }
                } else {
                    // 【通常接触時】捕獲成功（スタン中のネズミも通常接触で捕獲可能！）
                    obj->SetAlive(false);
                    m_caughtCount++;
                    ApplyMouseSpeedBonus();
                }
            }
        }
    }
}

void ObjectManager3D::CheckObstacleAndStageCollisions() {
    // 1. プレイヤーと障害物・壁の衝突解決
    if (m_player && m_player->IsAlive()) {
        VECTOR pPos = m_player->GetPos();
        float pRadius = m_player->GetRadius();
        bool collided = false;

        // 障害物との押し出し判定
        for (const auto& obs : m_obstacles) {
            if (obs->ResolveCollision(pPos, pRadius)) {
                collided = true;
            }
        }

        // ステージ壁との押し出し判定
        if (m_stage.ClampToBounds(pPos, pRadius)) {
            collided = true;
        }

        // ★【タックル中に壁や家具に当たったら猫がスタンする！】
        if (collided && m_player->IsTackling()) {
            // 猫が1.5秒（90フレーム）気絶スタン
            m_player->ApplyStun(90);
        }

        m_player->SetPos(pPos);
    }

    // 2. ネズミと障害物・壁の衝突解決
    for (auto& obj : m_objects) {
        if (!obj->IsAlive()) continue;

        VECTOR mPos = obj->GetPos();
        float mRadius = obj->GetRadius();

        for (const auto& obs : m_obstacles) {
            obs->ResolveCollision(mPos, mRadius);
        }

        m_stage.ClampToBounds(mPos, mRadius);
        obj->SetPos(mPos);
    }
}

int ObjectManager3D::GetRemainingMouseCount() const {
    int count = 0;
    for (const auto& obj : m_objects) {
        if (obj->IsAlive() && obj->IsMouse()) {
            count++;
        }
    }
    return count;
}

void ObjectManager3D::ApplyMouseSpeedBonus() {
    float bonus = static_cast<float>(m_caughtCount) * 0.15f;
    for (auto& obj : m_objects) {
        if (obj->IsAlive() && obj->IsMouse()) {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (mouse) {
                mouse->SetSpeedBonus(bonus);
            }
        }
    }
}
