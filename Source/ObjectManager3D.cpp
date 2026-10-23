#include "ObjectManager3D.h"
#include "GameObject3D.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "Camera3D.h"
#include "CsvManager.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "CustomEffectManager.h"
#include "Config.h"
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
    m_comboCount = 0;
    m_comboTimer = 0;
    m_catchFlashTimer = 0;
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
    // コンボ持続タイマー更新
    if (m_comboTimer > 0)
    {
        m_comboTimer--;
        if (m_comboTimer <= 0)
        {
            m_comboCount = 0;
        }
    }

    // 画面インパクトフラッシュ更新
    if (m_catchFlashTimer > 0)
    {
        m_catchFlashTimer--;
    }

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

    // 捕獲瞬間の画面インパクトゴールドフラッシュ
    if (m_catchFlashTimer > 0)
    {
        float flashRatio = static_cast<float>(m_catchFlashTimer) / 16.0f;
        int flashAlpha = static_cast<int>(flashRatio * 90.0f);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, flashAlpha);
        int scrW = Config::SCREEN_WIDTH;
        int scrH = Config::SCREEN_HEIGHT;
        unsigned int fCol = GetColor(255, 220, 60);
        for (int i = 0; i < 12; ++i)
        {
            DrawBox(i, i, scrW - i, scrH - i, fCol, FALSE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 捕獲ポップアップ（バウンス拡大・コミック調カード・キラキラ星演出）
    int font36 = FontManager::GetInstance().GetFont36();
    int font24 = FontManager::GetInstance().GetFont24();
    int font16 = FontManager::GetInstance().GetFont16();

    for (const auto& popup : m_catchPopups)
    {
        // 3D座標から画面座標(2D)に変換
        VECTOR screenPos = ConvWorldPosToScreenPos(popup.worldPos);
        if (screenPos.z >= 0.0f && screenPos.z <= 1.0f)
        {
            float progress = 1.0f - static_cast<float>(popup.timer) / static_cast<float>(popup.maxTimer);

            int yOffset = static_cast<int>(progress * 60.0f);
            int cx = static_cast<int>(screenPos.x);
            int cy = static_cast<int>(screenPos.y) - 45 - yOffset;

            int alpha = 255;
            if (popup.timer < 16)
            {
                alpha = popup.timer * 255 / 16;
            }

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

            int titleFont = (popup.combo >= 2 || popup.isFast) ? font36 : font24;
            int tW = GetDrawStringWidthToHandle(popup.titleText.c_str(), static_cast<int>(popup.titleText.length()), titleFont);
            int sW = GetDrawStringWidthToHandle(popup.subText.c_str(), static_cast<int>(popup.subText.length()), font16);
            int cardW = (std::max)(tW, sW) + 36;
            int cardH = (popup.combo >= 2 || popup.isFast) ? 68 : 56;

            int left   = cx - cardW / 2;
            int right  = cx + cardW / 2;
            int top    = cy - cardH / 2;
            int bottom = cy + cardH / 2;

            // コミック調ポップカード（黒縁・バッジ枠・シャドウ）
            DrawBox(left - 4, top - 4, right + 4, bottom + 4, GetColor(0, 0, 0), TRUE);
            DrawBox(left - 2, top - 2, right + 2, bottom + 2, popup.badgeColor, TRUE);
            DrawBox(left, top, right, bottom, GetColor(20, 24, 38), TRUE);

            // タイトル文字（黒極太縁取り＋ビビッドカラー）
            int titleX = cx - tW / 2;
            int titleY = top + ((popup.combo >= 2 || popup.isFast) ? 6 : 4);
            for (int ox = -2; ox <= 2; ++ox)
            {
                for (int oy = -2; oy <= 2; ++oy)
                {
                    if (ox != 0 || oy != 0)
                    {
                        DrawStringToHandle(titleX + ox, titleY + oy, popup.titleText.c_str(), GetColor(0, 0, 0), titleFont);
                    }
                }
            }
            DrawStringToHandle(titleX, titleY, popup.titleText.c_str(), popup.titleColor, titleFont);

            // サブテキスト
            int subX = cx - sW / 2;
            int subY = titleY + ((popup.combo >= 2 || popup.isFast) ? 38 : 28);
            DrawStringToHandle(subX - 1, subY, popup.subText.c_str(), GetColor(0, 0, 0), font16);
            DrawStringToHandle(subX + 1, subY, popup.subText.c_str(), GetColor(0, 0, 0), font16);
            DrawStringToHandle(subX, subY - 1, popup.subText.c_str(), GetColor(0, 0, 0), font16);
            DrawStringToHandle(subX, subY + 1, popup.subText.c_str(), GetColor(0, 0, 0), font16);
            DrawStringToHandle(subX, subY, popup.subText.c_str(), GetColor(255, 255, 230), font16);

            // カード周囲のダンシング2D星
            float sparkTime = static_cast<float>(popup.timer) * 0.25f;
            int s1x = left - 12 + static_cast<int>(std::sin(sparkTime) * 4.0f);
            int s1y = top + 8 + static_cast<int>(std::cos(sparkTime) * 4.0f);
            DrawCircle(s1x, s1y, 4, popup.titleColor, TRUE);
            DrawCircle(s1x, s1y, 2, GetColor(255, 255, 255), TRUE);

            int s2x = right + 12 + static_cast<int>(std::cos(sparkTime) * 4.0f);
            int s2y = bottom - 10 + static_cast<int>(std::sin(sparkTime) * 4.0f);
            DrawCircle(s2x, s2y, 4, popup.titleColor, TRUE);
            DrawCircle(s2x, s2y, 2, GetColor(255, 255, 255), TRUE);

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // 画面上部：コンボラッシュHUD（2コンボ以上で表示）
    if (m_comboCount >= 2 && m_comboTimer > 0)
    {
        int scrW = Config::SCREEN_WIDTH;
        int cX = scrW / 2;
        int cY = 70;
        float comboRatio = static_cast<float>(m_comboTimer) / 210.0f;

        char comboBuf[64];
        snprintf(comboBuf, sizeof(comboBuf), "🔥 %d COMBO RUSH! 🔥", m_comboCount);
        int strW = GetDrawStringWidthToHandle(comboBuf, static_cast<int>(std::strlen(comboBuf)), font24);

        int bW = strW + 36;
        int bH = 46;
        int bx1 = cX - bW / 2;
        int by1 = cY - bH / 2;
        int bx2 = cX + bW / 2;
        int by2 = cY + bH / 2;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
        DrawBox(bx1 - 3, by1 - 3, bx2 + 3, by2 + 3, GetColor(0, 0, 0), TRUE);
        DrawBox(bx1 - 1, by1 - 1, bx2 + 1, by2 + 1, GetColor(255, 140, 20), TRUE);
        DrawBox(bx1, by1, bx2, by2, GetColor(22, 25, 38), TRUE);

        // コンボ持続ゲージバー
        int barX = bx1 + 10;
        int barY = by2 - 8;
        int barW = bW - 20;
        int barH = 4;
        DrawBox(barX, barY, barX + barW, barY + barH, GetColor(50, 50, 60), TRUE);
        unsigned int gaugeCol = (comboRatio < 0.3f) ? GetColor(255, 60, 60) : GetColor(255, 200, 30);
        DrawBox(barX, barY, barX + static_cast<int>(barW * comboRatio), barY + barH, gaugeCol, TRUE);

        // テキスト
        DrawStringToHandle(cX - strW / 2, by1 + 6, comboBuf, GetColor(255, 235, 50), font24);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            VECTOR mPos = obj->GetPos();
            float mRadius = mouse ? mouse->GetCatchRadius() : obj->GetRadius();

            float distSq = (pPos.x - mPos.x) * (pPos.x - mPos.x) +
                           ((pPos.y - mPos.y) * 0.5f) * ((pPos.y - mPos.y) * 0.5f) +
                           (pPos.z - mPos.z) * (pPos.z - mPos.z);
            float totalR = pRadius + mRadius;

            if (distSq <= totalR * totalR)
            {
                if (isTackling)
                {
                    // タックル時：ネズミは捕まえられないが、スタン状態にする！
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

                    // コンボ判定（約3.5秒以内に次のネズミを捕まえればコンボ加算！）
                    if (m_comboTimer > 0)
                    {
                        m_comboCount++;
                    }
                    else
                    {
                        m_comboCount = 1;
                    }
                    m_comboTimer = 210; // 約3.5秒のコンボ猶予
                    m_catchFlashTimer = 16; // 画面端インパクトゴールドフラッシュ

                    // ネズミ捕獲3D祝祭エフェクト（星・ダイヤ噴水、ゴールド衝撃波、サンバースト光線、コミカル昇天ソウル）
                    CustomEffectManager::GetInstance().SpawnMouseCatchCelebration(mPos, isFast, m_comboCount);

                    // ネズミ捕獲Effekseerエフェクト再生
                    EffectManager::GetInstance().PlayCatchEffect(mPos, isFast);

                    // カメラシェイク演出発動（コンボ数・高速ネズミに応じて手応え強化）
                    float shakeAmp = 10.0f + static_cast<float>((std::min)(m_comboCount, 5)) * 2.2f + (isFast ? 3.5f : 0.0f);
                    int shakeFrames = 10 + (std::min)(m_comboCount, 4) * 2;
                    camera.TriggerShake(shakeAmp, shakeFrames);

                    // ヒットストップ発動（捕獲瞬間の手応え・重量感）
                    int hitStop = 6 + (isFast ? 2 : 0) + (m_comboCount >= 3 ? 2 : 0);
                    TriggerHitStop(hitStop);

                    // プレイヤーの祝祭リアクション（オーラフレア＆パンプ時間全快リセット）
                    if (m_player)
                    {
                        m_player->OnCatchMouse(isFast, m_comboCount);
                    }

                    // 豪華な弾けるポップアップUI追加
                    CatchPopup cp;
                    cp.worldPos = mPos;
                    cp.timer = 55;
                    cp.maxTimer = 55;
                    cp.combo = m_comboCount;
                    cp.isFast = isFast;

                    if (m_comboCount >= 5)
                    {
                        cp.titleText = "👑 " + std::to_string(m_comboCount) + " COMBO!! 👑";
                        cp.subText = "GODLIKE CAT!! +1";
                        cp.titleColor = GetColor(255, 220, 50);
                        cp.badgeColor = GetColor(180, 40, 255); // ロイヤルパープル
                    }
                    else if (m_comboCount == 4)
                    {
                        cp.titleText = "💥 4 COMBO!! 💥";
                        cp.subText = "MUSCLE FEVER!! +1";
                        cp.titleColor = GetColor(255, 80, 180);
                        cp.badgeColor = GetColor(255, 30, 80);
                    }
                    else if (m_comboCount == 3)
                    {
                        cp.titleText = "⚡ 3 COMBO!! ⚡";
                        cp.subText = "EXCELLENT!! +1";
                        cp.titleColor = GetColor(255, 240, 60);
                        cp.badgeColor = GetColor(255, 120, 0);
                    }
                    else if (m_comboCount == 2)
                    {
                        cp.titleText = "🔥 2 COMBO!! 🔥";
                        cp.subText = "NICE CATCH! +1";
                        cp.titleColor = GetColor(255, 150, 40);
                        cp.badgeColor = GetColor(220, 80, 10);
                    }
                    else
                    {
                        if (isFast)
                        {
                            cp.titleText = "⚡ FAST CATCH!! ⚡";
                            cp.subText = "SPEED GET! +1";
                            cp.titleColor = GetColor(70, 235, 255);
                            cp.badgeColor = GetColor(20, 120, 240);
                        }
                        else
                        {
                            cp.titleText = "★ CATCH!! ★";
                            cp.subText = "+1 POINT!";
                            cp.titleColor = GetColor(255, 230, 40);
                            cp.badgeColor = GetColor(255, 120, 20);
                        }
                    }
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