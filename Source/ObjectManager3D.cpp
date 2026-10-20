#include "ObjectManager3D.h"
#include "GameObject3D.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "Camera3D.h"
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

namespace
{
    // スポーン候補地点（検証済み床面座標）
    const VECTOR SPAWN_POINTS_HOUSE[] = {
        {   0.0f, 5.0f,    0.0f },
        { 180.0f, 5.0f,  180.0f },
        { -180.0f, 5.0f,  180.0f },
        { -180.0f, 5.0f, -180.0f },
        {  550.0f, 5.0f,  450.0f },
        {  780.0f, 5.0f,  450.0f },
        {  550.0f, 5.0f,  200.0f },
        {  600.0f, 5.0f, -100.0f },
        {  750.0f, 5.0f, -300.0f },
        {  650.0f, 5.0f, -700.0f },
        {  320.0f, 5.0f, -720.0f },
        { -100.0f, 5.0f, -480.0f },
        { -200.0f, 5.0f,    0.0f },
    };
    const int NUM_SPAWN_POINTS_HOUSE = sizeof(SPAWN_POINTS_HOUSE) / sizeof(SPAWN_POINTS_HOUSE[0]);

    const VECTOR SPAWN_POINTS_NEWMAP[] = {
        {     0.0f,  20.0f,  -400.0f },
        {   600.0f,  20.0f,   800.0f },
        {  -600.0f,  20.0f,   800.0f },
        {  1200.0f,  20.0f,     0.0f },
        {  1600.0f,  20.0f,   400.0f },
        {  1800.0f,  20.0f,  -400.0f },
        {  1400.0f,  20.0f,  -800.0f },
        {   400.0f,  20.0f, -1400.0f },
        {  -400.0f,  20.0f, -1200.0f },
        { -1200.0f,  20.0f, -1200.0f },
        { -1600.0f,  20.0f,  -400.0f },
        { -1600.0f,  20.0f,   400.0f },
        { -1800.0f,  20.0f,     0.0f },
        {     0.0f,  20.0f,  1000.0f },
    };
    const int NUM_SPAWN_POINTS_NEWMAP = sizeof(SPAWN_POINTS_NEWMAP) / sizeof(SPAWN_POINTS_NEWMAP[0]);
}

void ObjectManager3D::SpawnNormalMouse()
{
    const bool isHouse = (m_stage.GetMapType() == ModelConfig::MapType::House);
    const VECTOR* pts = isHouse ? SPAWN_POINTS_HOUSE : SPAWN_POINTS_NEWMAP;
    int count = isHouse ? NUM_SPAWN_POINTS_HOUSE : NUM_SPAWN_POINTS_NEWMAP;

    VECTOR pos = pts[rand() % count];
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR test = VGet(pos.x + ox, 0.0f, pos.z + oz);
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
}

void ObjectManager3D::SpawnFastMouse()
{
    const bool isHouse = (m_stage.GetMapType() == ModelConfig::MapType::House);
    const VECTOR* pts = isHouse ? SPAWN_POINTS_HOUSE : SPAWN_POINTS_NEWMAP;
    int count = isHouse ? NUM_SPAWN_POINTS_HOUSE : NUM_SPAWN_POINTS_NEWMAP;

    VECTOR pos = pts[rand() % count];
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR test = VGet(pos.x + ox, 0.0f, pos.z + oz);
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
}

void ObjectManager3D::Update(Camera3D& camera)
{
    // 捕獲ポップアップタイマーの更新
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

    // ヒットストップ中ならオブジェクトの物理・移動更新を一時停止（打撃インパクト強調）
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

    // 5. デバッグ当たり判定可視化ワイヤーフレーム
    DrawDebugHitboxes();
}

void ObjectManager3D::DrawDebugHitboxes() const
{
    if (!m_showDebugHitboxes)
    {
        return;
    }

    SetWriteZBuffer3D(FALSE);

    // 1. 障害物・家具 (Obstacle3D) の直方体当たり判定 (AABB: シアン)
    for (const auto& obs : m_obstacles)
    {
        if (!obs) continue;
        VECTOR pos = obs->GetPos();
        float w = obs->GetWidth();
        float h = obs->GetHeight();
        float d = obs->GetDepth();

        float minX = pos.x - w * 0.5f;
        float maxX = pos.x + w * 0.5f;
        float minZ = pos.z - d * 0.5f;
        float maxZ = pos.z + d * 0.5f;
        float minY = pos.y;
        float maxY = pos.y + h;

        unsigned int boxCol = GetColor(50, 220, 255); // シアン (水色)

        // 底面4辺
        DrawLine3D(VGet(minX, minY, minZ), VGet(maxX, minY, minZ), boxCol);
        DrawLine3D(VGet(maxX, minY, minZ), VGet(maxX, minY, maxZ), boxCol);
        DrawLine3D(VGet(maxX, minY, maxZ), VGet(minX, minY, maxZ), boxCol);
        DrawLine3D(VGet(minX, minY, maxZ), VGet(minX, minY, minZ), boxCol);

        // 天面4辺
        DrawLine3D(VGet(minX, maxY, minZ), VGet(maxX, maxY, minZ), boxCol);
        DrawLine3D(VGet(maxX, maxY, minZ), VGet(maxX, maxY, maxZ), boxCol);
        DrawLine3D(VGet(maxX, maxY, maxZ), VGet(minX, maxY, maxZ), boxCol);
        DrawLine3D(VGet(minX, maxY, maxZ), VGet(minX, maxY, minZ), boxCol);

        // 柱4辺
        DrawLine3D(VGet(minX, minY, minZ), VGet(minX, maxY, minZ), boxCol);
        DrawLine3D(VGet(maxX, minY, minZ), VGet(maxX, maxY, minZ), boxCol);
        DrawLine3D(VGet(maxX, minY, maxZ), VGet(maxX, maxY, maxZ), boxCol);
        DrawLine3D(VGet(minX, minY, maxZ), VGet(minX, maxY, maxZ), boxCol);
    }

    // 2. ネズミ (Mouse3D) の球体当たり判定（通常: 黄色、高速: オレンジ）
    for (const auto& obj : m_objects)
    {
        if (!obj || !obj->IsAlive()) continue;
        VECTOR mPos = obj->GetPos();
        float mRadius = obj->GetRadius();
        bool isFast = (obj->GetType() == ObjectType::FastMouse);

        unsigned int mouseCol = isFast ? GetColor(255, 90, 40) : GetColor(255, 230, 40);
        DrawSphere3D(VGet(mPos.x, mPos.y + mRadius, mPos.z), mRadius, 8, mouseCol, mouseCol, FALSE);
    }

    // 3. プレイヤー (Player3D) の球体当たり判定（通常: 緑、タックル/飛びつき中: 強化オレンジ）
    if (m_player && m_player->IsAlive())
    {
        VECTOR pPos = m_player->GetPos();
        float pRadius = m_player->GetRadius();

        unsigned int catCol = (m_player->IsPouncing() || m_player->IsTackling())
            ? GetColor(255, 170, 30)
            : GetColor(80, 255, 80);

        DrawSphere3D(VGet(pPos.x, pPos.y + pRadius, pPos.z), pRadius, 10, catCol, catCol, FALSE);

        // 前方向きベクトル
        float rotY = m_player->GetRotY();
        VECTOR forward = VGet(std::sin(rotY) * 35.0f, 0.0f, std::cos(rotY) * 35.0f);
        DrawLine3D(VGet(pPos.x, pPos.y + pRadius, pPos.z),
                   VGet(pPos.x + forward.x, pPos.y + pRadius, pPos.z + forward.z),
                   catCol);
    }

    // 4. ステージ壁ポリゴンのワイヤーフレーム（猫の周囲550以内: ネオンピンク）
    if (m_player)
    {
        VECTOR catPos = m_player->GetPos();
        const auto& walls = m_stage.GetWallPolygons();
        unsigned int wallCol = GetColor(255, 60, 200);

        for (const auto& wp : walls)
        {
            float dx = wp.v0.x - catPos.x;
            float dz = wp.v0.z - catPos.z;
            if (dx * dx + dz * dz < 550.0f * 550.0f)
            {
                DrawLine3D(wp.v0, wp.v1, wallCol);
                DrawLine3D(wp.v1, wp.v2, wallCol);
                DrawLine3D(wp.v2, wp.v0, wallCol);
            }
        }
    }

    SetWriteZBuffer3D(TRUE);
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

    // 捕獲ポップアップ描画（画面上への跳ね上がり＆フェードアウト演出）
    const auto& fm = FontManager::GetInstance();
    int font24 = fm.GetFont24();
    for (const auto& popup : m_catchPopups)
    {
        VECTOR screenPos = ConvWorldPosToScreenPos(popup.worldPos);
        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f)
        {
            float ratio = static_cast<float>(popup.timer) / static_cast<float>(popup.maxTimer);
            float bounceY = std::sin((1.0f - ratio) * DX_PI_F * 0.5f) * 35.0f;
            int drawX = static_cast<int>(screenPos.x);
            int drawY = static_cast<int>(screenPos.y - bounceY);

            int alpha = static_cast<int>(255 * (ratio < 0.25f ? (ratio / 0.25f) : 1.0f));
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

            int strW = GetDrawStringWidthToHandle(popup.text.c_str(), static_cast<int>(popup.text.length()), font24);
            int textX = drawX - strW / 2;
            int textY = drawY - 12;

            // コミック風太黒縁取り
            for (int ox = -2; ox <= 2; ++ox)
            {
                for (int oy = -2; oy <= 2; ++oy)
                {
                    if (ox != 0 || oy != 0)
                    {
                        DrawStringToHandle(textX + ox, textY + oy, popup.text.c_str(), GetColor(0, 0, 0), font24);
                    }
                }
            }
            // メインカラー（ビビッドイエロー〜ゴールド）
            DrawStringToHandle(textX, textY, popup.text.c_str(), GetColor(255, 230, 40), font24);

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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

            // 坂道や段差での上下（Y軸）の判定アシスト（Yの差を緩和して飛びつき空振りを防ぐ）
            float dx = pPos.x - mPos.x;
            float dy = (pPos.y - mPos.y) * 0.5f;
            float dz = pPos.z - mPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            float totalR = pRadius + mRadius;

            if (distSq <= totalR * totalR)
            {
                if (isTackling)
                {
                    // タックル時：ネズミは捕まえられないが、スタン状態にする！
                    auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
                    if (mouse && !mouse->IsStunned())
                    {
                        int stunDuration = m_player->GetMouseStunDuration();
                        mouse->ApplyStun(stunDuration);

                        // 強烈な打撃感：強シェイク ＆ ヒットストップ
                        camera.TriggerShake(18.0f, 9);
                        TriggerHitStop(6);
                    }
                }
                
                else
                {
                    // 通常接触時：捕獲成功（スタン中のネズミも通常接触で捕獲可能！）
                    bool isFast = (obj->GetType() == ObjectType::FastMouse);
                    obj->SetAlive(false);
                    m_caughtCount++;
                    if (isFast)
                    {
                        m_caughtFastCount++;
                    }
                    else
                    {
                        m_caughtNormalCount++;
                    }
                    ApplyMouseSpeedBonus();

                    // 捕獲の爽快感：カメラシェイク ＆ ヒットストップ
                    camera.TriggerShake(13.0f, 7);
                    TriggerHitStop(5);

                    // 捕獲演出ポップアップ
                    CatchPopup cp;
                    cp.worldPos = VGet(mPos.x, mPos.y + 24.0f, mPos.z);
                    cp.timer = 45;
                    cp.maxTimer = 45;
                    cp.text = isFast ? "FAST CATCH!!" : "CATCH!!";
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
        if (collided && m_player->IsTackling() && !m_player->IsStunned())
        {
            // 猫が1.5秒（90フレーム）気絶スタン
            m_player->ApplyStun(90);

            // 壁激突インパクト：特大シェイク ＆ 長めヒットストップ
            camera.TriggerShake(24.0f, 12);
            TriggerHitStop(8);
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

void ObjectManager3D::ApplyMouseSpeedBonus()
{
    float bonus = static_cast<float>(m_caughtCount) * 0.15f;
    for (auto& obj : m_objects)
    {
        if (obj->IsAlive() && obj->IsMouse())
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (mouse)
            {
                mouse->SetSpeedBonus(bonus);
            }
        }
    }
}