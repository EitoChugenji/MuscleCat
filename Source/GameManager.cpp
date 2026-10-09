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

    m_objManager.InitStage(m_selectedMap);
    SetupTitle();
}

void GameManager::SetupRoomObstacles()
{
    // AccessoryModels（キッチン、冷蔵庫、テーブル、テレビ）を部屋・ステージに配置
    // 各家具の接地高さ（床・高台・スロープ）を自動取得して配置
    const auto& stage = m_objManager.GetStage();
    auto PlaceObstacle = [&](const VECTOR& basePos, float width, float height, float depth,
                             const std::string& name, const std::string& modelPath,
                             unsigned int mainColor, unsigned int frameColor, float scale,
                             float rotY, const VECTOR& modelOffset) {
        VECTOR pos = basePos;
        float groundY = 0.0f;
        if (stage.GetGroundHeight(pos, groundY))
        {
            pos.y = groundY;
        }
        m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
            pos, width, height, depth, name, modelPath, mainColor, frameColor, scale, rotY, modelOffset
        ));
    };

    if (m_selectedMap == ModelConfig::MapType::House)
    {
        // 1. 北東大部屋（キッチン＆ダイニングエリア）
        // システムキッチン（北壁沿い）
        PlaceObstacle(
            VGet(520.0f, 0.0f, 600.0f),
            300.0f, 75.0f, 70.0f,
            "", ModelConfig::KITCHEN_MODEL_PATH,
            GetColor(200, 200, 210), GetColor(150, 150, 160),
            ModelConfig::KITCHEN_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y, 0.0f)
        );

        // 冷蔵庫（キッチンの隣・北壁沿い）
        PlaceObstacle(
            VGet(740.0f, 0.0f, 600.0f),
            80.0f, 160.0f, 64.0f,
            "", ModelConfig::REFRIGERATOR_MODEL_PATH,
            GetColor(220, 220, 230), GetColor(160, 160, 170),
            ModelConfig::REFRIGERATOR_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y, 0.0f)
        );

        // ダイニングテーブル（北東部屋中央）
        PlaceObstacle(
            VGet(600.0f, 0.0f, 360.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y, 0.0f)
        );

        // 2. メインリビング（中央エリア）
        // リビングのテレビ（西壁沿い・東向き）
        PlaceObstacle(
            VGet(-270.0f, 0.0f, 50.0f),
            30.0f, 60.0f, 120.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE,
            DX_PI_F * 0.5f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
        );

        // リビングのテーブル
        PlaceObstacle(
            VGet(-100.0f, 0.0f, 120.0f),
            140.0f, 50.0f, 140.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            70.0f,
            0.0f,
            VGet(0.0f, 35.0f, 0.0f)
        );

        // 3. 南東奥部屋（寝室/個室エリア）
        // 奥部屋のテレビ（南壁沿い・北向き）
        PlaceObstacle(
            VGet(600.0f, 0.0f, -820.0f),
            120.0f, 60.0f, 30.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
        );

        // 奥部屋のテーブル
        PlaceObstacle(
            VGet(480.0f, 0.0f, -700.0f),
            120.0f, 45.0f, 120.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            60.0f,
            0.0f,
            VGet(0.0f, 30.0f, 0.0f)
        );

        // 4. 東部屋のテーブル
        PlaceObstacle(
            VGet(700.0f, 0.0f, -100.0f),
            140.0f, 50.0f, 140.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            70.0f,
            0.0f,
            VGet(0.0f, 35.0f, 0.0f)
        );
    }
    else
    {
        // SlopeHills (15倍スロープマップ) 用の家具配置（中央平地および各高台）
        PlaceObstacle(
            VGet(600.0f, 0.0f, 800.0f),
            300.0f, 75.0f, 70.0f,
            "", ModelConfig::KITCHEN_MODEL_PATH,
            GetColor(200, 200, 210), GetColor(150, 150, 160),
            ModelConfig::KITCHEN_MODEL_SCALE * 1.5f,
            0.0f,
            VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );

        PlaceObstacle(
            VGet(950.0f, 0.0f, 800.0f),
            80.0f, 160.0f, 64.0f,
            "", ModelConfig::REFRIGERATOR_MODEL_PATH,
            GetColor(220, 220, 230), GetColor(160, 160, 170),
            ModelConfig::REFRIGERATOR_MODEL_SCALE * 1.5f,
            0.0f,
            VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );

        PlaceObstacle(
            VGet(-500.0f, 0.0f, 600.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.5f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );

        PlaceObstacle(
            VGet(-500.0f, 0.0f, 150.0f),
            30.0f, 60.0f, 120.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.5f,
            DX_PI_F * 0.5f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );

        // 高台フロアの家具
        PlaceObstacle(
            VGet(8000.0f, 0.0f, 7000.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.5f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );

        PlaceObstacle(
            VGet(-7500.0f, 0.0f, 8000.0f),
            120.0f, 60.0f, 30.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.5f,
            0.0f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.5f, 0.0f)
        );
    }
}

void GameManager::SwitchSelectedMap(ModelConfig::MapType newMap)
{
    if (m_selectedMap != newMap)
    {
        m_selectedMap = newMap;
        m_objManager.LoadStage(m_selectedMap);

        // タイトル画面中の猫の足元高さを再取得
        auto player = m_objManager.GetPlayer();
        if (player)
        {
            player->SetMapType(m_selectedMap);
            float gy = 0.0f;
            if (m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), gy))
            {
                player->SetPos(VGet(0.0f, gy, 0.0f));
                player->SetGroundY(gy);
            }
        }

        // 家具の配置を更新
        m_objManager.Clear();
        if (player)
        {
            m_objManager.SetPlayer(player);
        }
        SetupRoomObstacles();
    }
}

void GameManager::SetupTitle()
{
    m_state = GameState::Title;
    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();

    // 選択中マップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);

    // タイトル画面：マッチョな猫を生成（Rep=6の堂々たる姿）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    player->AddRep(6); // Lv.6のマッチョ体格
    player->SetMapType(m_selectedMap);

    float groundY = 0.0f;
    if (m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), groundY))
    {
        player->SetPos(VGet(0.0f, groundY, 0.0f));
        player->SetGroundY(groundY);
    }
    m_objManager.SetPlayer(player);

    m_titleCameraAngle = -0.7f;
    m_showHowToPlay = false;
    m_isPaused = false;
    m_pauseStartTime = 0;
    m_pauseMenuItem = 0;
    m_startTransitionTimer = 0;
    m_titleAnimTimer = 0;
    m_selectedMenuItem = 0;

    SetupRoomObstacles();
}

void GameManager::StartGame()
{
    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();
    m_isPaused = false;
    m_pauseStartTime = 0;
    m_pauseMenuItem = 0;

    // 選択されたマップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);

    // プレイヤー生成（原点）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    player->SetMapType(m_selectedMap);
    float playerGy = 0.0f;
    if (m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), playerGy))
    {
        player->SetPos(VGet(0.0f, playerGy, 0.0f));
        player->SetGroundY(playerGy);
    }
    m_objManager.SetPlayer(player);

    // カメラ初期化（猫の背後から開始）
    m_camera.Init(player->GetPos(), player->GetRotY());
    SetMouseDispFlag(FALSE); // ゲームプレイ中はカーソル非表示（マウス視点移動）

    // 部屋・ステージの家具配置
    SetupRoomObstacles();

    // ネズミの生成（マップ別のスポーン位置）
    const VECTOR spawnRoomsHouse[] = {
        VGet(  0.0f, 0.0f,  150.0f),  // リビング北
        VGet(600.0f, 0.0f,  400.0f),  // 北東大部屋
        VGet(600.0f, 0.0f, -150.0f),  // 東中央部屋
        VGet(600.0f, 0.0f, -700.0f),  // 南東奥部屋
        VGet( 50.0f, 0.0f, -700.0f),  // 南西奥部屋
        VGet(-150.0f, 0.0f, -200.0f), // リビング南西
        VGet(-500.0f, 0.0f,  500.0f), // 北西高台
        VGet(-600.0f, 0.0f, -200.0f), // 西高台
    };
    const int houseRoomCount = sizeof(spawnRoomsHouse) / sizeof(spawnRoomsHouse[0]);

    const VECTOR spawnRoomsSlope[] = {
        VGet(   500.0f, 0.0f,   500.0f),  // 中央平地北東
        VGet(  -500.0f, 0.0f,   500.0f),  // 中央平地北西
        VGet(   500.0f, 0.0f,  -500.0f),  // 中央平地南東
        VGet(  -500.0f, 0.0f,  -500.0f),  // 中央平地南西
        VGet(  1750.0f, 0.0f,   1750.0f),  // スロープ入口北東
        VGet( -1750.0f, 0.0f,   1750.0f),  // スロープ入口北西
        VGet(  6000.0f, 0.0f,   6000.0f),  // 北東高台
        VGet( -6000.0f, 0.0f,   6000.0f),  // 北西高台
        VGet(  6000.0f, 0.0f,  -6000.0f),  // 南東高台
        VGet( -6000.0f, 0.0f,  -6000.0f),  // 南西高台
    };
    const int slopeRoomCount = sizeof(spawnRoomsSlope) / sizeof(spawnRoomsSlope[0]);

    const VECTOR* spawnRooms = (m_selectedMap == ModelConfig::MapType::House) ? spawnRoomsHouse : spawnRoomsSlope;
    int roomCount = (m_selectedMap == ModelConfig::MapType::House) ? houseRoomCount : slopeRoomCount;

    const auto& stage = m_objManager.GetStage();
    for (int i = 0; i < Config::NORMAL_MOUSE_COUNT; ++i)
    {
        int rIdx = i % roomCount;
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR spawnPos = VGet(spawnRooms[rIdx].x + ox, 0.0f, spawnRooms[rIdx].z + oz);
        float gy = 0.0f;
        if (stage.GetGroundHeight(spawnPos, gy))
        {
            spawnPos.y = gy;
        }
        auto mouse = std::make_shared<NormalMouse3D>(spawnPos, player);
        mouse->SetMapType(m_selectedMap);
        m_objManager.AddObject(mouse);
    }

    for (int i = 0; i < Config::FAST_MOUSE_COUNT; ++i)
    {
        int rIdx = (Config::NORMAL_MOUSE_COUNT + i) % roomCount;
        float ox = static_cast<float>((rand() % 40) - 20);
        float oz = static_cast<float>((rand() % 40) - 20);
        VECTOR spawnPos = VGet(spawnRooms[rIdx].x + ox, 0.0f, spawnRooms[rIdx].z + oz);
        float gy = 0.0f;
        if (stage.GetGroundHeight(spawnPos, gy))
        {
            spawnPos.y = gy;
        }
        auto mouse = std::make_shared<FastMouse3D>(spawnPos, player);
        mouse->SetMapType(m_selectedMap);
        m_objManager.AddObject(mouse);
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
        auto player = m_objManager.GetPlayer();
        float catY = (player) ? player->GetPos().y : 0.0f;
        float camDist = 240.0f;
        float camHeight = catY + 38.0f + std::sin(m_titleAnimTimer * 0.02f) * 6.0f;
        VECTOR center = VGet(0.0f, catY + 20.0f, 0.0f);
        VECTOR camPos = VGet(
            std::sin(m_titleCameraAngle) * camDist,
            camHeight,
            std::cos(m_titleCameraAngle) * camDist
        );
        SetCameraPositionAndTarget_UpVecY(camPos, center);

        // 猫のアニメーション：定期的にスクワット運動＆黄金オーラ
        if (player)
        {
            // 2.5秒ごとにスクワットと待機を切り替え
            bool isSquatting = ((m_titleAnimTimer / 140) % 2 == 0);
            player->UpdateTitleAnimation(isSquatting);

            // マッスルオーラを華やかに更新
            EffectManager::GetInstance().UpdateMuscleAura(player->GetPos(), true, player->GetRepCount());
        }

        // キー入力
        bool keyH     = (CheckHitKey(KEY_INPUT_H) != 0);
        bool keyTab   = (CheckHitKey(KEY_INPUT_TAB) != 0);
        bool keyEsc   = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
        bool keyUp    = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
        bool keyDown  = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
        bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
        bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
        bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);

        bool triggerH     = keyH && !m_prevKeyH;
        bool triggerTab   = keyTab && !m_prevKeyTab;
        bool triggerEsc   = keyEsc && !m_prevKeyEsc;
        bool triggerUp    = keyUp && !m_prevKeyUp;
        bool triggerDown  = keyDown && !m_prevKeyDown;
        bool triggerLeft  = keyLeft && !m_prevKeyLeft;
        bool triggerRight = keyRight && !m_prevKeyRight;
        bool triggerEnter = isEnter && !m_prevKeyEnter;
        bool triggerSpace = isSpace && !m_prevKeySpace;

        m_prevKeyH     = keyH;
        m_prevKeyTab   = keyTab;
        m_prevKeyEsc   = keyEsc;
        m_prevKeyUp    = keyUp;
        m_prevKeyDown  = keyDown;
        m_prevKeyLeft  = keyLeft;
        m_prevKeyRight = keyRight;
        m_prevKeyEnter = isEnter;
        m_prevKeySpace = isSpace;

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

        // ゲーム説明 ダイアログ表示中の処理（点滅・即閉じ防止にトリガー判定を使用）
        if (m_showHowToPlay)
        {
            int modalW = 860;
            int modalH = 520;
            int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
            int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;
            int closeBtnX = modalX + modalW - 164;
            int closeBtnY = modalY + modalH - 52;
            int closeBtnW = 140;
            int closeBtnH = 38;

            bool clickCloseBtn = (isMouseTrigger &&
                mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                my >= closeBtnY && my <= closeBtnY + closeBtnH);

            if (triggerEsc || triggerH || triggerTab || ((triggerEnter || triggerSpace) && !isMouseTrigger) || clickCloseBtn)
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

        // タイトル画面でESCが押されたらゲーム終了
        if (triggerEsc && !m_showHowToPlay)
        {
            m_shouldQuit = true;
            return;
        }

        // メニュー項目の上下移動 (0: START, 1: STAGE, 2: ゲーム説明)
        if (triggerUp)   m_selectedMenuItem = (m_selectedMenuItem + 2) % 3;
        if (triggerDown) m_selectedMenuItem = (m_selectedMenuItem + 1) % 3;

        // ボタンの当たり判定座標
        int btnW = 420;
        int btnH = 52;
        int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
        int startBtnY = 356;
        int stageBtnY = 420;
        int howBtnY   = 484;

        bool hoverStart = (mx >= btnX && mx <= btnX + btnW && my >= startBtnY && my <= startBtnY + btnH);
        bool hoverStage = (mx >= btnX && mx <= btnX + btnW && my >= stageBtnY && my <= stageBtnY + btnH);
        bool hoverHow   = (mx >= btnX && mx <= btnX + btnW && my >= howBtnY && my <= howBtnY + btnH);

        if (hoverStart) m_selectedMenuItem = 0;
        if (hoverStage) m_selectedMenuItem = 1;
        if (hoverHow)   m_selectedMenuItem = 2;

        // 左右キーによるマップ切り替え
        if (triggerLeft)
        {
            SwitchSelectedMap(ModelConfig::MapType::House);
        }
        if (triggerRight)
        {
            SwitchSelectedMap(ModelConfig::MapType::SlopeHills);
        }

        // STAGEボタンクリックまたはEnter/Spaceによるマップ切り替え
        if (hoverStage && isMouseTrigger)
        {
            if (mx < btnX + 70)
            {
                SwitchSelectedMap(ModelConfig::MapType::House);
            }
            else if (mx > btnX + btnW - 70)
            {
                SwitchSelectedMap(ModelConfig::MapType::SlopeHills);
            }
            else
            {
                SwitchSelectedMap((m_selectedMap == ModelConfig::MapType::House)
                    ? ModelConfig::MapType::SlopeHills
                    : ModelConfig::MapType::House);
            }
        }
        else if ((triggerEnter || triggerSpace) && m_selectedMenuItem == 1)
        {
            SwitchSelectedMap((m_selectedMap == ModelConfig::MapType::House)
                ? ModelConfig::MapType::SlopeHills
                : ModelConfig::MapType::House);
        }

        // スタート実行判定
        if ((hoverStart && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_selectedMenuItem == 0))
        {
            m_startTransitionTimer = 22; // 閃光トランジション開始
            if (player)
            {
                EffectManager::GetInstance().PlayPumpSuccessEffect(player->GetPos(), 10);
            }
            return;
        }

        // ゲーム説明を開く判定
        if ((hoverHow && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_selectedMenuItem == 2))
        {
            m_showHowToPlay = true;
            return;
        }
    }
    
    else if (m_state == GameState::Playing)
    {
        // ESCキーによるポーズ切替判定
        bool keyEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
        bool triggerEsc = keyEsc && !m_prevKeyEsc;
        m_prevKeyEsc = keyEsc;

        // ポーズ中の更新処理
        if (m_isPaused)
        {
            SetMouseDispFlag(TRUE); // ポーズ中はカーソル表示
            UpdatePause(triggerEsc);
            return;
        }

        // プレイ中にESCが押されたらポーズ開始
        if (triggerEsc)
        {
            m_isPaused = true;
            m_pauseStartTime = GetNowCount();
            m_pauseMenuItem = 0;
            SetMouseDispFlag(TRUE);
            return;
        }

        SetMouseDispFlag(FALSE); // ゲームプレイ中はカーソル非表示（マウス視点移動）

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

    // 3Dエフェクトの更新（ポーズ中は更新停止）
    if (!m_isPaused)
    {
        EffectManager::GetInstance().Update();
    }
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
        float currentSec = (m_isPaused ? (m_pauseStartTime - m_startCount) : (GetNowCount() - m_startCount)) / 1000.0f;

        // =========================================================================
        // 1. 左上：プレイヤーステータス ＆ 特大レベルバッジ（一目で現在Lvがわかる）
        // =========================================================================
        int pX = 14;
        int pY = 14;
        int pW = 340;
        int pH = 84;

        // 白いステージ床でもクッキリ浮き立つ黒半透明プレート＆オレンジ太縁
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(pX - 4, pY - 4, pX + pW + 4, pY + pH + 4, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(pX - 2, pY - 2, pX + pW + 2, pY + pH + 2, GetColor(255, 110, 30), FALSE);
        DrawBox(pX, pY, pX + pW, pY + pH, GetColor(20, 24, 38), TRUE);

        auto player = m_objManager.GetPlayer();
        int reps = player ? player->GetRepCount() : 0;
        MuscleState ms = player ? player->GetMuscleState() : MuscleState::Normal;
        bool isGod = (reps >= Player3D::MAX_EFFECTIVE_REP);

        // --- 左側：レベルバッジ（Lv.数字を特大表示） ---
        int badgeX = pX + 8;
        int badgeY = pY + 8;
        int badgeW = 96;
        int badgeH = 68;

        unsigned int badgeBg = isGod ? GetColor(60, 20, 25) : GetColor(32, 40, 62);
        DrawBox(badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, badgeBg, TRUE);
        DrawBox(badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, isGod ? GetColor(255, 60, 40) : GetColor(255, 130, 30), FALSE);

        DrawStringToHandle(badgeX + 14, badgeY + 4, "LEVEL", GetColor(180, 195, 225), font13);

        char lvBuf[16];
        snprintf(lvBuf, sizeof(lvBuf), "Lv.%d", reps);
        unsigned int lvColor = isGod ? GetColor(255, 60, 40) : (reps >= 4 ? GetColor(255, 130, 30) : white);
        DrawStringToHandle(badgeX + 10, badgeY + 18, lvBuf, lvColor, font24);

        // 筋肉減衰ゲージバー（残り15秒の維持時間をバー表示）
        if (player && reps > 0 && ms != MuscleState::Soreness)
        {
            float decaySec = player->GetPumpDecayRemainingSeconds();
            float gaugeRatio = decaySec / 15.0f;
            if (gaugeRatio > 1.0f) gaugeRatio = 1.0f;
            if (gaugeRatio < 0.0f) gaugeRatio = 0.0f;

            int gBarW = badgeW - 14;
            int gBarH = 5;
            int gBarX = badgeX + 7;
            int gBarY = badgeY + badgeH - 12;

            DrawBox(gBarX, gBarY, gBarX + gBarW, gBarY + gBarH, GetColor(10, 10, 15), TRUE);
            unsigned int gColor = (decaySec <= 4.0f && (GetNowCount() / 200) % 2 == 0) ? GetColor(255, 50, 50) : GetColor(255, 130, 30);
            DrawBox(gBarX, gBarY, gBarX + static_cast<int>(gBarW * gaugeRatio), gBarY + gBarH, gColor, TRUE);
        }
        else if (ms == MuscleState::Soreness)
        {
            DrawStringToHandle(badgeX + 12, badgeY + badgeH - 16, "筋肉痛!", GetColor(100, 180, 255), font13);
        }

        // --- 右側：ミッション＆タイム情報 ---
        int infoX = badgeX + badgeW + 14;

        int remainMice = m_objManager.GetRemainingMouseCount();
        DrawStringToHandle(infoX, pY + 8, "ネズミ:", GetColor(200, 215, 240), font16);
        char miceBuf[32];
        snprintf(miceBuf, sizeof(miceBuf), "%d 匹", remainMice);
        DrawStringToHandle(infoX + 60, pY + 6, miceBuf, GetColor(255, 80, 80), font18);

        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "タイム: %.1f 秒", currentSec);
        DrawStringToHandle(infoX, pY + 34, timeBuf, white, font16);

        if (player)
        {
            float catSpeed = player->GetCurrentSpeed();
            char spdBuf[48];
            snprintf(spdBuf, sizeof(spdBuf), "猫速度: %.1f (%s)", catSpeed, (reps >= 4) ? "突進解放済" : "Lv.4で技解放");
            DrawStringToHandle(infoX, pY + 58, spdBuf, (reps >= 4) ? GetColor(100, 245, 140) : GetColor(160, 175, 200), font13);
        }

        // =========================================================================
        // 2. 画面右下：スマート操作ガイドカード（筋トレ長押し明記・見切れ防止）
        // =========================================================================
        int gW = 450;
        int gH = 70;
        int gX = Config::SCREEN_WIDTH - gW - 20;
        int gY = Config::SCREEN_HEIGHT - gH - 20;

        // 半透明プレート（ゲーム画面・床を邪魔しないコンパクトサイズ）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(gX - 3, gY - 3, gX + gW + 3, gY + gH + 3, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(gX - 1, gY - 1, gX + gW + 1, gY + gH + 1, GetColor(80, 100, 140), FALSE);
        DrawBox(gX, gY, gX + gW, gY + gH, GetColor(18, 22, 34), TRUE);

        // 上段：アクションスキル（筋トレが長押しであることを明記）
        DrawStringToHandle(gX + 14, gY + 10, "[SPACE] 筋トレ(長押し)", GetColor(100, 245, 140), font16);

        if (reps >= 4)
        {
            DrawStringToHandle(gX + 195, gY + 10, "[E] タックル", GetColor(255, 150, 40), font16);
            DrawStringToHandle(gX + 315, gY + 10, "[SHIFT] 飛びつき", GetColor(255, 90, 90), font16);
        }
        else
        {
            DrawStringToHandle(gX + 195, gY + 10, "[E] (Lv.4~)", GetColor(120, 130, 150), font16);
            DrawStringToHandle(gX + 315, gY + 10, "[SHIFT] (Lv.4~)", GetColor(120, 130, 150), font16);
        }

        // 下段：基本移動・カメラ
        DrawStringToHandle(gX + 14, gY + 40, "移動: WASD  |  視点: マウス / 矢印  |  視点リセット: [F]", GetColor(210, 225, 245), font13);

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

        // ポーズ画面描画
        if (m_isPaused)
        {
            DrawPauseModal();
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
    DrawStringToHandle(25, 14, "★ THE MOST PUMPED FELINE ADVENTURE! ★", GetColor(255, 230, 60), font16);

    // 下部操作ガイド（ポップなコミックガイドプレート）
    const char* helpStr = "[ ↑ / ↓ ] SELECT   [ ← / → ] CHANGE STAGE   [ ENTER / SPACE ] DECIDE   [ H / TAB ] HOW TO PLAY";
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

    // 5. カートゥーン・ステッカー風メニューボタン（3項目）
    int btnW = 420;
    int btnH = 52;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int startBtnY = 356;
    int stageBtnY = 420;
    int howBtnY   = 484;

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
        DrawStringToHandle(btnX + 22 + bounceX, startBtnY + 12, "🐾", GetColor(20, 20, 20), font24);

        const char* startTxt = "GAME START !!";
        int tW = GetDrawStringWidthToHandle(startTxt, static_cast<int>(strlen(startTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 13, startTxt, GetColor(20, 20, 30), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 12, startTxt, GetColor(0, 0, 0), font24);
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
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, startBtnY + 13, startTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: STAGE SELECT (MAP SWITCH) ---
    bool isStageSelected = (m_selectedMenuItem == 1);
    const char* stageNameStr = (m_selectedMap == ModelConfig::MapType::House)
        ? "STAGE : MAP 1 (HOUSE)"
        : "STAGE : MAP 2 (SLOPE 15x)";

    if (isStageSelected)
    {
        DrawBox(btnX + 6, stageBtnY + 6, btnX + btnW + 6, stageBtnY + btnH + 6, GetColor(0, 0, 0), TRUE);

        // ビビッドオレンジの弾力ボディ＋極太ブラック枠
        DrawBox(btnX - 2, stageBtnY - 2, btnX + btnW + 2, stageBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, stageBtnY, btnX + btnW, stageBtnY + btnH, GetColor(255, 140 + static_cast<int>(35 * btnPulse), 40), TRUE);
        DrawBox(btnX + 3, stageBtnY + 3, btnX + btnW - 3, stageBtnY + 11, GetColor(255, 230, 180), TRUE);

        // 左右の切り替え矢印 [◀] [▶] (アニメーションでパルス)
        int arrowPulse = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 4.0f);
        DrawStringToHandle(btnX + 18 - arrowPulse, stageBtnY + 12, "◀", GetColor(20, 20, 20), font24);
        DrawStringToHandle(btnX + btnW - 38 + arrowPulse, stageBtnY + 12, "▶", GetColor(20, 20, 20), font24);

        int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, stageBtnY + 13, stageNameStr, GetColor(20, 20, 30), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, stageBtnY + 12, stageNameStr, GetColor(0, 0, 0), font24);
    }
    else
    {
        DrawBox(btnX + 4, stageBtnY + 4, btnX + btnW + 4, stageBtnY + btnH + 4, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, stageBtnY - 1, btnX + btnW + 1, stageBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, stageBtnY, btnX + btnW, stageBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, stageBtnY + 2, btnX + btnW - 2, stageBtnY + 8, GetColor(60, 75, 110), TRUE);

        DrawStringToHandle(btnX + 20, stageBtnY + 12, "‹", GetColor(140, 170, 210), font24);
        DrawStringToHandle(btnX + btnW - 32, stageBtnY + 12, "›", GetColor(140, 170, 210), font24);

        int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, stageBtnY + 13, stageNameStr, GetColor(210, 230, 255), font24);
    }

    // --- ボタン3: ゲーム説明 ---
    bool isHowSelected = (m_selectedMenuItem == 2);
    if (isHowSelected)
    {
        DrawBox(btnX + 6, howBtnY + 6, btnX + btnW + 6, howBtnY + btnH + 6, GetColor(0, 0, 0), TRUE);

        // ビビッドスカイブルー＋極太黒枠
        DrawBox(btnX - 2, howBtnY - 2, btnX + btnW + 2, howBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, howBtnY, btnX + btnW, howBtnY + btnH, GetColor(80, 220 + static_cast<int>(35 * btnPulse), 255), TRUE);
        DrawBox(btnX + 3, howBtnY + 3, btnX + btnW - 3, howBtnY + 12, GetColor(210, 250, 255), TRUE);

        int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
        DrawStringToHandle(btnX + 22 + bounceX, howBtnY + 12, "📖", GetColor(20, 20, 20), font24);

        const char* howTxt = "ゲーム説明 !!";
        int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 13, howTxt, GetColor(20, 20, 30), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 12, howTxt, GetColor(0, 0, 0), font24);
    }
    else
    {
        DrawBox(btnX + 4, howBtnY + 4, btnX + btnW + 4, howBtnY + btnH + 4, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, howBtnY - 1, btnX + btnW + 1, howBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, howBtnY, btnX + btnW, howBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, howBtnY + 2, btnX + btnW - 2, howBtnY + 8, GetColor(60, 75, 110), TRUE);

        const char* howTxt = "ゲーム説明";
        int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, howBtnY + 13, howTxt, GetColor(210, 230, 255), font24);
    }

    // ステージ切り替えガイドヒント（半透明コミックプレート付きで視認性抜群）
    const char* stageHint = (m_selectedMap == ModelConfig::MapType::House)
        ? "★ [ ← / → ] またはクリックで MAP 2: 15倍スロープ に切替"
        : "★ [ ← / → ] またはクリックで MAP 1: 通常ハウス に切替";
    int hintW = GetDrawStringWidthToHandle(stageHint, static_cast<int>(strlen(stageHint)), font13);
    int hintX = (Config::SCREEN_WIDTH - hintW) / 2;
    int hintY = 546;

    // 半透明ブラックプレート＋イエロー枠
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
    DrawBox(hintX - 18, hintY - 4, hintX + hintW + 18, hintY + 20, GetColor(15, 20, 36), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(hintX - 18, hintY - 4, hintX + hintW + 18, hintY + 20, GetColor(255, 215, 40), FALSE);

    DrawStringToHandle(hintX, hintY, stageHint, GetColor(255, 235, 120), font13);

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
// ゲーム説明モーダルダイアログの描画（ゲーム画面UIと統一されたシンプル＆洗練デザイン）
// ============================================================================
void GameManager::DrawHowToPlayModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー（ゲーム中HUDと同じシックな半透明ダーク）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 16), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ本体（ゲーム中HUDと同じネイビー基調＆オレンジアクセント）
    int modalW = 860;
    int modalH = 520;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    // ドロップシャドウ＆極太外枠
    DrawBox(modalX + 8, modalY + 8, modalX + modalW + 8, modalY + modalH + 8, GetColor(0, 0, 0), TRUE);
    DrawBox(modalX - 3, modalY - 3, modalX + modalW + 3, modalY + modalH + 3, GetColor(0, 0, 0), TRUE);
    DrawBox(modalX - 1, modalY - 1, modalX + modalW + 1, modalY + modalH + 1, GetColor(255, 110, 30), FALSE);

    // 本体背景（ゲーム画面HUDと同じダークネイビー）
    DrawBox(modalX, modalY, modalX + modalW, modalY + modalH, GetColor(18, 22, 34), TRUE);

    // ヘッダーバー
    DrawBox(modalX, modalY, modalX + modalW, modalY + 52, GetColor(26, 32, 50), TRUE);
    DrawBox(modalX, modalY + 50, modalX + modalW, modalY + 53, GetColor(255, 110, 30), TRUE);

    const char* modalHeader = "💥 ゲーム説明 - 筋肉ネズミ大捕獲作戦!! 💥";
    int hW = GetDrawStringWidthToHandle(modalHeader, static_cast<int>(strlen(modalHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 13, modalHeader, GetColor(255, 230, 80), font24);

    // 3. 上段：3ステップ基本ルールカード（シンプル＆要点のみ）
    int cardY = modalY + 68;
    int cardH = 196;
    int cardW = 260;
    int gap = 16;
    int cardStartX = modalX + 24;

    // --- CARD 1: 筋トレ ---
    int c1X = cardStartX;
    DrawBox(c1X + 4, cardY + 4, c1X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(c1X - 2, cardY - 2, c1X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(c1X, cardY, c1X + cardW, cardY + cardH, GetColor(24, 30, 46), TRUE);
    DrawBox(c1X, cardY, c1X + cardW, cardY + 32, GetColor(35, 55, 70), TRUE);
    DrawBox(c1X, cardY + 30, c1X + cardW, cardY + 32, GetColor(100, 245, 140), TRUE);
    DrawStringToHandle(c1X + 14, cardY + 6, "① 筋トレ (パワーUP)", GetColor(100, 245, 140), font16);

    DrawStringToHandle(c1X + 14, cardY + 46, "・[SPACE] 長押し ➔ 緑で離す", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c1X + 14, cardY + 74, "・成功: 巨大化＆スピードUP!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c1X + 14, cardY + 106, "・失敗: 5秒間 筋肉痛 (停止)", GetColor(255, 110, 110), font13);
    DrawStringToHandle(c1X + 14, cardY + 134, "・放置: 15秒ごとに -3 レベル", GetColor(255, 160, 100), font13);

    // --- CARD 2: 必殺技 ---
    int c2X = cardStartX + cardW + gap;
    DrawBox(c2X + 4, cardY + 4, c2X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(c2X - 2, cardY - 2, c2X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(c2X, cardY, c2X + cardW, cardY + cardH, GetColor(24, 30, 46), TRUE);
    DrawBox(c2X, cardY, c2X + cardW, cardY + 32, GetColor(55, 45, 40), TRUE);
    DrawBox(c2X, cardY + 30, c2X + cardW, cardY + 32, GetColor(255, 150, 40), TRUE);
    DrawStringToHandle(c2X + 14, cardY + 6, "② 必殺技 (Lv.4以上)", GetColor(255, 150, 40), font16);

    DrawStringToHandle(c2X + 14, cardY + 46, "・[E] タックル", GetColor(255, 150, 40), font13);
    DrawStringToHandle(c2X + 26, cardY + 68, "➔ 突進してネズミを気絶!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c2X + 14, cardY + 104, "・[SHIFT] / [X] 飛びつき", GetColor(255, 90, 90), font13);
    DrawStringToHandle(c2X + 26, cardY + 126, "➔ 前方のネズミへ大跳躍!", GetColor(255, 255, 255), font13);

    // --- CARD 3: 目標 ---
    int c3X = cardStartX + (cardW + gap) * 2;
    DrawBox(c3X + 4, cardY + 4, c3X + cardW + 4, cardY + cardH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(c3X - 2, cardY - 2, c3X + cardW + 2, cardY + cardH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(c3X, cardY, c3X + cardW, cardY + cardH, GetColor(24, 30, 46), TRUE);
    DrawBox(c3X, cardY, c3X + cardW, cardY + 32, GetColor(30, 50, 65), TRUE);
    DrawBox(c3X, cardY + 30, c3X + cardW, cardY + 32, GetColor(80, 210, 255), TRUE);
    DrawStringToHandle(c3X + 14, cardY + 6, "③ 目標 (ネズミ全滅)", GetColor(80, 210, 255), font16);

    DrawStringToHandle(c3X + 14, cardY + 46, "・部屋のネズミを全員捕獲!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c3X + 14, cardY + 74, "・気絶したネズミは捕まえやすい!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c3X + 14, cardY + 106, "・数が減るとネズミがスピードUP!", GetColor(255, 150, 150), font13);
    DrawStringToHandle(c3X + 14, cardY + 134, "・早いクリアで高ランク獲得!", GetColor(255, 215, 40), font13);

    // 4. 下段：操作一覧（シンプルに要点のみ配置）
    int keyTableY = modalY + 278;
    int keyTableW = 812;
    int keyTableH = 180;
    int keyTableX = modalX + 24;

    DrawBox(keyTableX + 4, keyTableY + 4, keyTableX + keyTableW + 4, keyTableY + keyTableH + 4, GetColor(0, 0, 0), TRUE);
    DrawBox(keyTableX - 2, keyTableY - 2, keyTableX + keyTableW + 2, keyTableY + keyTableH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(keyTableX, keyTableY, keyTableX + keyTableW, keyTableY + keyTableH, GetColor(20, 25, 38), TRUE);
    DrawBox(keyTableX, keyTableY, keyTableX + keyTableW, keyTableY + 30, GetColor(30, 38, 56), TRUE);
    DrawBox(keyTableX, keyTableY + 28, keyTableX + keyTableW, keyTableY + 30, GetColor(255, 110, 30), TRUE);
    DrawStringToHandle(keyTableX + 16, keyTableY + 6, "🎮 操作方法", GetColor(255, 220, 60), font16);

    // 2列レイアウト
    int col1X = keyTableX + 24;
    int col2X = keyTableX + 430;
    int row1Y = keyTableY + 42;
    int rowStep = 42;

    // 左列: アクション
    DrawStringToHandle(col1X, row1Y + rowStep * 0, "[SPACE] 筋トレ (長押し)", GetColor(100, 245, 140), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 0 + 20, "➔ 長押しして緑ゾーンで離す", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 1, "[E] タックル (Lv.4~)", GetColor(255, 150, 40), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 1 + 20, "➔ 突進してネズミを気絶させる", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 2, "[SHIFT] / [X] 飛びつき (Lv.4~)", GetColor(255, 90, 90), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 2 + 20, "➔ 前方のネズミへ大ジャンプ", GetColor(210, 225, 245), font13);

    // 右列: 移動・カメラ
    DrawStringToHandle(col2X, row1Y + rowStep * 0, "移動: [W] [A] [S] [D]", GetColor(255, 255, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 0 + 20, "➔ キャラクターの前後左右移動", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 1, "カメラ回転: マウス移動 / 矢印 [←→↑↓]", GetColor(100, 220, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 1 + 20, "➔ マウス移動または矢印で自在に視点旋回", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 2, "視点リセット: [F] キー", GetColor(180, 210, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 2 + 20, "➔ カメラを猫の背後に即戻す", GetColor(210, 225, 245), font13);

    // 5. 閉じるボタン
    int closeBtnW = 140;
    int closeBtnH = 38;
    int closeBtnX = modalX + modalW - 164;
    int closeBtnY = modalY + modalH - 52;

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
        DrawStringToHandle(closeBtnX + 20, closeBtnY + 9, "閉じる (ESC)", GetColor(255, 255, 255), font16);
    }
    else
    {
        DrawBox(closeBtnX, closeBtnY, closeBtnX + closeBtnW, closeBtnY + closeBtnH, GetColor(200, 40, 40), TRUE);
        DrawBox(closeBtnX + 2, closeBtnY + 2, closeBtnX + closeBtnW - 2, closeBtnY + 8, GetColor(240, 100, 100), TRUE);
        DrawStringToHandle(closeBtnX + 20, closeBtnY + 9, "閉じる (ESC)", GetColor(255, 255, 255), font16);
    }
}

// ============================================================================
// ポーズ中の更新処理（ESC / リトライ / タイトルに戻る）
// ============================================================================
void GameManager::UpdatePause(bool triggerEsc)
{
    // マウス入力
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
    m_prevMouseLeft = isLeftClick;

    // キー入力
    bool keyUp    = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool keyDown  = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
    bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool keyR     = (CheckHitKey(KEY_INPUT_R) != 0);
    bool keyT     = (CheckHitKey(KEY_INPUT_T) != 0);

    bool triggerUp    = keyUp && !m_prevKeyUp;
    bool triggerDown  = keyDown && !m_prevKeyDown;
    bool triggerEnter = isEnter && !m_prevKeyEnter;
    bool triggerSpace = isSpace && !m_prevKeySpace;
    bool triggerR     = keyR && !m_prevKeyR;
    bool triggerT     = keyT && !m_prevKeyT;

    m_prevKeyUp    = keyUp;
    m_prevKeyDown  = keyDown;
    m_prevKeyEnter = isEnter;
    m_prevKeySpace = isSpace;
    m_prevKeyR     = keyR;
    m_prevKeyT     = keyT;

    // ESCキーでポーズ解除（ゲーム再開）
    if (triggerEsc)
    {
        m_startCount += (GetNowCount() - m_pauseStartTime);
        m_isPaused = false;
        m_camera.ResetMouseToCenter();
        SetMouseDispFlag(FALSE);
        return;
    }

    // 上下キーで選択切り替え (0: リトライ, 1: タイトルに戻る)
    if (triggerUp || triggerDown)
    {
        m_pauseMenuItem = (m_pauseMenuItem + 1) % 2;
    }

    // ボタン当たり判定座標
    int modalW = 460;
    int modalH = 340;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    int btnW = 340;
    int btnH = 54;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int retryBtnY = modalY + 76;
    int titleBtnY = modalY + 148;

    int resumeBtnW = 240;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 230;

    bool hoverRetry  = (mx >= btnX && mx <= btnX + btnW && my >= retryBtnY && my <= retryBtnY + btnH);
    bool hoverTitle  = (mx >= btnX && mx <= btnX + btnW && my >= titleBtnY && my <= titleBtnY + btnH);
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW && my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    if (hoverRetry) m_pauseMenuItem = 0;
    if (hoverTitle) m_pauseMenuItem = 1;

    // 1. リトライ判定（ボタンクリック / 選択中Enter/Space / Rキー）
    if ((hoverRetry && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_pauseMenuItem == 0) || triggerR)
    {
        m_isPaused = false;
        StartGame();
        return;
    }

    // 2. タイトルに戻る判定（ボタンクリック / 選択中Enter/Space / Tキー）
    if ((hoverTitle && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_pauseMenuItem == 1) || triggerT)
    {
        m_isPaused = false;
        SetupTitle();
        return;
    }

    // 3. ゲームに戻る判定（再開ボタンクリック）
    if (hoverResume && isMouseTrigger)
    {
        m_startCount += (GetNowCount() - m_pauseStartTime);
        m_isPaused = false;
        m_camera.ResetMouseToCenter();
        SetMouseDispFlag(FALSE);
        return;
    }
}

// ============================================================================
// ポーズモーダルダイアログ描画
// ============================================================================
void GameManager::DrawPauseModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー（背面のゲーム画面が薄暗く透ける）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ外枠・本体
    int modalW = 460;
    int modalH = 340;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    // ドロップシャドウ
    DrawBox(modalX + 8, modalY + 8, modalX + modalW + 8, modalY + modalH + 8, GetColor(0, 0, 0), TRUE);
    // 極太黒枠
    DrawBox(modalX - 3, modalY - 3, modalX + modalW + 3, modalY + modalH + 3, GetColor(0, 0, 0), TRUE);
    DrawBox(modalX - 1, modalY - 1, modalX + modalW + 1, modalY + modalH + 1, GetColor(255, 120, 30), FALSE);
    // 背景
    DrawBox(modalX, modalY, modalX + modalW, modalY + modalH, GetColor(20, 24, 38), TRUE);

    // ヘッダーバー
    DrawBox(modalX, modalY, modalX + modalW, modalY + 52, GetColor(26, 32, 50), TRUE);
    DrawBox(modalX, modalY + 50, modalX + modalW, modalY + 53, GetColor(255, 120, 30), TRUE);

    const char* pauseHeader = "⏸ PAUSE (一時停止)";
    int hW = GetDrawStringWidthToHandle(pauseHeader, static_cast<int>(strlen(pauseHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 13, pauseHeader, GetColor(255, 220, 60), font24);

    // ボタン配置
    int btnW = 340;
    int btnH = 54;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int retryBtnY = modalY + 76;
    int titleBtnY = modalY + 148;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: リトライ ---
    bool isRetrySelected = (m_pauseMenuItem == 0);
    if (isRetrySelected)
    {
        DrawBox(btnX + 5, retryBtnY + 5, btnX + btnW + 5, retryBtnY + btnH + 5, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 2, retryBtnY - 2, btnX + btnW + 2, retryBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, retryBtnY, btnX + btnW, retryBtnY + btnH, GetColor(100, 230 + static_cast<int>(25 * btnPulse), 120), TRUE);
        DrawBox(btnX + 3, retryBtnY + 3, btnX + btnW - 3, retryBtnY + 11, GetColor(210, 255, 220), TRUE);

        DrawStringToHandle(btnX + 20, retryBtnY + 13, "🔄", GetColor(20, 20, 20), font24);
        const char* retryTxt = "リトライ (R)";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 14, retryTxt, GetColor(15, 25, 20), font24);
    }
    else
    {
        DrawBox(btnX + 3, retryBtnY + 3, btnX + btnW + 3, retryBtnY + btnH + 3, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, retryBtnY - 1, btnX + btnW + 1, retryBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, retryBtnY, btnX + btnW, retryBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, retryBtnY + 2, btnX + btnW - 2, retryBtnY + 8, GetColor(60, 75, 110), TRUE);

        DrawStringToHandle(btnX + 20, retryBtnY + 13, "🔄", GetColor(140, 170, 210), font24);
        const char* retryTxt = "リトライ";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 14, retryTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: タイトルに戻る ---
    bool isTitleSelected = (m_pauseMenuItem == 1);
    if (isTitleSelected)
    {
        DrawBox(btnX + 5, titleBtnY + 5, btnX + btnW + 5, titleBtnY + btnH + 5, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 2, titleBtnY - 2, btnX + btnW + 2, titleBtnY + btnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, titleBtnY, btnX + btnW, titleBtnY + btnH, GetColor(70, 190 + static_cast<int>(35 * btnPulse), 255), TRUE);
        DrawBox(btnX + 3, titleBtnY + 3, btnX + btnW - 3, titleBtnY + 11, GetColor(210, 240, 255), TRUE);

        DrawStringToHandle(btnX + 20, titleBtnY + 13, "🏠", GetColor(20, 20, 20), font24);
        const char* titleTxt = "タイトルに戻る (T)";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 14, titleTxt, GetColor(15, 20, 30), font24);
    }
    else
    {
        DrawBox(btnX + 3, titleBtnY + 3, btnX + btnW + 3, titleBtnY + btnH + 3, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, titleBtnY - 1, btnX + btnW + 1, titleBtnY + btnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, titleBtnY, btnX + btnW, titleBtnY + btnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, titleBtnY + 2, btnX + btnW - 2, titleBtnY + 8, GetColor(60, 75, 110), TRUE);

        DrawStringToHandle(btnX + 20, titleBtnY + 13, "🏠", GetColor(140, 170, 210), font24);
        const char* titleTxt = "タイトルに戻る";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 14, titleTxt, GetColor(210, 230, 255), font24);
    }

    // --- 下部: ゲームに戻る (ESC) ガイドボタン ---
    int resumeBtnW = 240;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 230;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW &&
                       my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    DrawBox(resumeBtnX + 2, resumeBtnY + 2, resumeBtnX + resumeBtnW + 2, resumeBtnY + resumeBtnH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(resumeBtnX, resumeBtnY, resumeBtnX + resumeBtnW, resumeBtnY + resumeBtnH, hoverResume ? GetColor(45, 60, 90) : GetColor(24, 30, 46), TRUE);
    DrawBox(resumeBtnX, resumeBtnY, resumeBtnX + resumeBtnW, resumeBtnY + resumeBtnH, hoverResume ? GetColor(100, 220, 255) : GetColor(60, 75, 110), FALSE);

    const char* resumeTxt = "▶ ゲームに戻る (ESC)";
    int rW = GetDrawStringWidthToHandle(resumeTxt, static_cast<int>(strlen(resumeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - rW) / 2, resumeBtnY + 9, resumeTxt, hoverResume ? GetColor(255, 255, 255) : GetColor(180, 205, 235), font16);

    // 操作説明ガイド
    const char* guideTxt = "選択: [↑/↓]  決定: [ENTER] / [SPACE]";
    int gW = GetDrawStringWidthToHandle(guideTxt, static_cast<int>(strlen(guideTxt)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW) / 2, modalY + 288, guideTxt, GetColor(140, 160, 190), font13);
}