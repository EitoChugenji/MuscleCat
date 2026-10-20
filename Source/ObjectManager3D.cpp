#include "ObjectManager3D.h"
#include "GameObject3D.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "Camera3D.h"
#include "CsvManager.h"
#include "FontManager.h"
#include "Common.h"
#include "DxLib.h"
#include <algorithm>

void ObjectManager3D::Clear()
{
    m_objects.clear();
    m_obstacles.clear();
    m_player.reset();
    m_caughtCount = 0;
    m_caughtNormalCount = 0;
    m_caughtFastCount = 0;
    m_isTimeFrozen = false;
    m_hitStopFrames = 0;
    m_catchPopups.clear();
}

void ObjectManager3D::InitStage(ModelConfig::MapType mapType)
{
    m_stage.Init(mapType);
}

void ObjectManager3D::LoadStage(ModelConfig::MapType mapType)
{
    m_stage.LoadStage(mapType);
}

void ObjectManager3D::SetPlayer(std::shared_ptr<Player3D> player)
{
    m_player = player;
}

void ObjectManager3D::AddObject(std::shared_ptr<GameObject3D> obj)
{
    m_objects.push_back(obj);
}

void ObjectManager3D::AddObstacle(std::shared_ptr<Obstacle3D> obs)
{
    m_obstacles.push_back(obs);
}

void ObjectManager3D::SpawnNormalMouse()
{
    std::vector<VECTOR> pts = CsvManager::GetInstance().GetMousePositionsForMap(m_stage.GetMapType());
    if (pts.empty())
    {
        pts.push_back(VGet(0.0f, 5.0f, 0.0f));
    }

    VECTOR pos = pts[rand() % pts.size()];
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float ox = (attempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
        float oz = (attempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
        VECTOR test = VGet(pos.x + ox, pos.y, pos.z + oz);
        float gy = 0.0f;
        if (m_stage.GetGroundHeight(test, gy))
        {
            test.y = gy;
            pos = test;
            break;
        }
    }

    auto mouse = std::make_shared<NormalMouse3D>(pos, m_player);
    mouse->SetMapType(m_stage.GetMapType());
    AddObject(mouse);
    ApplyMouseSpeedBonus();
}

void ObjectManager3D::SpawnFastMouse()
{
    std::vector<VECTOR> pts = CsvManager::GetInstance().GetMousePositionsForMap(m_stage.GetMapType());
    if (pts.empty())
    {
        pts.push_back(VGet(0.0f, 5.0f, 0.0f));
    }

    VECTOR pos = pts[rand() % pts.size()];
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float ox = (attempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
        float oz = (attempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
        VECTOR test = VGet(pos.x + ox, pos.y, pos.z + oz);
        float gy = 0.0f;
        if (m_stage.GetGroundHeight(test, gy))
        {
            test.y = gy;
            pos = test;
            break;
        }
    }

    auto mouse = std::make_shared<FastMouse3D>(pos, m_player);
    mouse->SetMapType(m_stage.GetMapType());
    AddObject(mouse);
    ApplyMouseSpeedBonus();
}

void ObjectManager3D::Update(Camera3D& camera)
{
    // ポップアップタイマー更新
    for (auto it = m_catchPopups.begin(); it != m_catchPopups.end(); )
    {
        it->timer--;
        if (it->timer <= 0)
        {
            it = m_catchPopups.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // ヒットストップ中（捕獲時などの一瞬の静止演出）
    if (m_hitStopFrames > 0)
    {
        m_hitStopFrames--;
        return;
    }

    // プレイヤー更新（TPSカメラ基準・飛びつき追尾用ネズミ座標収集）
    if (m_player && m_player->IsAlive())
    {
        std::vector<VECTOR> targets;
        targets.reserve(m_objects.size());
        for (const auto& obj : m_objects)
        {
            if (obj && obj->IsAlive())
            {
                ObjectType t = obj->GetType();
                if (t == ObjectType::NormalMouse || t == ObjectType::FastMouse)
                {
                    targets.push_back(obj->GetPos());
                }
            }
        }
        m_player->UpdateWithCamera(camera, targets);
    }

    // ネズミ等オブジェクト更新（時間停止中は更新スキップ）
    if (!m_isTimeFrozen)
    {
        for (auto& obj : m_objects)
        {
            if (obj->IsAlive())
            {
                obj->Update();
            }
        }
    }

    // 障害物・ステージ壁との衝突押し出し
    CheckObstacleAndStageCollisions(camera);

    // ネズミと猫の捕獲当たり判定
    CheckCollisions(camera);

    // 死亡（捕獲された）オブジェクトの削除
    m_objects.erase
    (
        std::remove_if(m_objects.begin(), m_objects.end(),
            [](const std::shared_ptr<GameObject3D>& obj)
            {
                return !obj->IsAlive();
            }),
        m_objects.end()
    );
}

void ObjectManager3D::Draw3D()
{
    // 1. ステージ描画（床・外壁）
    m_stage.Draw3D();

    // 2. 障害物描画
    for (auto& obs : m_obstacles)
    {
        obs->Draw3D();
    }

    // 3. ネズミ描画
    for (auto& obj : m_objects)
    {
        if (obj->IsAlive())
        {
            obj->Draw3D();
        }
    }

    // 4. プレイヤー描画
    if (m_player && m_player->IsAlive())
    {
        m_player->Draw3D();
    }
}

void ObjectManager3D::Draw2D()
{
    // 障害物の頭上UI
    for (auto& obs : m_obstacles)
    {
        obs->Draw2D();
    }

    // ネズミ等の2D UI（スタン頭上アイコン・残り時間表示等）
    for (auto& obj : m_objects)
    {
        if (obj->IsAlive())
        {
            obj->Draw2D();
        }
    }

    // プレイヤーの2D UI（QTEバー、状態通知等）
    if (m_player && m_player->IsAlive())
    {
        m_player->Draw2D();
    }

    // 捕獲ポップアップ（+1 CATCH! 演出）
    int font24 = FontManager::GetInstance().GetFont24();
    for (const auto& popup : m_catchPopups)
    {
        // 3D座標から画面座標(2D)に変換
        VECTOR screenPos = ConvWorldPosToScreenPos(popup.worldPos);
        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f)
        {
            float progress = 1.0f - static_cast<float>(popup.timer) / static_cast<float>(popup.maxTimer);
            int yOffset = static_cast<int>(progress * 40.0f);
            int x = static_cast<int>(screenPos.x);
            int y = static_cast<int>(screenPos.y) - 30 - yOffset;

            int textW = GetDrawStringWidthToHandle(popup.text.c_str(), static_cast<int>(popup.text.length()), font24);
            int drawX = x - textW / 2;

            // ポップな縁取り描画
            DrawStringToHandle(drawX - 2, y, popup.text.c_str(), GetColor(0, 0, 0), font24);
            DrawStringToHandle(drawX + 2, y, popup.text.c_str(), GetColor(0, 0, 0), font24);
            DrawStringToHandle(drawX, y - 2, popup.text.c_str(), GetColor(0, 0, 0), font24);
            DrawStringToHandle(drawX, y + 2, popup.text.c_str(), GetColor(0, 0, 0), font24);
            DrawStringToHandle(drawX, y, popup.text.c_str(), GetColor(255, 230, 40), font24);
        }
    }
}

void ObjectManager3D::CheckCollisions(Camera3D& camera)
{
    if (!m_player || !m_player->IsAlive())
    {
        return;
    }

    VECTOR pPos = m_player->GetPos();
    float pRadius = m_player->GetRadius();
    bool isTackling = m_player->IsTackling();

    for (auto& obj : m_objects)
    {
        if (!obj->IsAlive())
        {
            continue;
        }

        if (obj->IsMouse())
        {
            VECTOR mPos = obj->GetPos();
            float mRadius = obj->GetRadius();

            float distSq = (pPos.x - mPos.x) * (pPos.x - mPos.x) +
                           ((pPos.y - mPos.y) * 0.5f) * ((pPos.y - mPos.y) * 0.5f) +
                           (pPos.z - mPos.z) * (pPos.z - mPos.z);
            float totalR = pRadius + mRadius;

            if (distSq <= totalR * totalR)
            {
                if (isTackling)
                {
                    // タックル時：ネズミは捕まえられないが、スタン状態にする！
                    auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
                    if (mouse)
                    {
                        int stunDuration = m_player->GetMouseStunDuration();
                        mouse->ApplyStun(stunDuration);
                    }
                }
                
                else
                {
                    // 通常接触時：捕獲成功（スタン中のネズミも通常接触で捕獲可能！）
                    bool isFast = (obj->GetType() == ObjectType::FastMouse);
                    if (isFast)
                    {
                        m_caughtFastCount++;
                    }
                    else
                    {
                        m_caughtNormalCount++;
                    }
                    obj->SetAlive(false);
                    m_caughtCount++;
                    ApplyMouseSpeedBonus();

                    // カメラシェイク演出発動
                    camera.TriggerShake(8.0f, 10);

                    // ヒットストップ発動（捕獲瞬間の手応え・重量感）
                    TriggerHitStop(6);

                    // 捕獲ポップアップ追加
                    CatchPopup cp;
                    cp.worldPos = mPos;
                    cp.timer = 45;
                    cp.maxTimer = 45;
                    cp.text = isFast ? "FAST GET! +1" : "GET! +1";
                    m_catchPopups.push_back(cp);
                }
            }
        }
    }
}

void ObjectManager3D::CheckObstacleAndStageCollisions(Camera3D& camera)
{
    // 1. プレイヤーと障害物・壁の衝突解決
    if (m_player && m_player->IsAlive())
    {
        VECTOR pPos = m_player->GetPos();
        float pRadius = m_player->GetBodyRadius();
        bool collided = false;

        // 障害物との押し出し判定
        for (const auto& obs : m_obstacles)
        {
            if (obs->ResolveCollision(pPos, pRadius))
            {
                collided = true;
            }
        }

        // ステージ壁との押し出し判定（マップモデルの壁ポリゴン対応）
        if (m_stage.ResolveWallCollision(pPos, pRadius))
        {
            collided = true;
        }

        // タックル中に壁や家具に当たったら猫がスタンする！
        if (collided && m_player->IsTackling())
        {
            // 猫が1.5秒（90フレーム）気絶スタン
            m_player->ApplyStun(90);
            camera.TriggerShake(12.0f, 15);
        }

        // 床・坂道への接地判定およびマップ外（足場のない空間・崖）への落下・はみ出し防止
        float groundY = 0.0f;
        if (m_stage.GetGroundHeight(pPos, groundY))
        {
            m_player->SetGroundY(groundY);
            if (!m_player->IsPouncing())
            {
                pPos.y = groundY;
            }
        }
        else
        {
            // 床のない空間・マップ外・足場の端から外へ出ようとした場合
            // まず X方向のみ移動を試行（壁・崖沿いスライド）
            VECTOR prevPos = m_player->GetPrevPos();
            float testGroundY = 0.0f;
            VECTOR testPosX = VGet(pPos.x, prevPos.y, prevPos.z);
            VECTOR testPosZ = VGet(prevPos.x, prevPos.y, pPos.z);

            if (m_stage.GetGroundHeight(testPosX, testGroundY))
            {
                pPos.x = testPosX.x;
                pPos.z = prevPos.z;
                pPos.y = testGroundY;
                m_player->SetGroundY(testGroundY);
            }
            else if (m_stage.GetGroundHeight(testPosZ, testGroundY))
            {
                pPos.x = prevPos.x;
                pPos.z = testPosZ.z;
                pPos.y = testGroundY;
                m_player->SetGroundY(testGroundY);
            }
            else
            {
                // X・Zどちらも床外の場合は前フレームの有効位置に戻す
                pPos.x = prevPos.x;
                pPos.z = prevPos.z;
                float prevGy = 0.0f;
                if (m_stage.GetGroundHeight(prevPos, prevGy))
                {
                    pPos.y = prevGy;
                    m_player->SetGroundY(prevGy);
                }
            }
            collided = true;
        }

        m_player->SetPos(pPos);
    }

    // 2. ネズミと障害物・壁の衝突解決
    for (auto& obj : m_objects)
    {
        if (!obj->IsAlive())
        {
            continue;
        }

        auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
        VECTOR mPos = obj->GetPos();
        float mRadius = obj->GetRadius();
        VECTOR prevPos = mouse ? mouse->GetPrevPos() : mPos;
        bool collided = false;

        // 障害物との押し出し判定（家具が配置されている場合）
        for (const auto& obs : m_obstacles)
        {
            if (obs->ResolveCollision(mPos, mRadius))
            {
                collided = true;
            }
        }

        // ステージ壁との押し出し判定
        VECTOR pushNormal = VGet(0.0f, 0.0f, 0.0f);
        if (m_stage.ResolveWallCollision(mPos, mRadius, &pushNormal))
        {
            collided = true;
        }

        // セーフティネット：外周境界ボックスによる制限
        VECTOR preClampPos = mPos;
        if (m_stage.ClampToBounds(mPos, mRadius))
        {
            collided = true;
            if (pushNormal.x == 0.0f && pushNormal.z == 0.0f)
            {
                pushNormal.x = mPos.x - preClampPos.x;
                pushNormal.z = mPos.z - preClampPos.z;
            }
        }

        // 床・坂道接地判定および壁外・崖外への抜け落ち防止
        float groundY = 0.0f;
        if (m_stage.GetGroundHeight(mPos, groundY))
        {
            mPos.y = groundY;
        }
        else
        {
            // 床のない空間（壁の外側・マップ外）に出ようとした場合
            // まず X方向のみ移動を試行（壁沿いスライディング）
            float testGroundY = 0.0f;
            VECTOR testPosX = VGet(mPos.x, prevPos.y, prevPos.z);
            VECTOR testPosZ = VGet(prevPos.x, prevPos.y, mPos.z);

            if (m_stage.GetGroundHeight(testPosX, testGroundY))
            {
                mPos.x = testPosX.x;
                mPos.z = prevPos.z;
                mPos.y = testGroundY;
            }
            else if (m_stage.GetGroundHeight(testPosZ, testGroundY))
            {
                mPos.x = prevPos.x;
                mPos.z = testPosZ.z;
                mPos.y = testGroundY;
            }
            else
            {
                // 両方床外なら前フレーム位置に戻す
                mPos.x = prevPos.x;
                mPos.z = prevPos.z;
                float prevGy = 0.0f;
                if (m_stage.GetGroundHeight(prevPos, prevGy))
                {
                    mPos.y = prevGy;
                }
            }
            collided = true;
            if (pushNormal.x == 0.0f && pushNormal.z == 0.0f && mouse)
            {
                pushNormal.x = -mouse->GetVelocityX();
                pushNormal.z = -mouse->GetVelocityZ();
            }
        }

        // 壁や境界に衝突した場合、ネズミの速度と方向を反射・更新
        if (collided && mouse)
        {
            mouse->OnWallCollision(pushNormal);
        }

        obj->SetPos(mPos);
    }
}

int ObjectManager3D::GetRemainingMouseCount() const
{
    int count = 0;
    for (const auto& obj : m_objects)
    {
        if (obj->IsAlive() && obj->IsMouse())
        {
            count++;
        }
    }
    return count;
}

float ObjectManager3D::GetCurrentMouseSpeed() const
{
    const auto& stat = CsvManager::GetInstance().GetMouseStatus();
    float s = stat.baseSpeed + static_cast<float>(m_caughtCount) * stat.speedGrowthPerUnit;
    return (s > stat.maxSpeed) ? stat.maxSpeed : s;
}

void ObjectManager3D::ApplyMouseSpeedBonus()
{
    for (auto& obj : m_objects)
    {
        if (obj->IsAlive() && obj->IsMouse())
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (mouse)
            {
                float bonus = static_cast<float>(m_caughtCount) * mouse->GetSpeedGrowthPerUnit();
                mouse->SetSpeedBonus(bonus);
            }
        }
    }
}