#include "GameManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "ModelConfig.h"
#include "Config.h"
#include "Common.h"
#include "DxLib.h"
#include <cstdlib>

void GameManager::Init()
{
    m_state = GameState::Title;
    m_titleCameraAngle = 0.0f;
    m_frameCount = 0;
    m_fpsTimer = GetNowCount();
    m_prevMouseLeft = true; // 起動直後のクリック暴発防止

    m_objManager.InitStage();
    SetupTitle();
}

void GameManager::SetupRoomObstacles()
{
    // AccessoryModels（キッチン、冷蔵庫、テーブル、テレビ）を部屋に配置
    // 各家具の当たり判定と3Dモデル描画を登録

    // 1. 北東大部屋（キッチン＆ダイニングエリア）
    // システムキッチン（北壁沿い）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(520.0f, 0.0f, 600.0f),
        300.0f, 75.0f, 70.0f,
        "", ModelConfig::KITCHEN_MODEL_PATH,
        GetColor(200, 200, 210), GetColor(150, 150, 160),
        ModelConfig::KITCHEN_MODEL_SCALE,
        0.0f,
        VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y, 0.0f)
    ));

    // 冷蔵庫（キッチンの隣・北壁沿い）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(740.0f, 0.0f, 600.0f),
        80.0f, 160.0f, 64.0f,
        "", ModelConfig::REFRIGERATOR_MODEL_PATH,
        GetColor(220, 220, 230), GetColor(160, 160, 170),
        ModelConfig::REFRIGERATOR_MODEL_SCALE,
        0.0f,
        VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y, 0.0f)
    ));

    // ダイニングテーブル（北東部屋中央）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(600.0f, 0.0f, 360.0f),
        150.0f, 50.0f, 150.0f,
        "", ModelConfig::TABLE_MODEL_PATH,
        GetColor(180, 140, 100), GetColor(140, 100, 70),
        ModelConfig::TABLE_MODEL_SCALE,
        0.0f,
        VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y, 0.0f)
    ));

    // 2. メインリビング（中央エリア）
    // リビングのテレビ（西壁沿い・東向き）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(-270.0f, 0.0f, 50.0f),
        30.0f, 60.0f, 120.0f,
        "", ModelConfig::TV_MODEL_PATH,
        GetColor(40, 40, 45), GetColor(20, 20, 25),
        ModelConfig::TV_MODEL_SCALE,
        DX_PI_F * 0.5f,
        VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
    ));

    // リビングのテーブル（中央北西寄り・猫の初期位置(0,0)から離れた位置）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(-100.0f, 0.0f, 120.0f),
        140.0f, 50.0f, 140.0f,
        "", ModelConfig::TABLE_MODEL_PATH,
        GetColor(180, 140, 100), GetColor(140, 100, 70),
        70.0f,
        0.0f,
        VGet(0.0f, 35.0f, 0.0f)
    ));

    // 3. 南東奥部屋（寝室/個室エリア）
    // 奥部屋のテレビ（南壁沿い・北向き）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(600.0f, 0.0f, -820.0f),
        120.0f, 60.0f, 30.0f,
        "", ModelConfig::TV_MODEL_PATH,
        GetColor(40, 40, 45), GetColor(20, 20, 25),
        ModelConfig::TV_MODEL_SCALE,
        0.0f,
        VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
    ));

    // 奥部屋のテーブル
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(480.0f, 0.0f, -700.0f),
        120.0f, 45.0f, 120.0f,
        "", ModelConfig::TABLE_MODEL_PATH,
        GetColor(180, 140, 100), GetColor(140, 100, 70),
        60.0f,
        0.0f,
        VGet(0.0f, 30.0f, 0.0f)
    ));

    // 4. 東部屋
    // 東部屋のテーブル
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
        VGet(700.0f, 0.0f, -100.0f),
        140.0f, 50.0f, 140.0f,
        "", ModelConfig::TABLE_MODEL_PATH,
        GetColor(180, 140, 100), GetColor(140, 100, 70),
        70.0f,
        0.0f,
        VGet(0.0f, 35.0f, 0.0f)
    ));
}

void GameManager::SetupTitle()
{
    m_state = GameState::Title;
    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();

    // タイトル画面：マッチョな猫を生成（Rep=6の堂々たる姿）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    player->AddRep(6); // Lv.6のマッチョ体格
    m_objManager.SetPlayer(player);

    m_titleCameraAngle = -0.7f;
    m_showHowToPlay = false;
    m_startTransitionTimer = 0;
    m_titleAnimTimer = 0;
    m_selectedMenuItem = 0;

    SetupRoomObstacles();
}

void GameManager::StartGame()
{
    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();

    // プレイヤー生成（原点）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    m_objManager.SetPlayer(player);

    // カメラ初期化（猫の背後から開始）
    m_camera.Init(player->GetPos(), player->GetRotY());

    // 部屋の家具配置
    SetupRoomObstacles();

    // ネズミの生成（建物内の様々な部屋・通路に分散して初期配置）
    const VECTOR spawnRooms[] = {
        VGet(  0.0f, 0.0f,  150.0f),  // リビング北
        VGet(600.0f, 0.0f,  400.0f),  // 北東大部屋
        VGet(600.0f, 0.0f, -150.0f),  // 東中央部屋
        VGet(600.0f, 0.0f, -700.0f),  // 南東奥部屋
        VGet( 50.0f, 0.0f, -700.0f),  // 南西奥部屋
        VGet(-150.0f, 0.0f, -200.0f), // リビング南西
    };
    int roomCount = sizeof(spawnRooms) / sizeof(spawnRooms[0]);

    for (int i = 0; i < Config::NORMAL_MOUSE_COUNT; ++i)
    {
        int rIdx = i % roomCount;
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR spawnPos = VGet(spawnRooms[rIdx].x + ox, 0.0f, spawnRooms[rIdx].z + oz);
        m_objManager.AddObject(std::make_shared<NormalMouse3D>(spawnPos, player));
    }

    for (int i = 0; i < Config::FAST_MOUSE_COUNT; ++i)
    {
        int rIdx = (Config::NORMAL_MOUSE_COUNT + i) % roomCount;
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR spawnPos = VGet(spawnRooms[rIdx].x + ox, 0.0f, spawnRooms[rIdx].z + oz);
        m_objManager.AddObject(std::make_shared<FastMouse3D>(spawnPos, player));
    }

    m_startCount = GetNowCount();
    m_clearCount = 0;
    m_clearTimeSeconds = 0.0f;
    m_state = GameState::Playing;
}

void GameManager::Update()
{
    if (m_state == GameState::Title)
    {
        SetMouseDispFlag(TRUE); // タイトル画面ではカーソル表示
        m_titleAnimTimer++;

        // シネマティックカメラ演出：猫を中心に低アングルから優雅に旋回（全身が綺麗に収まる距離感に調整）
        m_titleCameraAngle += 0.0055f;
        float camDist = 240.0f;
        float camHeight = 38.0f + std::sin(m_titleAnimTimer * 0.02f) * 6.0f;
        VECTOR center = VGet(0.0f, 20.0f, 0.0f);
        VECTOR camPos = VGet(
            std::sin(m_titleCameraAngle) * camDist,
            camHeight,
            std::cos(m_titleCameraAngle) * camDist
        );
        SetCameraPositionAndTarget_UpVecY(camPos, center);

        // 猫のアニメーション：定期的にスクワット運動＆黄金オーラ
        auto player = m_objManager.GetPlayer();
        if (player)
        {
            // 2.5秒ごとにスクワットと待機を切り替え
            bool isSquatting = ((m_titleAnimTimer / 140) % 2 == 0);
            player->UpdateTitleAnimation(isSquatting);

            // マッスルオーラを華やかに更新
            EffectManager::GetInstance().UpdateMuscleAura(player->GetPos(), true, player->GetRepCount());
        }

        // キー入力
        bool keyH    = (CheckHitKey(KEY_INPUT_H) != 0);
        bool keyTab  = (CheckHitKey(KEY_INPUT_TAB) != 0);
        bool keyEsc  = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
        bool keyUp   = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
        bool keyDown = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
        bool isEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isSpace = (CheckHitKey(KEY_INPUT_SPACE) != 0);

        bool triggerH    = keyH && !m_prevKeyH;
        bool triggerTab  = keyTab && !m_prevKeyTab;
        bool triggerEsc  = keyEsc && !m_prevKeyEsc;
        bool triggerUp   = keyUp && !m_prevKeyUp;
        bool triggerDown = keyDown && !m_prevKeyDown;

        m_prevKeyH   = keyH;
        m_prevKeyTab = keyTab;
        m_prevKeyEsc = keyEsc;
        m_prevKeyUp  = keyUp;
        m_prevKeyDown = keyDown;

        // マウス入力
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
        bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
        m_prevMouseLeft = isLeftClick;

        // スタート演出（ホワイトトランジション）中
        if (m_startTransitionTimer > 0)
        {
            m_startTransitionTimer--;
            if (m_startTransitionTimer <= 0)
            {
                StartGame();
            }
            return;
        }

        // HOW TO PLAY ダイアログ表示中の処理
        if (m_showHowToPlay)
        {
            int modalW = 840;
            int modalH = 530;
            int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
            int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;
            int closeBtnX = modalX + modalW - 140;
            int closeBtnY = modalY + modalH - 50;
            int closeBtnW = 120;
            int closeBtnH = 38;

            bool clickCloseBtn = (isMouseTrigger &&
                mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                my >= closeBtnY && my <= closeBtnY + closeBtnH);

            if (triggerEsc || triggerH || triggerTab || ((isEnter || isSpace) && !isMouseTrigger) || clickCloseBtn)
            {
                m_showHowToPlay = false;
            }
            return;
        }

        // Hキー または TABキーで遊び方を開く
        if (triggerH || triggerTab)
        {
            m_showHowToPlay = true;
            return;
        }

        // メニュー項目の上下移動
        if (triggerUp)   m_selectedMenuItem = 0;
        if (triggerDown) m_selectedMenuItem = 1;

        // ボタンの当たり判定座標
        int btnW = 380;
        int btnH = 54;
        int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
        int startBtnY = 380;
        int howBtnY   = 448;

        bool hoverStart = (mx >= btnX && mx <= btnX + btnW && my >= startBtnY && my <= startBtnY + btnH);
        bool hoverHow   = (mx >= btnX && mx <= btnX + btnW && my >= howBtnY && my <= howBtnY + btnH);

        if (hoverStart) m_selectedMenuItem = 0;
        if (hoverHow)   m_selectedMenuItem = 1;

        // スタート実行判定
        if ((hoverStart && isMouseTrigger) || ((isEnter || isSpace) && m_selectedMenuItem == 0))
        {
            m_startTransitionTimer = 22; // 閃光トランジション開始
            if (player)
            {
                EffectManager::GetInstance().PlayPumpSuccessEffect(player->GetPos(), 10);
            }
            return;
        }

        // 遊び方を開く判定
        if ((hoverHow && isMouseTrigger) || ((isEnter || isSpace) && m_selectedMenuItem == 1))
        {
            m_showHowToPlay = true;
            return;
        }
    }
    
    else if (m_state == GameState::Playing)
    {
        SetMouseDispFlag(TRUE); // ゲーム中もカーソル表示（マウス移動操作用）

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            bool isFrontView = player->IsSkillChecking();
            const auto& stage = m_objManager.GetStage();
            m_camera.Update(player->GetPos(), player->GetRotY(), true, isFrontView, stage.GetModelHandle(), &stage.GetMinBounds(), &stage.GetMaxBounds());
        }

        m_camera.Apply();

        // デバッグチート入力処理（F1でON/OFF切り替え）
        bool keyF1 = (CheckHitKey(KEY_INPUT_F1) != 0);

        if (keyF1 && !m_prevKeyF1)
        {
            m_cheatEnabled = !m_cheatEnabled;
        }

        m_prevKeyF1 = keyF1;

        if (m_cheatEnabled && player)
        {
            // [1] 通常ネズミを1体追加
            bool key1 = (CheckHitKey(KEY_INPUT_1) != 0);

            if (key1 && !m_prevKey1)
            {
                m_objManager.SpawnNormalMouse();
            }

            m_prevKey1 = key1;

            // [2] 高速ネズミを1体追加
            bool key2 = (CheckHitKey(KEY_INPUT_2) != 0);

            if (key2 && !m_prevKey2)
            {
                m_objManager.SpawnFastMouse();
            }

            m_prevKey2 = key2;

            // [3] レベル（Rep）を+1
            bool key3 = (CheckHitKey(KEY_INPUT_3) != 0);

            if (key3 && !m_prevKey3)
            {
                player->AddRep(1);
            }

            m_prevKey3 = key3;

            // [4] 時間停止ON/OFF
            bool key4 = (CheckHitKey(KEY_INPUT_4) != 0);

            if (key4 && !m_prevKey4)
            {
                m_objManager.ToggleTimeFrozen();
            }

            m_prevKey4 = key4;

            // [5] クールタイム無効ON/OFF
            bool key5 = (CheckHitKey(KEY_INPUT_5) != 0);

            if (key5 && !m_prevKey5)
            {
                player->ToggleNoCooldown();
            }

            m_prevKey5 = key5;

            // [6] エフェクト表示ON/OFF
            bool key6 = (CheckHitKey(KEY_INPUT_6) != 0);

            if (key6 && !m_prevKey6)
            {
                EffectManager::GetInstance().ToggleEffectEnabled();
            }

            m_prevKey6 = key6;
        }

        m_objManager.Update(m_camera);

        // クリア判定
        if (m_objManager.GetRemainingMouseCount() == 0)
        {
            m_state = GameState::GameClear;
            m_clearCount = GetNowCount();
            m_clearTimeSeconds = (m_clearCount - m_startCount) / 1000.0f;
        }
    }
    
    else if (m_state == GameState::GameClear)
    {
        SetMouseDispFlag(TRUE); // クリア画面ではカーソル表示
        auto player = m_objManager.GetPlayer();

        if (player)
        {
            const auto& stage = m_objManager.GetStage();
            m_camera.Update(player->GetPos(), player->GetRotY(), false, false, stage.GetModelHandle(), &stage.GetMinBounds(), &stage.GetMaxBounds());
        }

        m_camera.Apply();

        // リトライまたはタイトル
        if (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_R))
        {
            StartGame();
        }
        
        else if (CheckHitKey(KEY_INPUT_T))
        {
            SetupTitle();
        }
    }

    // 3Dエフェクトの更新
    EffectManager::GetInstance().Update();
}

void GameManager::Draw()
{
    // 1. 3Dシーンの描画 (Zバッファ有効)
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    m_objManager.Draw3D();

    // Effekseer 3Dエフェクト描画
    EffectManager::GetInstance().SyncCamera();
    EffectManager::GetInstance().Draw3D();

    // 2. 2D HUD / UI描画 (Zバッファ無効)
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    if (m_state == GameState::Playing)
    {
        m_objManager.Draw2D();
    }

    const auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font36 = fm.GetFont36();
    int font48 = fm.GetFont48();

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int yellow = GetColor(255, 240, 60);
    unsigned int gray   = GetColor(180, 180, 180);
    unsigned int cyan   = GetColor(100, 220, 255);

    // FPS & 処理時間計測（0.5秒ごとに更新）
    m_frameCount++;
    int now = GetNowCount();
    if (now - m_fpsTimer >= 500)
    {
        m_currentFps = (m_frameCount * 1000.0f) / static_cast<float>(now - m_fpsTimer);
        m_frameCount = 0;
        m_fpsTimer = now;
    }

    DrawFormatStringToHandle(Config::SCREEN_WIDTH - 170, 18, GetColor(140, 240, 140), font13, "FPS: %.1f (%.1fms)", m_currentFps, m_frameProcessTimeMs);

    if (m_state == GameState::Title)
    {
        DrawTitleScreen();
    }
    
    else if (m_state == GameState::Playing)
    {
        // ゲームプレイHUD（バカゲー風ポップ装飾）
        float currentSec = (GetNowCount() - m_startCount) / 1000.0f;

        // 左上ステータス枠（ポップな黒＋黄色の太縁）
        DrawBox(10, 10, 440, 118, GetColor(0, 0, 0), TRUE);
        DrawBox(12, 12, 438, 116, GetColor(255, 220, 40), FALSE);
        DrawBox(14, 14, 436, 114, GetColor(20, 25, 40), TRUE);

        DrawFormatStringToHandle(24, 18, white, font16, "タイム: %.2f 秒", currentSec);
        DrawFormatStringToHandle(24, 40, yellow, font16, "残りネズミ: %d 匹", m_objManager.GetRemainingMouseCount());

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            MuscleState ms = player->GetMuscleState();
            float speed = player->GetCurrentSpeed();
            int reps = player->GetRepCount();
            float mouseSpeed = m_objManager.GetCurrentMouseSpeed();
            int caught = m_objManager.GetCaughtCount();

            if (player->IsStunned())
            {
                DrawFormatStringToHandle(24, 62, GetColor(255, 90, 90), font16, "猫速度: 0.0 [激突気絶中!! 残り%.1fs]", player->GetCatStunRemainingSeconds());
            }
            
            else if (player->IsTackling())
            {
                DrawFormatStringToHandle(24, 62, GetColor(255, 140, 30), font16, "猫速度: %.1f [Lv.%d タックル突進中!!]", speed, reps);
            }
            
            else if (player->IsPouncing())
            {
                DrawFormatStringToHandle(24, 62, GetColor(255, 80, 0), font16, "猫速度: %.1f [Lv.%d 飛びつき突進中!!]", speed, reps);
            }
            
            else if (ms == MuscleState::Soreness)
            {
                DrawFormatStringToHandle(24, 62, GetColor(100, 180, 255), font16, "猫速度: 0.0 [筋肉痛!! 残り%.1fs]", player->GetSorenessRemainingSeconds());
            }
            
            else if (reps >= Player3D::MAX_EFFECTIVE_REP)
            {
                // 能力値上限15到達（表示レベルは無限に上がる）
                DrawFormatStringToHandle(24, 62, GetColor(255, 215, 0), font16, "猫速度: %.1f [Lv.%d (★MAX GOD MUSCLE!★ 残り%.1fs)]", speed, reps, player->GetPumpDecayRemainingSeconds());
            }
            
            else if (reps >= 4)
            {
                DrawFormatStringToHandle(24, 62, GetColor(255, 150, 20), font16, "猫速度: %.1f [Lv.%d (攻撃解放! タックル[E]/飛びつき可 残り%.1fs)]", speed, reps, player->GetPumpDecayRemainingSeconds());
            }
            
            else if (reps > 0)
            {
                DrawFormatStringToHandle(24, 62, GetColor(255, 210, 60), font16, "猫速度: %.1f [Lv.%d (+%.2f / 残り%.1fs)]", speed, reps, reps * 0.15f, player->GetPumpDecayRemainingSeconds());
            }
            
            else
            {
                DrawFormatStringToHandle(24, 62, white, font16, "猫速度: %.1f [Lv.0 (通常)]", speed);
            }

            DrawFormatStringToHandle(24, 85, (caught > 0) ? GetColor(255, 130, 130) : gray, font13, "鼠速度: %.1f (上限5.3 / %d匹捕獲パニック加速中)", mouseSpeed, caught);
        }

        // 画面下の操作ヒント枠（ポップな縁取り）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        DrawBox(10, Config::SCREEN_HEIGHT - 78, Config::SCREEN_WIDTH - 10, Config::SCREEN_HEIGHT - 6, GetColor(15, 20, 30), TRUE);
        DrawBox(10, Config::SCREEN_HEIGHT - 78, Config::SCREEN_WIDTH - 10, Config::SCREEN_HEIGHT - 6, GetColor(255, 210, 50), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        if (player && player->GetRepCount() >= 4 && player->GetMuscleState() != MuscleState::Soreness)
        {
            DrawFormatStringToHandle(20, Config::SCREEN_HEIGHT - 72, GetColor(255, 230, 80), font16, "★ タックル: [E] (Push突進！ネズミ気絶) | 飛びつき: [SHIFT]/[X] (Lv.%d 跳躍突進！)", player->GetRepCount());
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 50, "移動: WASD | カメラ: 矢印キー [←→↑↓] または [Q][R] | 視点リセット: [F]", white, font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 28, "筋トレ: [SPACE] 長押し→タイミングよく離してRep追加！ (15秒放置で0Rep / 失敗で5秒移動不可)", GetColor(255, 220, 100), font13);
        }
        
        else
        {
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 72, "【Lv.4でタックル[E]＆飛びつき解放】 筋トレ: [SPACE] で筋肉をつけよう！", GetColor(255, 200, 80), font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 50, "移動: WASD | カメラ: 矢印キー [←→↑↓] または [Q][R] | 視点リセット: [F]", white, font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 28, "筋トレ: [SPACE] 長押し→タイミングよく離してRep追加！ (15秒放置で0Rep / 失敗で5秒移動不可)", GetColor(255, 220, 100), font13);
        }

        // デバッグチートパネル表示
        auto cheatPlayer = m_objManager.GetPlayer();
        // F1を押したらON（どの状態でも表示を出す）
        if (m_cheatEnabled)
        {
            // 半透明背景
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(Config::SCREEN_WIDTH - 380, 40, Config::SCREEN_WIDTH - 5, 246, GetColor(15, 15, 30), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(Config::SCREEN_WIDTH - 380, 40, Config::SCREEN_WIDTH - 5, 246, GetColor(255, 80, 80), FALSE);

            DrawStringToHandle(Config::SCREEN_WIDTH - 370, 48, "★ DEBUG CHEAT MODE ON ★", GetColor(255, 80, 80), font16);

            bool tf  = m_objManager.IsTimeFrozen();
            bool ncd = cheatPlayer ? cheatPlayer->IsNoCooldown() : false;
            int  rep = cheatPlayer ? cheatPlayer->GetRepCount() : 0;
            bool eff = EffectManager::GetInstance().IsEffectEnabled();

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 76,  GetColor(180, 230, 255), font16,
                "[1] 通常ネズミ追加  [2] 高速ネズミ追加");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 100, GetColor(180, 230, 255), font16,
                "[3] Rep+1  現在Rep: %d %s", rep, (rep >= 15) ? "(能力MAX)" : "");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 124, tf ? GetColor(255, 240, 60) : GetColor(180, 230, 255), font16,
                "[4] 時間停止: %s", tf ? "ON" : "OFF");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 148, ncd ? GetColor(255, 240, 60) : GetColor(180, 230, 255), font16,
                "[5] クールタイム無効: %s", ncd ? "ON (スタンも無効)" : "OFF");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 172, eff ? GetColor(255, 240, 60) : GetColor(180, 230, 255), font16,
                "[6] エフェクト表示: %s", eff ? "ON" : "OFF");

            DrawStringToHandle(Config::SCREEN_WIDTH - 370, 196, "[F1] チートOFF", GetColor(255, 120, 120), font16);
            
            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 220, GetColor(140, 200, 140), font13,
                "ネズミ残り: %d 体", m_objManager.GetRemainingMouseCount());
        }
        
        else
        {
            // チートOFF時は右上に小さく案内だけ表示
            DrawStringToHandle(Config::SCREEN_WIDTH - 220, 42, "[F1] DEBUG CHEAT", GetColor(100, 100, 120), font13);
        }


    }

    else if (m_state == GameState::GameClear)
    {
        // リザルト画面（簡易）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        int resultBoxW = 600;
        int resultBoxH = 380;
        int boxX = (Config::SCREEN_WIDTH - resultBoxW) / 2;
        int boxY = (Config::SCREEN_HEIGHT - resultBoxH) / 2;

        DrawBox(boxX, boxY, boxX + resultBoxW, boxY + resultBoxH, GetColor(30, 35, 50), TRUE);
        DrawBox(boxX, boxY, boxX + resultBoxW, boxY + resultBoxH, yellow, FALSE);

        int clearW = GetDrawStringWidthToHandle("★ STAGE CLEAR!! ★", static_cast<int>(std::string("★ STAGE CLEAR!! ★").length()), font36);
        DrawStringToHandle((Config::SCREEN_WIDTH - clearW) / 2, boxY + 30, "★ STAGE CLEAR!! ★", yellow, font36);

        DrawFormatStringToHandle(boxX + 130, boxY + 110, white, font24, "クリアタイム : %.2f 秒", m_clearTimeSeconds);

        // 評価ランク算出
        const char* rankText = "C";
        unsigned int rankColor = GetColor(180, 180, 180);
        if (m_clearTimeSeconds <= 20.0f)
        {
            rankText = "S (GOD MUSCLE CAT)";
            rankColor = GetColor(255, 215, 0);
        }
        
        else if (m_clearTimeSeconds <= 35.0f)
        {
            rankText = "A (GREAT MUSCLE)";
            rankColor = GetColor(255, 130, 50);
        }
        
        else if (m_clearTimeSeconds <= 50.0f)
        {
            rankText = "B (NICE PUMP)";
            rankColor = GetColor(100, 220, 120);
        }

        DrawFormatStringToHandle(boxX + 130, boxY + 160, rankColor, font24, "ランク       : %s", rankText);

        DrawStringToHandle(boxX + 130, boxY + 240, "[SPACE] または [R] キー: リトライ", GetColor(150, 255, 150), font18);
        DrawStringToHandle(boxX + 130, boxY + 280, "[T] キー: タイトル画面へ戻る", cyan, font18);
    }

    if (CheckHitKey(KEY_INPUT_F12))
    {
        SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "d:\\MuscleCat\\screen_shot.png");
    }
}

// ============================================================================
// タイトル画面の描画（ポップ＆カートゥーン風デザイン・アメコミ演出）
// ============================================================================
void GameManager::DrawTitleScreen()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font60 = fm.GetFont60();

    // 1. ポップなコミック風ヘッダー＆フッターバー
    // 上部バー（ビビッドなインディゴブルー＋イエローの太いボーダーライン）
    DrawBox(0, 0, Config::SCREEN_WIDTH, 48, GetColor(20, 24, 45), TRUE);
    DrawBox(0, 44, Config::SCREEN_WIDTH, 48, GetColor(255, 215, 30), TRUE);
    DrawBox(0, 48, Config::SCREEN_WIDTH, 50, GetColor(0, 0, 0), TRUE);

    // 下部バー（インディゴブルー＋イエローライン＋ブラック枠）
    DrawBox(0, Config::SCREEN_HEIGHT - 50, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 48, GetColor(0, 0, 0), TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 48, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 44, GetColor(255, 215, 30), TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 44, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(20, 24, 45), TRUE);

    // 上部ヘッダー（アメコミ風の元気なタイトルタグライン）
    DrawStringToHandle(25, 14, "🐱💥 THE MOST PUMPED FELINE ADVENTURE! 💥🐱", GetColor(255, 230, 60), font16);

    // 下部操作ガイド（ポップなコミックガイドプレート）
    const char* helpStr = "[ ↑ / ↓ ] SELECT   [ ENTER / SPACE ] PUSH!   [ H / TAB ] HOW TO PLAY";
    int helpW = GetDrawStringWidthToHandle(helpStr, static_cast<int>(strlen(helpStr)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - helpW) / 2, Config::SCREEN_HEIGHT - 32, helpStr, GetColor(255, 255, 255), font16);
    DrawStringToHandle(Config::SCREEN_WIDTH - 100, Config::SCREEN_HEIGHT - 30, "VER 1.0", GetColor(100, 220, 255), font13);

    // 2. カートゥーン・サンバースト（回転するポップな放射状の光条）
    int sunburstCX = Config::SCREEN_WIDTH / 2;
    int sunburstCY = 150;
    float sunAngle = GetNowCount() * 0.0006f;
    int rayCount = 14;
    float rayRadius = 480.0f;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 75);
    for (int r = 0; r < rayCount; ++r)
    {
        float a1 = sunAngle + static_cast<float>(r * 2) * (MathHelper::PI / rayCount);
        float a2 = a1 + (MathHelper::PI / rayCount) * 0.85f;
        int p1x = sunburstCX + static_cast<int>(std::cos(a1) * rayRadius);
        int p1y = sunburstCY + static_cast<int>(std::sin(a1) * rayRadius);
        int p2x = sunburstCX + static_cast<int>(std::cos(a2) * rayRadius);
        int p2y = sunburstCY + static_cast<int>(std::sin(a2) * rayRadius);

        DrawTriangle(sunburstCX, sunburstCY, p1x, p1y, p2x, p2y, GetColor(255, 220, 50), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 3. タイトルロゴ背面の半透明コミックプレート
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
    DrawBox(0, 65, Config::SCREEN_WIDTH, 275, GetColor(15, 18, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 4. 元気で弾む「MUSCLE CAT 3D」カートゥーンロゴ（ボヨヨンバウンス＋極太ブラックアウトライン＋ビビッドカラー）
    // リズミカルなバウンスアニメーション
    float t = GetNowCount() * 0.005f;
    float bounceOffset = std::abs(std::sin(t)) * -12.0f; // ポンポン弾むバウンス
    int logoBaseY = static_cast<int>(88.0f + bounceOffset);

    // 単語ごとの幅を計算
    const char* txtMuscle = "MUSCLE ";
    const char* txtCat    = "CAT ";
    const char* txt3D     = "3D";
    int wMuscle = GetDrawStringWidthToHandle(txtMuscle, static_cast<int>(strlen(txtMuscle)), font60);
    int wCat    = GetDrawStringWidthToHandle(txtCat, static_cast<int>(strlen(txtCat)), font60);
    int w3D     = GetDrawStringWidthToHandle(txt3D, static_cast<int>(strlen(txt3D)), font60);
    int totalLogoW = wMuscle + wCat + w3D;

    int logoStartX = (Config::SCREEN_WIDTH - totalLogoW) / 2;
    int xMuscle = logoStartX;
    int xCat    = logoStartX + wMuscle;
    int x3D     = logoStartX + wMuscle + wCat;

    // A. コミック太ブラックシャドウ（右下オフセット +6, +7）
    for (int dy = 5; dy <= 8; ++dy)
    {
        for (int dx = 5; dx <= 8; ++dx)
        {
            DrawStringToHandle(xMuscle + dx, logoBaseY + dy, txtMuscle, GetColor(0, 0, 0), font60);
            DrawStringToHandle(xCat + dx,    logoBaseY + dy, txtCat,    GetColor(0, 0, 0), font60);
            DrawStringToHandle(x3D + dx,     logoBaseY + dy, txt3D,     GetColor(0, 0, 0), font60);
        }
    }

    // B. 極太ブラックアウトライン（コミック境界線: 半径-4〜+4）
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(xMuscle + dx, logoBaseY + dy, txtMuscle, GetColor(0, 0, 0), font60);
                DrawStringToHandle(xCat + dx,    logoBaseY + dy, txtCat,    GetColor(0, 0, 0), font60);
                DrawStringToHandle(x3D + dx,     logoBaseY + dy, txt3D,     GetColor(0, 0, 0), font60);
            }
        }
    }

    // C. カートゥーン本体カラー
    // MUSCLE: ホットオレンジ〜ビタミンイエロー
    DrawStringToHandle(xMuscle, logoBaseY + 2, txtMuscle, GetColor(255, 120, 20), font60);
    DrawStringToHandle(xMuscle, logoBaseY,     txtMuscle, GetColor(255, 215, 30), font60);
    DrawStringToHandle(xMuscle, logoBaseY - 2, txtMuscle, GetColor(255, 255, 180), font60); // トップハイライト

    // CAT: スカイブルー〜ピュアホワイト
    DrawStringToHandle(xCat, logoBaseY + 2, txtCat, GetColor(40, 160, 240), font60);
    DrawStringToHandle(xCat, logoBaseY,     txtCat, GetColor(120, 230, 255), font60);
    DrawStringToHandle(xCat, logoBaseY - 2, txtCat, GetColor(240, 255, 255), font60);

    // 3D: ビビッドホットピンク〜イエロー
    DrawStringToHandle(x3D, logoBaseY + 2, txt3D, GetColor(240, 40, 120), font60);
    DrawStringToHandle(x3D, logoBaseY,     txt3D, GetColor(255, 80, 180), font60);
    DrawStringToHandle(x3D, logoBaseY - 2, txt3D, GetColor(255, 220, 240), font60);

    // D. ロゴ周囲のポップな星漫符（★ ✦ 💥）
    int star1Y = logoBaseY - 12 + static_cast<int>(std::sin(t * 1.5f) * 6.0f);
    int star2Y = logoBaseY + 16 + static_cast<int>(std::cos(t * 1.5f) * 6.0f);
    DrawStringToHandle(logoStartX - 48, star1Y, "★", GetColor(255, 230, 40), font24);
    DrawStringToHandle(logoStartX + totalLogoW + 20, star2Y, "★", GetColor(255, 100, 200), font24);
    DrawStringToHandle(logoStartX + totalLogoW + 48, star1Y - 6, "✦", GetColor(80, 230, 255), font18);

    // E. サブタイトル（アメコミバナー風）
    const char* subText = "★ MAXIMUM CARTOON PUMP CHASE ★";
    int subW = GetDrawStringWidthToHandle(subText, static_cast<int>(strlen(subText)), font18);
    int subX = (Config::SCREEN_WIDTH - subW) / 2;
    int subY = logoBaseY + 76;

    // サブタイトルの黒縁＋ビビッドイエロー
    DrawStringToHandle(subX + 2, subY + 2, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX - 1, subY,     subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX + 1, subY,     subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY - 1, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY + 1, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY,     subText, GetColor(255, 245, 150), font18);

    // 5. カートゥーン・ステッカー風メニューボタン
    int btnW = 390;
    int btnH = 58;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int startBtnY = 378;
    int howBtnY   = 452;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: GAME START ---
    bool isStartSelected = (m_selectedMenuItem == 0);
    if (isStartSelected)
    {
        // ドロップシャドウ
        DrawBox(btnX + 6, startBtnY + 6, btnX + btnW + 6, startBtnY + btnH + 6, GetColor(0, 0, 0), TRUE);

        // ビタミンイエローの弾力ボディ＋極太ブラック枠
        DrawBox(btnX - 2, startBtnY - 2, btnX + btnW + 2, startBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, startBtnY, btnX + btnW, startBtnY + btnH, GetColor(255, 220 + static_cast<int>(35 * btnPulse), 20), TRUE);
        DrawBox(btnX + 3, startBtnY + 3, btnX + btnW - 3, startBtnY + 12, GetColor(255, 255, 180), TRUE); // 上部ハイライト

        // 元気な肉球／星アイコン
        int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
        DrawStringToHandle(btnX + 22 + bounceX, startBtnY + 14, "🐾", GetColor(20, 20, 20), font24);

        const char* startTxt = "GAME START !!";
        int tW = GetDrawStringWidthToHandle(startTxt, static_cast<int>(strlen(startTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 15, startTxt, GetColor(20, 20, 30), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 14, startTxt, GetColor(0, 0, 0), font24);
    }
    else
    {
        // 通常状態: コミックブルーグレー
        DrawBox(btnX + 4, startBtnY + 4, btnX + btnW + 4, startBtnY + btnH + 4, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, startBtnY - 1, btnX + btnW + 1, startBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, startBtnY, btnX + btnW, startBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, startBtnY + 2, btnX + btnW - 2, startBtnY + 8, GetColor(60, 75, 110), TRUE);

        const char* startTxt = "GAME START";
        int tW = GetDrawStringWidthToHandle(startTxt, static_cast<int>(strlen(startTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 15, startTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: HOW TO PLAY ---
    bool isHowSelected = (m_selectedMenuItem == 1);
    if (isHowSelected)
    {
        DrawBox(btnX + 6, howBtnY + 6, btnX + btnW + 6, howBtnY + btnH + 6, GetColor(0, 0, 0), TRUE);

        // ビビッドスカイブルー＋極太黒枠
        DrawBox(btnX - 2, howBtnY - 2, btnX + btnW + 2, howBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, howBtnY, btnX + btnW, howBtnY + btnH, GetColor(80, 220 + static_cast<int>(35 * btnPulse), 255), TRUE);
        DrawBox(btnX + 3, howBtnY + 3, btnX + btnW - 3, howBtnY + 12, GetColor(210, 250, 255), TRUE);

        int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
        DrawStringToHandle(btnX + 22 + bounceX, howBtnY + 14, "📖", GetColor(20, 20, 20), font24);

        const char* howTxt = "HOW TO PLAY !!";
        int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 15, howTxt, GetColor(20, 20, 30), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 14, howTxt, GetColor(0, 0, 0), font24);
    }
    else
    {
        DrawBox(btnX + 4, howBtnY + 4, btnX + btnW + 4, howBtnY + btnH + 4, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, howBtnY - 1, btnX + btnW + 1, howBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, howBtnY, btnX + btnW, howBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, howBtnY + 2, btnX + btnW - 2, howBtnY + 8, GetColor(60, 75, 110), TRUE);

        const char* howTxt = "HOW TO PLAY (遊び方)";
        int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 15, howTxt, GetColor(210, 230, 255), font24);
    }

    // 6. 遊び方モーダルが開いていれば描画
    if (m_showHowToPlay)
    {
        DrawHowToPlayModal();
    }

    // 7. ゲーム開始ホワイトトランジション
    if (m_startTransitionTimer > 0)
    {
        float alphaRatio = (22.0f - m_startTransitionTimer) / 22.0f;
        int alphaVal = static_cast<int>(alphaRatio * 255.0f);
        if (alphaVal > 255) alphaVal = 255;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaVal);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// ============================================================================
// 遊び方モーダルダイアログの描画（カートゥーン・コミックブック調）
// ============================================================================
void GameManager::DrawHowToPlayModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー（コミック半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 12, 25), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ本体（ステッカー風の極太ブラック枠＋ドロップシャドウ）
    int modalW = 840;
    int modalH = 530;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    // 黒ドロップシャドウ
    DrawBox(modalX + 8, modalY + 8, modalX + modalW + 8, modalY + modalH + 8, GetColor(0, 0, 0), TRUE);

    // 極太ブラック枠
    DrawBox(modalX - 4, modalY - 4, modalX + modalW + 4, modalY + modalH + 4, GetColor(0, 0, 0), TRUE);

    // 本体背景（ダークネイビー・コミックページ）
    DrawBox(modalX, modalY, modalX + modalW, modalY + modalH, GetColor(22, 28, 48), TRUE);

    // ヘッダーバー（ビビッドイエロー＋ブラック枠）
    DrawBox(modalX, modalY, modalX + modalW, modalY + 54, GetColor(255, 215, 30), TRUE);
    DrawBox(modalX, modalY + 52, modalX + modalW, modalY + 56, GetColor(0, 0, 0), TRUE);

    const char* modalHeader = "💥 HOW TO PLAY - 筋肉ネズミ大捕獲作戦!! 💥";
    int hW = GetDrawStringWidthToHandle(modalHeader, static_cast<int>(strlen(modalHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 12, modalHeader, GetColor(20, 20, 30), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2 + 1, modalY + 13, modalHeader, GetColor(0, 0, 0), font24);

    // 3. 3ステップ攻略カード（3分割コミックパネル）
    int cardY = modalY + 68;
    int cardH = 202;
    int cardW = 252;
    int gap = 20;
    int cardStartX = modalX + 22;

    // --- CARD 1: 筋トレ（ビタミンイエロー・コミックパネル） ---
    int c1X = cardStartX;
    DrawBox(c1X + 4, cardY + 4, c1X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE); // シャドウ
    DrawBox(c1X - 2, cardY - 2, c1X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE); // 黒枠
    DrawBox(c1X, cardY, c1X + cardW, cardY + cardH, GetColor(32, 38, 62), TRUE);
    DrawBox(c1X, cardY, c1X + cardW, cardY + 34, GetColor(255, 205, 30), TRUE); // ヘッダー
    DrawBox(c1X, cardY + 32, c1X + cardW, cardY + 35, GetColor(0, 0, 0), TRUE);
    DrawStringToHandle(c1X + 16, cardY + 7, "【STEP 1】 筋トレで巨大化!", GetColor(20, 20, 30), font16);

    DrawStringToHandle(c1X + 12, cardY + 44, "・[SPACE] 長押しでチャージ!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c1X + 12, cardY + 68, "・緑ゾーンで離してRep獲得!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c1X + 12, cardY + 92, "・体がグングン巨大化＆超加速!", GetColor(255, 225, 100), font13);
    DrawStringToHandle(c1X + 12, cardY + 120, "※ 失敗で5秒間筋肉痛で停止!", GetColor(255, 110, 110), font13);
    DrawStringToHandle(c1X + 12, cardY + 144, "※ 15秒間放置で筋肉減衰!", GetColor(255, 170, 110), font13);

    // --- CARD 2: 特殊技（ホットオレンジ・コミックパネル） ---
    int c2X = cardStartX + cardW + gap;
    DrawBox(c2X + 4, cardY + 4, c2X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(c2X - 2, cardY - 2, c2X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(c2X, cardY, c2X + cardW, cardY + cardH, GetColor(32, 38, 62), TRUE);
    DrawBox(c2X, cardY, c2X + cardW, cardY + 34, GetColor(255, 140, 30), TRUE);
    DrawBox(c2X, cardY + 32, c2X + cardW, cardY + 35, GetColor(0, 0, 0), TRUE);
    DrawStringToHandle(c2X + 16, cardY + 7, "【STEP 2】 必殺技で圧倒!!", GetColor(20, 20, 30), font16);

    DrawStringToHandle(c2X + 12, cardY + 44, "★ 4 Rep以上で必殺技が解放!", GetColor(255, 240, 120), font13);
    DrawStringToHandle(c2X + 12, cardY + 70, "・[E] タックル突進:", GetColor(255, 160, 60), font13);
    DrawStringToHandle(c2X + 22, cardY + 92, "猛ダッシュでネズミを気絶!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c2X + 12, cardY + 118, "・[SHIFT] / [X] 飛びつき:", GetColor(255, 100, 100), font13);
    DrawStringToHandle(c2X + 22, cardY + 140, "ホーミング大跳躍で急襲!", GetColor(255, 255, 255), font13);

    // --- CARD 3: 全滅目標（ビビッドシアン・コミックパネル） ---
    int c3X = cardStartX + (cardW + gap) * 2;
    DrawBox(c3X + 4, cardY + 4, c3X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(c3X - 2, cardY - 2, c3X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(c3X, cardY, c3X + cardW, cardY + cardH, GetColor(32, 38, 62), TRUE);
    DrawBox(c3X, cardY, c3X + cardW, cardY + 34, GetColor(40, 215, 255), TRUE);
    DrawBox(c3X, cardY + 32, c3X + cardW, cardY + 35, GetColor(0, 0, 0), TRUE);
    DrawStringToHandle(c3X + 16, cardY + 7, "【STEP 3】 ネズミ全滅クリア!", GetColor(20, 20, 30), font16);

    DrawStringToHandle(c3X + 12, cardY + 44, "・部屋の逃げ回るネズミを全滅!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c3X + 12, cardY + 70, "・全員捕まえればゲームクリア!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c3X + 12, cardY + 98, "・クリア時間でマッスルランク判定:", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c3X + 22, cardY + 120, "Sランク: 20秒以内 (GOD)", GetColor(255, 215, 40), font13);
    DrawStringToHandle(c3X + 22, cardY + 142, "Aランク: 35秒以内 (GREAT)", GetColor(255, 150, 60), font13);

    // 4. キーボード操作一覧表（ポップな黒枠プレート）
    int keyTableY = modalY + 288;
    int keyTableH = 175;
    DrawBox(modalX + 26, keyTableY + 4, modalX + modalW - 18, keyTableY + keyTableH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(modalX + 20, keyTableY - 2, modalX + modalW - 20, keyTableY + keyTableH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(modalX + 22, keyTableY, modalX + modalW - 22, keyTableY + keyTableH, GetColor(28, 34, 54), TRUE);

    DrawBox(modalX + 22, keyTableY, modalX + modalW - 22, keyTableY + 30, GetColor(40, 50, 78), TRUE);
    DrawBox(modalX + 22, keyTableY + 28, modalX + modalW - 22, keyTableY + 31, GetColor(0, 0, 0), TRUE);
    DrawStringToHandle(modalX + 38, keyTableY + 6, "🎮 操作キーバインド一覧", GetColor(255, 225, 40), font16);

    // 2列レイアウト
    int col1X = modalX + 40;
    int col2X = modalX + 430;
    int row1Y = keyTableY + 42;
    int rowStep = 32;

    DrawStringToHandle(col1X, row1Y + rowStep * 0, "・移動操作", GetColor(255, 220, 60), font16);
    DrawStringToHandle(col1X + 130, row1Y + rowStep * 0, ": [W] [A] [S] [D] キー", GetColor(255, 255, 255), font16);

    DrawStringToHandle(col1X, row1Y + rowStep * 1, "・カメラ回転", GetColor(100, 230, 255), font16);
    DrawStringToHandle(col1X + 130, row1Y + rowStep * 1, ": 矢印キー [←][↑][→][↓] / [Q][R]", GetColor(255, 255, 255), font16);

    DrawStringToHandle(col1X, row1Y + rowStep * 2, "・視点リセット", GetColor(100, 230, 255), font16);
    DrawStringToHandle(col1X + 130, row1Y + rowStep * 2, ": [F] キー", GetColor(255, 255, 255), font16);

    DrawStringToHandle(col2X, row1Y + rowStep * 0, "・筋トレ", GetColor(100, 245, 140), font16);
    DrawStringToHandle(col2X + 130, row1Y + rowStep * 0, ": [SPACE] キー (ホールド＆離す)", GetColor(255, 255, 255), font16);

    DrawStringToHandle(col2X, row1Y + rowStep * 1, "・タックル", GetColor(255, 160, 50), font16);
    DrawStringToHandle(col2X + 130, row1Y + rowStep * 1, ": [E] キー (4 Rep以上)", GetColor(255, 255, 255), font16);

    DrawStringToHandle(col2X, row1Y + rowStep * 2, "・飛びつき", GetColor(255, 100, 100), font16);
    DrawStringToHandle(col2X + 130, row1Y + rowStep * 2, ": [SHIFT] または [X] キー (4 Rep以上)", GetColor(255, 255, 255), font16);

    // 5. 閉じるボタン（ポップな赤いコミックボタン）
    int closeBtnW = 120;
    int closeBtnH = 38;
    int closeBtnX = modalX + modalW - 140;
    int closeBtnY = modalY + modalH - 50;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                       my >= closeBtnY && my <= closeBtnY + closeBtnH);

    DrawBox(closeBtnX + 3, closeBtnY + 3, closeBtnX + closeBtnW + 3, closeBtnY + closeBtnH + 3, GetColor(0, 0, 0), TRUE);
    DrawBox(closeBtnX - 2, closeBtnY - 2, closeBtnX + closeBtnW + 2, closeBtnY + closeBtnH + 2, GetColor(0, 0, 0), TRUE);

    if (hoverClose)
    {
        DrawBox(closeBtnX, closeBtnY, closeBtnX + closeBtnW, closeBtnY + closeBtnH, GetColor(255, 80, 80), TRUE);
        DrawBox(closeBtnX + 2, closeBtnY + 2, closeBtnX + closeBtnW - 2, closeBtnY + 8, GetColor(255, 180, 180), TRUE);
        DrawStringToHandle(closeBtnX + 14, closeBtnY + 9, "閉じる (ESC)", GetColor(255, 255, 255), font16);
    }
    else
    {
        DrawBox(closeBtnX, closeBtnY, closeBtnX + closeBtnW, closeBtnY + closeBtnH, GetColor(200, 40, 40), TRUE);
        DrawBox(closeBtnX + 2, closeBtnY + 2, closeBtnX + closeBtnW - 2, closeBtnY + 8, GetColor(240, 100, 100), TRUE);
        DrawStringToHandle(closeBtnX + 14, closeBtnY + 9, "閉じる (ESC)", GetColor(255, 255, 255), font16);
    }
}