#include "GameManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "CsvManager.h"
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
    // ★ ユーザーリクエスト: 家具を一度どかす（家具配置を無効化）
    return;

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
        // NEW MAP (newmap.mv1) 用の家具配置（各部屋・通路エリア）
        // 1. 北東エリアのシステムキッチン
        PlaceObstacle(
            VGet(600.0f, 0.0f, 800.0f),
            300.0f, 75.0f, 70.0f,
            "", ModelConfig::KITCHEN_MODEL_PATH,
            GetColor(200, 200, 210), GetColor(150, 150, 160),
            ModelConfig::KITCHEN_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 2. 北東エリアの冷蔵庫
        PlaceObstacle(
            VGet(850.0f, 0.0f, 800.0f),
            80.0f, 160.0f, 64.0f,
            "", ModelConfig::REFRIGERATOR_MODEL_PATH,
            GetColor(220, 220, 230), GetColor(160, 160, 170),
            ModelConfig::REFRIGERATOR_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 3. 中央西エリアのテーブル
        PlaceObstacle(
            VGet(-500.0f, 0.0f, 600.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 4. 中央西エリアのテレビ
        PlaceObstacle(
            VGet(-500.0f, 0.0f, 150.0f),
            30.0f, 60.0f, 120.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.2f,
            DX_PI_F * 0.5f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 5. 東エリアのテーブル
        PlaceObstacle(
            VGet(1600.0f, 0.0f, -400.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 6. 南東エリアのテレビ
        PlaceObstacle(
            VGet(800.0f, 0.0f, -1400.0f),
            120.0f, 60.0f, 30.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.2f, 0.0f)
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
    // ゲーム開始・リトライ時に最新のCSV設定（Statu.csv / Position.csv）を読み込み反映
    CsvManager::GetInstance().ReloadAll();

    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();
    m_isPaused = false;
    m_pauseStartTime = 0;
    m_pauseMenuItem = 0;

    // 選択されたマップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);
    const auto& stage = m_objManager.GetStage();

    // プレイヤー生成（Position.csv の Cat 行から初期位置を取得）
    VECTOR defaultStartPos = (m_selectedMap == ModelConfig::MapType::House)
        ? VGet(0.0f, 0.0f, 0.0f)
        : VGet(0.0f, 0.0f, 450.0f);

    VECTOR startPos = defaultStartPos;
    std::vector<VECTOR> catPositions = CsvManager::GetInstance().GetCatPositionsForMap(m_selectedMap);
    if (!catPositions.empty())
    {
        VECTOR candidate = catPositions[rand() % catPositions.size()];
        float testGy = 0.0f;
        if (stage.GetGroundHeight(candidate, testGy))
        {
            startPos = VGet(candidate.x, testGy, candidate.z);
        }
    }

    auto player = std::make_shared<Player3D>(startPos);
    player->SetMapType(m_selectedMap);
    float playerGy = 0.0f;
    if (stage.GetGroundHeight(startPos, playerGy))
    {
        player->SetPos(VGet(startPos.x, playerGy, startPos.z));
        player->SetGroundY(playerGy);
    }
    m_objManager.SetPlayer(player);

    // カメラ初期化（猫の背後から開始）
    m_camera.Init(player->GetPos(), player->GetRotY());
    SetMouseDispFlag(FALSE); // ゲームプレイ中はカーソル非表示（マウス視点移動）

    // 部屋・ステージの家具配置
    SetupRoomObstacles();

    // ネズミの生成（Position.csv の Mouse 行からスポーン候補座標を取得）
    std::vector<VECTOR> spawnRooms = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    if (spawnRooms.empty())
    {
        spawnRooms.push_back(defaultStartPos);
    }
    int roomCount = static_cast<int>(spawnRooms.size());

    // ステージモデルの床上に確実に接地させるスポーン座標決定ヘルパー
    auto FindGuaranteedGroundSpawnPos = [&](int candidateIndex) -> VECTOR
    {
        for (int offsetAttempt = 0; offsetAttempt < roomCount; ++offsetAttempt)
        {
            int rIdx = (candidateIndex + offsetAttempt) % roomCount;
            const VECTOR& basePos = spawnRooms[rIdx];

            for (int jitterAttempt = 0; jitterAttempt < 6; ++jitterAttempt)
            {
                float ox = (jitterAttempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
                float oz = (jitterAttempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
                VECTOR testPos = VGet(basePos.x + ox, basePos.y, basePos.z + oz);
                float gy = 0.0f;
                if (stage.GetGroundHeight(testPos, gy))
                {
                    testPos.y = gy;
                    return testPos;
                }
            }
        }

        // 万一のセーフティネット：猫の初期位置近傍
        float fallbackGy = 0.0f;
        VECTOR fallbackPos = defaultStartPos;
        if (stage.GetGroundHeight(fallbackPos, fallbackGy))
        {
            fallbackPos.y = fallbackGy;
        }
        return fallbackPos;
    };

    for (int i = 0; i < Config::NORMAL_MOUSE_COUNT; ++i)
    {
        VECTOR spawnPos = FindGuaranteedGroundSpawnPos(i);
        auto mouse = std::make_shared<NormalMouse3D>(spawnPos, player);
        mouse->SetMapType(m_selectedMap);
        m_objManager.AddObject(mouse);
    }

    for (int i = 0; i < Config::FAST_MOUSE_COUNT; ++i)
    {
        VECTOR spawnPos = FindGuaranteedGroundSpawnPos(Config::NORMAL_MOUSE_COUNT + i);
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

        // マウス感度クイック調整（[-] / [[] でダウン、[+] / []] / [;] でアップ）
        bool keyMinus = (CheckHitKey(KEY_INPUT_MINUS) != 0 || CheckHitKey(KEY_INPUT_SUBTRACT) != 0 || CheckHitKey(KEY_INPUT_LBRACKET) != 0);
        bool keyPlus  = (CheckHitKey(KEY_INPUT_ADD) != 0 || CheckHitKey(KEY_INPUT_RBRACKET) != 0 || CheckHitKey(KEY_INPUT_SEMICOLON) != 0 || CheckHitKey(KEY_INPUT_COLON) != 0);

        if (keyMinus && !m_prevKeyMinus)
        {
            m_camera.AdjustMouseSensitivity(-0.1f);
            m_sensitivityNoticeTimer = 90; // 約1.5秒通知表示
        }
        if (keyPlus && !m_prevKeyPlus)
        {
            m_camera.AdjustMouseSensitivity(+0.1f);
            m_sensitivityNoticeTimer = 90;
        }
        m_prevKeyMinus = keyMinus;
        m_prevKeyPlus  = keyPlus;

        if (m_sensitivityNoticeTimer > 0)
        {
            m_sensitivityNoticeTimer--;
        }

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

        // [7] 10m×10m×10m目測キューブ表示モード切替 / [8] 10m全体目測用・広角カメラ切替
        if (m_cheatEnabled)
        {
            bool key7 = (CheckHitKey(KEY_INPUT_7) != 0);
            if (key7 && !m_prevKey7)
            {
                m_debugCubeMode = (m_debugCubeMode + 1) % 4;
            }
            m_prevKey7 = key7;

            bool key8 = (CheckHitKey(KEY_INPUT_8) != 0);
            if (key8 && !m_prevKey8)
            {
                m_debugWideCamera = !m_debugWideCamera;
            }
            m_prevKey8 = key8;
        }

        m_objManager.Update(m_camera);

        // F1デバッグ広角カメラがONの場合は、10m×10m×10mの■全体と猫の1.8m身長が一目で比較できる俯瞰視点にセット
        if (m_cheatEnabled && m_debugWideCamera && player)
        {
            VECTOR pPos = player->GetPos();
            float yaw = m_camera.GetAngleH();
            float dist = 780.0f; // 約13.4m後方から10m×10m×10mキューブ全体を収める
            float height = 520.0f; // 約9.0m高さから見下ろす
            VECTOR camPos = VGet(pPos.x - std::sin(yaw) * dist, pPos.y + height, pPos.z - std::cos(yaw) * dist);
            VECTOR target = VGet(pPos.x, pPos.y + 180.0f, pPos.z);
            SetCameraNearFar(10.0f, 30000.0f);
            SetCameraPositionAndTarget_UpVecY(camPos, target);
        }

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

    // F1デバッグモード時の3Dオーバーレイ描画（10m×10m×10m目測キューブ・1.8m身長バー・スポーン位置・当たり判定リング）
    if (m_state == GameState::Playing && m_cheatEnabled)
    {
        DrawDebug3DOverlay();
    }

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

    DrawFormatStringToHandle(Config::SCREEN_WIDTH - 170, 14, GetColor(140, 240, 140), font13, "FPS: %.1f (%.1fms)", m_currentFps, m_frameProcessTimeMs);

    if (m_state == GameState::Title)
    {
        DrawTitleScreen();
    }
    
    else if (m_state == GameState::Playing)
    {
        // ゲームプレイHUD（バカゲー風ポップ装飾）
        float currentSec = (m_isPaused ? (m_pauseStartTime - m_startCount) : (GetNowCount() - m_startCount)) / 1000.0f;

        // =========================================================================
        // 1. 左上：プレイヤーステータス ＆ 特大レベルバッジ（一目で現在Lv・現在スピードがわかる）
        // =========================================================================
        int pX = 14;
        int pY = 14;
        int pW = 365;
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

        // --- 右側：ミッション＆タイム＆プレイヤースピード情報 ---
        int infoX = badgeX + badgeW + 14;

        int remainMice = m_objManager.GetRemainingMouseCount();
        DrawStringToHandle(infoX, pY + 8, "ネズミ:", GetColor(200, 215, 240), font16);
        char miceBuf[32];
        snprintf(miceBuf, sizeof(miceBuf), "%d 匹", remainMice);
        DrawStringToHandle(infoX + 60, pY + 6, miceBuf, GetColor(255, 80, 80), font18);

        char timeBuf[32];
        snprintf(timeBuf, sizeof(timeBuf), "タイム: %.1f 秒", currentSec);
        DrawStringToHandle(infoX, pY + 32, timeBuf, white, font16);

        if (player)
        {
            float catSpeed = player->GetCurrentSpeed();
            float actualSpd = player->GetActualMoveSpeed();
            float mPerSec = (catSpeed * 60.0f) / Config::WORLD_UNITS_PER_METER;
            char spdBuf[96];
            snprintf(spdBuf, sizeof(spdBuf), "現在速度: %.2f (実移動:%.2f / %.1fm/s)", catSpeed, actualSpd, mPerSec);
            DrawStringToHandle(infoX, pY + 58, spdBuf, GetColor(100, 245, 140), font13);
        }

        // =========================================================================
        // 2. 画面右下：スマート操作ガイドカード（マウス攻撃・視点移動対応）
        // =========================================================================
        int gW = 590;
        int gH = 72;
        int gX = Config::SCREEN_WIDTH - gW - 20;
        int gY = Config::SCREEN_HEIGHT - gH - 20;

        // 半透明プレート（ゲーム画面・床を邪魔しないコンパクトサイズ）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
        DrawBox(gX - 3, gY - 3, gX + gW + 3, gY + gH + 3, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawBox(gX - 1, gY - 1, gX + gW + 1, gY + gH + 1, GetColor(80, 100, 140), FALSE);
        DrawBox(gX, gY, gX + gW, gY + gH, GetColor(18, 22, 34), TRUE);

        // 上段：アクションスキル（左クリック: 飛びつき、右クリック: スタン攻撃、SPACE: 筋トレ）
        if (reps >= 4)
        {
            float pounceCd = player ? static_cast<float>(player->GetPounceCooldown()) / 60.0f : 0.0f;
            float tackleCd = player ? static_cast<float>(player->GetTackleCooldown()) / 60.0f : 0.0f;

            if (pounceCd > 0.05f)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "[左] 飛びつき(%.1fs)", pounceCd);
                DrawStringToHandle(gX + 14, gY + 10, buf, GetColor(160, 170, 185), font16);
            }
            else
            {
                DrawStringToHandle(gX + 14, gY + 10, "[左クリック] 飛びつき", GetColor(255, 90, 90), font16);
            }

            if (tackleCd > 0.05f)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "[右] スタン(%.1fs)", tackleCd);
                DrawStringToHandle(gX + 200, gY + 10, buf, GetColor(160, 170, 185), font16);
            }
            else
            {
                DrawStringToHandle(gX + 200, gY + 10, "[右クリック] スタン攻撃", GetColor(255, 150, 40), font16);
            }
        }
        else
        {
            DrawStringToHandle(gX + 14, gY + 10, "[左/右クリック] 攻撃スキル (Lv.4で解放)", GetColor(140, 155, 180), font16);
        }
        DrawStringToHandle(gX + 400, gY + 10, "[SPACE] 筋トレ(長押し)", GetColor(100, 245, 140), font16);

        // 下段：基本移動・カメラ
        char camInfo[128];
        snprintf(camInfo, sizeof(camInfo), "移動: WASD  |  視点: マウス(感度:%.1fx) / 矢印  |  感度変更: [- / +]  |  視点リセット: [F]", m_camera.GetMouseSensitivity());
        DrawStringToHandle(gX + 14, gY + 42, camInfo, GetColor(210, 225, 245), font13);

        // =========================================================================
        // 3. F1 デバッグ情報＆ミニマップ＆10m距離目測オーバーレイ
        // =========================================================================
        if (m_cheatEnabled)
        {
            DrawDebug2DOverlay();
        }
        else
        {
            // F1 OFF時は右上に案内バッジを表示
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
            DrawBox(Config::SCREEN_WIDTH - 270, 34, Config::SCREEN_WIDTH - 10, 56, GetColor(15, 20, 35), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(Config::SCREEN_WIDTH - 270, 34, Config::SCREEN_WIDTH - 10, 56, GetColor(80, 120, 170), FALSE);
            DrawStringToHandle(Config::SCREEN_WIDTH - 262, 38, "[F1] デバッグ情報・マップ・10m目測表示", GetColor(160, 210, 255), font13);
        }

        // プレイ中の感度変更通知ポップアップ
        if (m_sensitivityNoticeTimer > 0 && !m_isPaused)
        {
            float alphaRatio = (m_sensitivityNoticeTimer < 20) ? (m_sensitivityNoticeTimer / 20.0f) : 1.0f;
            int alpha = static_cast<int>(230 * alphaRatio);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

            int notW = 300;
            int notH = 46;
            int notX = (Config::SCREEN_WIDTH - notW) / 2;
            int notY = 24;

            DrawBox(notX + 4, notY + 4, notX + notW + 4, notY + notH + 4, GetColor(0, 0, 0), TRUE);
            DrawBox(notX, notY, notX + notW, notY + notH, GetColor(18, 22, 36), TRUE);
            DrawBox(notX, notY, notX + notW, notY + notH, GetColor(100, 220, 255), FALSE);

            char notBuf[64];
            snprintf(notBuf, sizeof(notBuf), "🎯 マウス感度: %.1fx", m_camera.GetMouseSensitivity());
            int nW = GetDrawStringWidthToHandle(notBuf, static_cast<int>(strlen(notBuf)), font18);
            DrawStringToHandle((Config::SCREEN_WIDTH - nW) / 2, notY + 12, notBuf, GetColor(255, 230, 70), font18);

            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
        : "STAGE : MAP 2 (NEW MAP)";

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
        ? "★ [ ← / → ] またはクリックで MAP 2: NEW MAP に切替"
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
    DrawStringToHandle(c2X + 14, cardY + 6, "② アクション (Lv.4~)", GetColor(255, 150, 40), font16);

    DrawStringToHandle(c2X + 14, cardY + 46, "・[右クリック / E] スタン攻撃", GetColor(255, 150, 40), font13);
    DrawStringToHandle(c2X + 26, cardY + 68, "➔ 突進して気絶! (Lv.4以上)", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c2X + 14, cardY + 104, "・[左クリック / SHIFT] 飛びつき", GetColor(255, 90, 90), font13);
    DrawStringToHandle(c2X + 26, cardY + 126, "➔ 前方へ跳躍捕獲! (Lv.4以上)", GetColor(255, 255, 255), font13);

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

    DrawStringToHandle(col1X, row1Y + rowStep * 1, "[右クリック / E] スタン攻撃 (Lv.4~)", GetColor(255, 150, 40), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 1 + 20, "➔ 突進して気絶 (4 Rep以上で解放)", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 2, "[左クリック / SHIFT] 飛びつき (Lv.4~)", GetColor(255, 90, 90), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 2 + 20, "➔ 跳躍して即捕獲 (4 Rep以上で解放)", GetColor(210, 225, 245), font13);

    // 右列: 移動・カメラ
    DrawStringToHandle(col2X, row1Y + rowStep * 0, "移動: [W] [A] [S] [D]", GetColor(255, 255, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 0 + 20, "➔ キャラクターの前後左右移動", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 1, "カメラ回転: マウス移動 / 矢印 [←→↑↓]", GetColor(100, 220, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 1 + 20, "➔ 感度調整: [- / +] または ポーズ画面(ESC)", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 2, "視点リセット: [F]  |  一時停止: [ESC]", GetColor(180, 210, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 2 + 20, "➔ カメラを猫背後に戻す / ポーズ開閉", GetColor(210, 225, 245), font13);

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
// ポーズ中の更新処理（ESC / リトライ / タイトルに戻る / マウス感度調整）
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
    bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_MINUS) != 0 || CheckHitKey(KEY_INPUT_SUBTRACT) != 0 || CheckHitKey(KEY_INPUT_LBRACKET) != 0);
    bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0 || CheckHitKey(KEY_INPUT_ADD) != 0 || CheckHitKey(KEY_INPUT_RBRACKET) != 0 || CheckHitKey(KEY_INPUT_SEMICOLON) != 0 || CheckHitKey(KEY_INPUT_COLON) != 0);
    bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
    bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool keyR     = (CheckHitKey(KEY_INPUT_R) != 0);
    bool keyT     = (CheckHitKey(KEY_INPUT_T) != 0);

    bool triggerUp    = keyUp && !m_prevKeyUp;
    bool triggerDown  = keyDown && !m_prevKeyDown;
    bool triggerLeft  = keyLeft && !m_prevKeyLeft;
    bool triggerRight = keyRight && !m_prevKeyRight;
    bool triggerEnter = isEnter && !m_prevKeyEnter;
    bool triggerSpace = isSpace && !m_prevKeySpace;
    bool triggerR     = keyR && !m_prevKeyR;
    bool triggerT     = keyT && !m_prevKeyT;

    m_prevKeyUp    = keyUp;
    m_prevKeyDown  = keyDown;
    m_prevKeyLeft  = keyLeft;
    m_prevKeyRight = keyRight;
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

    // 上下キーで選択切り替え (0: リトライ, 1: タイトルに戻る, 2: マウス感度)
    if (triggerUp)
    {
        m_pauseMenuItem = (m_pauseMenuItem + 2) % 3;
    }
    if (triggerDown)
    {
        m_pauseMenuItem = (m_pauseMenuItem + 1) % 3;
    }

    // ボタン・スライダー当たり判定座標
    int modalW = 480;
    int modalH = 400;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    int btnW = 360;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;

    int retryBtnY = modalY + 68;
    int retryBtnH = 50;

    int titleBtnY = modalY + 126;
    int titleBtnH = 50;

    int sensRowY  = modalY + 184;
    int sensRowH  = 74;

    int sensBtnW   = 46;
    int sensBtnH   = 28;
    int sensBtnY   = sensRowY + 36;
    int minusBtnX  = btnX + 10;
    int sliderBarX = btnX + 64;
    int sliderBarW = 232;
    int sliderBarY = sensRowY + 44;
    int sliderBarH = 12;
    int plusBtnX   = btnX + 304;

    int resumeBtnW = 260;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 280;

    bool hoverRetry  = (mx >= btnX && mx <= btnX + btnW && my >= retryBtnY && my <= retryBtnY + retryBtnH);
    bool hoverTitle  = (mx >= btnX && mx <= btnX + btnW && my >= titleBtnY && my <= titleBtnY + titleBtnH);
    bool hoverSens   = (mx >= btnX && mx <= btnX + btnW && my >= sensRowY && my <= sensRowY + sensRowH);
    bool hoverMinus  = (mx >= minusBtnX && mx <= minusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);
    bool hoverPlus   = (mx >= plusBtnX && mx <= plusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);
    bool hoverSlider = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= sliderBarY - 8 && my <= sliderBarY + sliderBarH + 8);
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW && my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    if (hoverRetry) m_pauseMenuItem = 0;
    if (hoverTitle) m_pauseMenuItem = 1;
    if (hoverSens)  m_pauseMenuItem = 2;

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

    // 3. マウス感度調整処理
    // 左右キーまたは感度行選択中の左右操作
    if (triggerLeft)
    {
        m_camera.AdjustMouseSensitivity(-0.1f);
    }
    if (triggerRight)
    {
        m_camera.AdjustMouseSensitivity(+0.1f);
    }

    // 感度行が選択されている状態で Enter / Space が押されたら標準(1.0x)に戻す
    if ((triggerEnter || triggerSpace) && m_pauseMenuItem == 2 && !hoverMinus && !hoverPlus && !hoverSlider)
    {
        m_camera.SetMouseSensitivity(1.0f);
    }

    // マウスクリック操作: [◀ -] ボタン
    if (hoverMinus && isMouseTrigger)
    {
        m_camera.AdjustMouseSensitivity(-0.1f);
    }
    // マウスクリック操作: [+ ▶] ボタン
    if (hoverPlus && isMouseTrigger)
    {
        m_camera.AdjustMouseSensitivity(+0.1f);
    }
    // スライダーバー クリック / ドラッグ操作
    if (hoverSlider && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        float newSens = 0.2f + MathHelper::Clamp(ratio, 0.0f, 1.0f) * (3.0f - 0.2f);
        m_camera.SetMouseSensitivity(newSens);
    }

    // 4. ゲームに戻る判定（再開ボタンクリック）
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
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー（背面のゲーム画面が薄暗く透ける）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ外枠・本体
    int modalW = 480;
    int modalH = 400;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    // ドロップシャドウ
    DrawBox(modalX + 8, modalY + 8, modalX + modalW + 8, modalY + modalH + 8, GetColor(0, 0, 0), TRUE);
    // 極太枠
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
    int btnW = 360;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int retryBtnY = modalY + 68;
    int retryBtnH = 50;
    int titleBtnY = modalY + 126;
    int titleBtnH = 50;
    int sensRowY  = modalY + 184;
    int sensRowH  = 74;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: リトライ ---
    bool isRetrySelected = (m_pauseMenuItem == 0);
    if (isRetrySelected)
    {
        DrawBox(btnX + 5, retryBtnY + 5, btnX + btnW + 5, retryBtnY + retryBtnH + 5, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 2, retryBtnY - 2, btnX + btnW + 2, retryBtnY + retryBtnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, retryBtnY, btnX + btnW, retryBtnY + retryBtnH, GetColor(100, 230 + static_cast<int>(25 * btnPulse), 120), TRUE);
        DrawBox(btnX + 3, retryBtnY + 3, btnX + btnW - 3, retryBtnY + 11, GetColor(210, 255, 220), TRUE);

        DrawStringToHandle(btnX + 20, retryBtnY + 11, "🔄", GetColor(20, 20, 20), font24);
        const char* retryTxt = "リトライ (R)";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 12, retryTxt, GetColor(15, 25, 20), font24);
    }
    else
    {
        DrawBox(btnX + 3, retryBtnY + 3, btnX + btnW + 3, retryBtnY + retryBtnH + 3, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, retryBtnY - 1, btnX + btnW + 1, retryBtnY + retryBtnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, retryBtnY, btnX + btnW, retryBtnY + retryBtnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, retryBtnY + 2, btnX + btnW - 2, retryBtnY + 8, GetColor(60, 75, 110), TRUE);

        DrawStringToHandle(btnX + 20, retryBtnY + 11, "🔄", GetColor(140, 170, 210), font24);
        const char* retryTxt = "リトライ";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 12, retryTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: タイトルに戻る ---
    bool isTitleSelected = (m_pauseMenuItem == 1);
    if (isTitleSelected)
    {
        DrawBox(btnX + 5, titleBtnY + 5, btnX + btnW + 5, titleBtnY + titleBtnH + 5, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 2, titleBtnY - 2, btnX + btnW + 2, titleBtnY + titleBtnH + 2, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, titleBtnY, btnX + btnW, titleBtnY + titleBtnH, GetColor(70, 190 + static_cast<int>(35 * btnPulse), 255), TRUE);
        DrawBox(btnX + 3, titleBtnY + 3, btnX + btnW - 3, titleBtnY + 11, GetColor(210, 240, 255), TRUE);

        DrawStringToHandle(btnX + 20, titleBtnY + 11, "🏠", GetColor(20, 20, 20), font24);
        const char* titleTxt = "タイトルに戻る (T)";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 12, titleTxt, GetColor(15, 20, 30), font24);
    }
    else
    {
        DrawBox(btnX + 3, titleBtnY + 3, btnX + btnW + 3, titleBtnY + titleBtnH + 3, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX - 1, titleBtnY - 1, btnX + btnW + 1, titleBtnY + titleBtnH + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(btnX, titleBtnY, btnX + btnW, titleBtnY + titleBtnH, GetColor(32, 42, 65), TRUE);
        DrawBox(btnX + 2, titleBtnY + 2, btnX + btnW - 2, titleBtnY + 8, GetColor(60, 75, 110), TRUE);

        DrawStringToHandle(btnX + 20, titleBtnY + 11, "🏠", GetColor(140, 170, 210), font24);
        const char* titleTxt = "タイトルに戻る";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 12, titleTxt, GetColor(210, 230, 255), font24);
    }

    // --- コントロール3: マウス感度調整 ---
    bool isSensSelected = (m_pauseMenuItem == 2);
    int sensBtnW   = 46;
    int sensBtnH   = 28;
    int sensBtnY   = sensRowY + 36;
    int minusBtnX  = btnX + 10;
    int sliderBarX = btnX + 64;
    int sliderBarW = 232;
    int sliderBarY = sensRowY + 44;
    int sliderBarH = 12;
    int plusBtnX   = btnX + 304;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverMinus = (mx >= minusBtnX && mx <= minusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);
    bool hoverPlus  = (mx >= plusBtnX && mx <= plusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);

    // 感度行の外枠・背景
    DrawBox(btnX + 3, sensRowY + 3, btnX + btnW + 3, sensRowY + sensRowH + 3, GetColor(0, 0, 0), TRUE);
    DrawBox(btnX - 1, sensRowY - 1, btnX + btnW + 1, sensRowY + sensRowH + 1, isSensSelected ? GetColor(255, 180, 50) : GetColor(0, 0, 0), TRUE);
    DrawBox(btnX, sensRowY, btnX + btnW, sensRowY + sensRowH, isSensSelected ? GetColor(28, 38, 62) : GetColor(24, 30, 48), TRUE);
    if (isSensSelected)
    {
        DrawBox(btnX, sensRowY, btnX + btnW, sensRowY + sensRowH, GetColor(255, 180, 50), FALSE);
    }

    // 感度テキスト表示（上段）
    float currentSens = m_camera.GetMouseSensitivity();
    char sensTxt[64];
    snprintf(sensTxt, sizeof(sensTxt), "🎯 マウス感度: %.1fx", currentSens);
    DrawStringToHandle(btnX + 14, sensRowY + 9, sensTxt, isSensSelected ? GetColor(255, 230, 90) : GetColor(220, 235, 255), font16);

    const char* sensHint = "[←/→ で変更]";
    DrawStringToHandle(btnX + btnW - 110, sensRowY + 11, sensHint, GetColor(140, 165, 195), font13);

    // [◀ -] ボタン描画
    DrawBox(minusBtnX, sensBtnY, minusBtnX + sensBtnW, sensBtnY + sensBtnH, hoverMinus ? GetColor(60, 90, 140) : GetColor(38, 48, 75), TRUE);
    DrawBox(minusBtnX, sensBtnY, minusBtnX + sensBtnW, sensBtnY + sensBtnH, hoverMinus ? GetColor(100, 220, 255) : GetColor(70, 85, 120), FALSE);
    DrawStringToHandle(minusBtnX + 11, sensBtnY + 5, "◀ -", hoverMinus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font16);

    // スライダーバー背景（溝）
    DrawBox(sliderBarX, sliderBarY, sliderBarX + sliderBarW, sliderBarY + sliderBarH, GetColor(12, 16, 26), TRUE);
    DrawBox(sliderBarX, sliderBarY, sliderBarX + sliderBarW, sliderBarY + sliderBarH, GetColor(60, 75, 105), FALSE);

    // 1.0x（標準位置）の目盛りマーク
    float standardRatio = (1.0f - 0.2f) / (3.0f - 0.2f);
    int standardX = sliderBarX + static_cast<int>(sliderBarW * standardRatio);
    DrawLine(standardX, sliderBarY - 3, standardX, sliderBarY + sliderBarH + 3, GetColor(180, 200, 220));

    // 現在感度に応じた塗りつぶしプログレス
    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    sensRatio = MathHelper::Clamp(sensRatio, 0.0f, 1.0f);
    int fillW = static_cast<int>(sliderBarW * sensRatio);
    if (fillW > 0)
    {
        unsigned int progColor = (currentSens >= 2.0f) ? GetColor(255, 120, 60) : ((currentSens >= 1.0f) ? GetColor(100, 220, 255) : GetColor(70, 180, 240));
        DrawBox(sliderBarX + 1, sliderBarY + 1, sliderBarX + fillW, sliderBarY + sliderBarH - 1, progColor, TRUE);
    }

    // スライダーつまみ（ノブ）
    int knobX = sliderBarX + fillW;
    int knobY = sliderBarY + sliderBarH / 2;
    DrawCircle(knobX, knobY, 8, GetColor(0, 0, 0), TRUE);
    DrawCircle(knobX, knobY, 7, isSensSelected ? GetColor(255, 230, 80) : GetColor(220, 240, 255), TRUE);
    DrawCircle(knobX, knobY, 4, GetColor(30, 40, 60), TRUE);

    // [+ ▶] ボタン描画
    DrawBox(plusBtnX, sensBtnY, plusBtnX + sensBtnW, sensBtnY + sensBtnH, hoverPlus ? GetColor(60, 90, 140) : GetColor(38, 48, 75), TRUE);
    DrawBox(plusBtnX, sensBtnY, plusBtnX + sensBtnW, sensBtnY + sensBtnH, hoverPlus ? GetColor(100, 220, 255) : GetColor(70, 85, 120), FALSE);
    DrawStringToHandle(plusBtnX + 11, sensBtnY + 5, "+ ▶", hoverPlus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font16);

    // --- 下部: ゲームに戻る (ESC) ガイドボタン ---
    int resumeBtnW = 260;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 280;

    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW &&
                       my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    DrawBox(resumeBtnX + 2, resumeBtnY + 2, resumeBtnX + resumeBtnW + 2, resumeBtnY + resumeBtnH + 2, GetColor(0, 0, 0), TRUE);
    DrawBox(resumeBtnX, resumeBtnY, resumeBtnX + resumeBtnW, resumeBtnY + resumeBtnH, hoverResume ? GetColor(45, 60, 90) : GetColor(24, 30, 46), TRUE);
    DrawBox(resumeBtnX, resumeBtnY, resumeBtnX + resumeBtnW, resumeBtnY + resumeBtnH, hoverResume ? GetColor(100, 220, 255) : GetColor(60, 75, 110), FALSE);

    const char* resumeTxt = "▶ ゲームに戻る (ESC)";
    int rW = GetDrawStringWidthToHandle(resumeTxt, static_cast<int>(strlen(resumeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - rW) / 2, resumeBtnY + 9, resumeTxt, hoverResume ? GetColor(255, 255, 255) : GetColor(180, 205, 235), font16);

    // 操作説明ガイド
    const char* guideTxt1 = "選択: [↑/↓]  感度変更: [←/→] または クリック";
    int gW1 = GetDrawStringWidthToHandle(guideTxt1, static_cast<int>(strlen(guideTxt1)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW1) / 2, modalY + 338, guideTxt1, GetColor(160, 185, 220), font13);

    const char* guideTxt2 = "決定: [ENTER] / [SPACE]  |  閉じる: [ESC]";
    int gW2 = GetDrawStringWidthToHandle(guideTxt2, static_cast<int>(strlen(guideTxt2)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW2) / 2, modalY + 360, guideTxt2, GetColor(130, 150, 180), font13);
}

void GameManager::DrawDebug3DOverlay()
{
    auto player = m_objManager.GetPlayer();
    if (!player)
    {
        return;
    }

    // 3Dデバッグ辺（DrawCapsule3D）がステージ照明の影で暗くならないようライティングを一時OFFにして鮮やかな色で描画
    SetUseLighting(FALSE);

    const auto& stage = m_objManager.GetStage();
    const VECTOR pPos = player->GetPos();
    const float unitPerMeter = Config::WORLD_UNITS_PER_METER; // 58.0533f (猫身長1.8m = 104.496u 基準)
    const float cubeSize = Config::CUBE_10M_SIZE_UNITS;       // 580.5333f (10m)
    const float halfCube = cubeSize * 0.5f;                   // 290.2667f (5m)

    // 白いステージ床の上でもクッキリ視認できる3Dソリッド辺描画ヘルパー (DrawCapsule3D)
    auto drawBeam3D = [](VECTOR a, VECTOR b, float radius, unsigned int col)
    {
        DrawCapsule3D(a, b, radius, 4, col, col, TRUE);
    };

    // 3D水平サークル描画ヘルパー
    auto drawCircle3D = [](const VECTOR& center, float radius, float yOffset, unsigned int col, int segments = 32)
    {
        const float step = (DX_PI_F * 2.0f) / static_cast<float>(segments);
        for (int i = 0; i < segments; ++i)
        {
            float a0 = step * static_cast<float>(i);
            float a1 = step * static_cast<float>(i + 1);
            VECTOR p0 = VGet(center.x + std::cos(a0) * radius, center.y + yOffset, center.z + std::sin(a0) * radius);
            VECTOR p1 = VGet(center.x + std::cos(a1) * radius, center.y + yOffset, center.z + std::sin(a1) * radius);
            DrawLine3D(p0, p1, col);
        }
    };

    // =========================================================================
    // 1. ステージ全体の10mグリッド目盛り（モード0, 1, 2のとき床面に10m間隔ラインを描画）
    // =========================================================================
    if (m_debugCubeMode != 3)
    {
        VECTOR minB = stage.GetMinBounds();
        VECTOR maxB = stage.GetMaxBounds();
        float floorY = pPos.y + 1.5f;

        unsigned int stageGridCol = GetColor(70, 130, 180);
        unsigned int stageAxisCol = GetColor(20, 90, 210);

        int minIdxX = static_cast<int>(std::floor(minB.x / cubeSize));
        int maxIdxX = static_cast<int>(std::ceil(maxB.x / cubeSize));
        int minIdxZ = static_cast<int>(std::floor(minB.z / cubeSize));
        int maxIdxZ = static_cast<int>(std::ceil(maxB.z / cubeSize));

        for (int ix = minIdxX; ix <= maxIdxX; ++ix)
        {
            float x = static_cast<float>(ix) * cubeSize;
            if (x < minB.x || x > maxB.x) continue;
            unsigned int c = (ix == 0) ? stageAxisCol : stageGridCol;
            DrawLine3D(VGet(x, floorY, minB.z), VGet(x, floorY, maxB.z), c);
        }
        for (int iz = minIdxZ; iz <= maxIdxZ; ++iz)
        {
            float z = static_cast<float>(iz) * cubeSize;
            if (z < minB.z || z > maxB.z) continue;
            unsigned int c = (iz == 0) ? stageAxisCol : stageGridCol;
            DrawLine3D(VGet(minB.x, floorY, z), VGet(maxB.x, floorY, z), c);
        }
    }

    // =========================================================================
    // 2. 10m × 10m × 10m の■（ワイヤーフレーム立方体）を12本の辺で再現（距離目測用）
    //    猫の基本身長 104.496u = 1.8m としたとき、1m = 58.053u、10m = 580.533u
    // =========================================================================
    if (m_debugCubeMode != 3)
    {
        VECTOR cubeCenter = pPos;
        if (m_debugCubeMode == 1)
        {
            // モード1: カメラ前方へ10m伸びる配置（通常カメラでも足元0m〜前方10mの奥行き全体が一目で見える）
            float yaw = m_camera.GetAngleH();
            cubeCenter = VGet(pPos.x + std::sin(yaw) * halfCube, pPos.y, pPos.z + std::cos(yaw) * halfCube);
        }
        else if (m_debugCubeMode == 2)
        {
            // モード2: ステージ原点(0,0,0)中心
            cubeCenter = VGet(0.0f, pPos.y, 0.0f);
        }

        float baseY = cubeCenter.y + 2.2f; // 床と重ならない高さ
        float topY  = baseY + cubeSize;    // +10m (580.53u)
        float midY  = baseY + halfCube;    // +5m  (290.27u)

        float x0 = cubeCenter.x - halfCube;
        float x1 = cubeCenter.x + halfCube;
        float z0 = cubeCenter.z - halfCube;
        float z1 = cubeCenter.z + halfCube;

        // 底面4頂点 (0m) と 上面4頂点 (+10m)
        VECTOR b00 = VGet(x0, baseY, z0);
        VECTOR b10 = VGet(x1, baseY, z0);
        VECTOR b11 = VGet(x1, baseY, z1);
        VECTOR b01 = VGet(x0, baseY, z1);

        VECTOR t00 = VGet(x0, topY, z0);
        VECTOR t10 = VGet(x1, topY, z0);
        VECTOR t11 = VGet(x1, topY, z1);
        VECTOR t01 = VGet(x0, topY, z1);

        // 中間5m高さの4頂点 (+5m)
        VECTOR m00 = VGet(x0, midY, z0);
        VECTOR m10 = VGet(x1, midY, z0);
        VECTOR m11 = VGet(x1, midY, z1);
        VECTOR m01 = VGet(x0, midY, z1);

        // 底面1m刻みグリッド（10m×10mの内部を1m=58.05u間隔で分割して距離目測しやすくする）
        unsigned int grid1mCol = GetColor(30, 145, 175);
        unsigned int grid5mCol = GetColor(0, 110, 235);
        for (int i = 1; i < 10; ++i)
        {
            float offset = -halfCube + static_cast<float>(i) * unitPerMeter;
            VECTOR gx0 = VGet(x0, baseY, cubeCenter.z + offset);
            VECTOR gx1 = VGet(x1, baseY, cubeCenter.z + offset);
            VECTOR gz0 = VGet(cubeCenter.x + offset, baseY, z0);
            VECTOR gz1 = VGet(cubeCenter.x + offset, baseY, z1);

            if (i == 5)
            {
                // 中心5m十字ラインは太めのビームで強調
                drawBeam3D(gx0, gx1, 1.3f, grid5mCol);
                drawBeam3D(gz0, gz1, 1.3f, grid5mCol);
            }
            else
            {
                DrawLine3D(gx0, gx1, grid1mCol);
                DrawLine3D(gz0, gz1, grid1mCol);
            }
        }

        // 10m×10m×10m 立方体の12本のメイン辺をソリッド3Dビームで描画（白い床でも鮮明に見える高コントラスト色）
        unsigned int bottomEdgeCol = GetColor(0, 95, 255);    // 底面4辺(10m×10m): 鮮やかなエレクトリックブルー
        unsigned int pillarEdgeCol = GetColor(255, 55, 20);   // 垂直4辺(高さ10m): 鮮やかなクリムゾンレッド
        unsigned int topEdgeCol    = GetColor(0, 170, 240);   // 上面4辺(10m天井): ディープシアン
        unsigned int midBeltCol    = GetColor(235, 135, 0);   // 中間5mベルト: アンバーオレンジ

        const float mainEdgeR = 2.6f;
        const float subEdgeR  = 1.3f;

        // 底面4辺 (10m × 10m)
        drawBeam3D(b00, b10, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b10, b11, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b11, b01, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b01, b00, mainEdgeR, bottomEdgeCol);

        // 上面4辺 (高さ10m)
        drawBeam3D(t00, t10, mainEdgeR, topEdgeCol);
        drawBeam3D(t10, t11, mainEdgeR, topEdgeCol);
        drawBeam3D(t11, t01, mainEdgeR, topEdgeCol);
        drawBeam3D(t01, t00, mainEdgeR, topEdgeCol);

        // 垂直の4柱 (0m -> 10m)
        drawBeam3D(b00, t00, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b10, t10, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b11, t11, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b01, t01, mainEdgeR, pillarEdgeCol);

        // 高さ5mの中間ベルト
        drawBeam3D(m00, m10, subEdgeR, midBeltCol);
        drawBeam3D(m10, m11, subEdgeR, midBeltCol);
        drawBeam3D(m11, m01, subEdgeR, midBeltCol);
        drawBeam3D(m01, m00, subEdgeR, midBeltCol);

        // 垂直4柱に1mごとの高さ目盛りティック（1m〜9m）を入れる
        float tickLen = 16.0f;
        for (int m = 1; m < 10; ++m)
        {
            float ty = baseY + static_cast<float>(m) * unitPerMeter;
            float tLen = (m == 5) ? tickLen * 1.8f : tickLen;
            unsigned int tCol = (m == 5) ? midBeltCol : pillarEdgeCol;

            drawBeam3D(VGet(x0, ty, z0), VGet(x0 + tLen, ty, z0 + tLen), 1.0f, tCol);
            drawBeam3D(VGet(x1, ty, z0), VGet(x1 - tLen, ty, z0 + tLen), 1.0f, tCol);
            drawBeam3D(VGet(x1, ty, z1), VGet(x1 - tLen, ty, z1 - tLen), 1.0f, tCol);
            drawBeam3D(VGet(x0, ty, z1), VGet(x0 + tLen, ty, z1 - tLen), 1.0f, tCol);
        }
    }

    // =========================================================================
    // 3. 猫の横の「身長1.8m基準ルーラー（物差し）」と当たり判定・攻撃範囲リング
    // =========================================================================
    {
        float rulerX = pPos.x + player->GetBodyRadius() + 26.0f;
        float rulerZ = pPos.z;
        float baseH  = Config::CAT_BASE_HEIGHT_UNITS; // 104.496u (1.80m)
        float curH   = baseH * (player->GetCurrentHeightMeters() / Config::CAT_BASE_HEIGHT_METERS);

        unsigned int rulerCol = GetColor(215, 0, 160);
        drawBeam3D(VGet(rulerX, pPos.y, rulerZ), VGet(rulerX, pPos.y + baseH, rulerZ), 1.5f, rulerCol);

        // 0m, 0.5m, 1.0m, 1.5m, 1.8m の目盛り横線
        const float marksM[] = { 0.0f, 0.5f, 1.0f, 1.5f, 1.8f };
        for (float m : marksM)
        {
            float my = pPos.y + m * unitPerMeter;
            float w = (m == 1.8f || m == 0.0f) ? 16.0f : 9.0f;
            drawBeam3D(VGet(rulerX - w, my, rulerZ), VGet(rulerX + w, my, rulerZ), 1.0f, rulerCol);
        }

        // パンプアップで1.8mより大きくなっている場合は現在身長までオレンジ線を延長
        if (curH > baseH + 1.0f)
        {
            unsigned int pumpCol = GetColor(255, 120, 0);
            drawBeam3D(VGet(rulerX, pPos.y + baseH, rulerZ), VGet(rulerX, pPos.y + curH, rulerZ), 1.5f, pumpCol);
            drawBeam3D(VGet(rulerX - 18.0f, pPos.y + curH, rulerZ), VGet(rulerX + 18.0f, pPos.y + curH, rulerZ), 1.1f, pumpCol);
        }

        // プレイヤーの体当たり半径・飛びつき半径・タックル半径リング
        drawCircle3D(pPos, player->GetBodyRadius(), 2.5f, GetColor(0, 190, 90), 28);
        if (player->GetRepCount() >= 4)
        {
            drawCircle3D(pPos, player->GetPounceRadius(), 3.0f, GetColor(240, 40, 40), 32);
            drawCircle3D(pPos, player->GetTackleRadius(), 3.5f, GetColor(240, 120, 0), 36);
        }
    }

    // =========================================================================
    // 4. Position.csv のスポーン/巡回座標 (Pos1〜Pos8) の3Dピン表示
    // =========================================================================
    std::vector<VECTOR> csvPositions = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    {
        unsigned int posCol = GetColor(20, 190, 60);
        for (size_t i = 0; i < csvPositions.size(); ++i)
        {
            VECTOR wp = csvPositions[i];
            float gy = wp.y;
            stage.GetGroundHeight(VGet(wp.x, wp.y + 100.0f, wp.z), gy);

            VECTOR groundPt = VGet(wp.x, gy + 2.0f, wp.z);
            VECTOR topPt    = VGet(wp.x, gy + 120.0f, wp.z);
            drawBeam3D(groundPt, topPt, 1.8f, posCol);
            drawCircle3D(VGet(wp.x, gy, wp.z), 28.0f, 2.5f, posCol, 20);
        }
    }

    // =========================================================================
    // 5. 生存ネズミへの3D距離ライン＆逃走感知リング
    // =========================================================================
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            VECTOR mPos = obj->GetPos();
            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            unsigned int mCol = isFast ? GetColor(235, 45, 45) : GetColor(220, 140, 0);

            // 猫からネズミへの3D直線
            DrawLine3D(VGet(pPos.x, pPos.y + 25.0f, pPos.z), VGet(mPos.x, mPos.y + 15.0f, mPos.z), mCol);
            // ネズミの当たり判定リング
            drawCircle3D(mPos, obj->GetRadius(), 2.5f, mCol, 20);
            // FastMouseの逃走開始距離リング (160u = 2.76m)
            if (isFast)
            {
                drawCircle3D(mPos, 160.0f, 2.5f, GetColor(210, 40, 150), 28);
            }
        }
    }

    SetUseLighting(TRUE);
}

void GameManager::DrawDebug2DOverlay()
{
    auto player = m_objManager.GetPlayer();
    if (!player)
    {
        return;
    }

    const auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();

    const auto& stage = m_objManager.GetStage();
    const VECTOR pPos = player->GetPos();
    const float unitPerMeter = Config::WORLD_UNITS_PER_METER; // 58.0533f
    const float cubeSize = Config::CUBE_10M_SIZE_UNITS;       // 580.5333f (10m)
    const float halfCube = cubeSize * 0.5f;                   // 290.2667f (5m)

    // 3Dワールド座標を2D画面上にタグ描画するヘルパー（左右のUIパネル領域と重なる場合はスキップ）
    auto drawWorldLabel = [&](const VECTOR& worldPos, const char* text, unsigned int textCol, unsigned int bgCol = GetColor(10, 16, 28))
    {
        VECTOR sp = ConvWorldPosToScreenPos(worldPos);
        if (sp.z > 0.0f && sp.z < 1.0f &&
            sp.x >= 0.0f && sp.x <= static_cast<float>(Config::SCREEN_WIDTH) &&
            sp.y >= 0.0f && sp.y <= static_cast<float>(Config::SCREEN_HEIGHT))
        {
            int tw = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), font13);
            int tx = static_cast<int>(sp.x) - tw / 2;
            int ty = static_cast<int>(sp.y) - 10;

            // 左側ステータス/デバッグパネル領域または右側ミニマップ領域と重なる場合は描画しない
            if (tx < 435 && ty < 608) return;
            if (tx + tw > Config::SCREEN_WIDTH - 365 && ty < 530) return;
            if (tx + tw > Config::SCREEN_WIDTH - 615 && ty > Config::SCREEN_HEIGHT - 95) return;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 205);
            DrawBox(tx - 4, ty - 2, tx + tw + 4, ty + 17, bgCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(tx - 4, ty - 2, tx + tw + 4, ty + 17, textCol, FALSE);
            DrawStringToHandle(tx, ty, text, textCol, font13);
        }
    };

    // =========================================================================
    // 1. 3D空間上の距離目測ラベル投影（10m×10m×10mキューブ・猫1.8m身長・Pos1〜8・ネズミ距離）
    // =========================================================================
    VECTOR cubeCenter = pPos;
    if (m_debugCubeMode == 1)
    {
        float yaw = m_camera.GetAngleH();
        cubeCenter = VGet(pPos.x + std::sin(yaw) * halfCube, pPos.y, pPos.z + std::cos(yaw) * halfCube);
    }
    else if (m_debugCubeMode == 2)
    {
        cubeCenter = VGet(0.0f, pPos.y, 0.0f);
    }

    if (m_debugCubeMode != 3)
    {
        float baseY = cubeCenter.y + 2.2f;
        float topY  = baseY + cubeSize;
        float midY  = baseY + halfCube;

        unsigned int cyanCol   = GetColor(80, 235, 255);
        unsigned int yellowCol = GetColor(255, 220, 80);

        // 底面4辺の中央に「10m (580.5u)」ラベルを表示
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z - halfCube), "◀ 10m辺 (580.5u) ▶", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z + halfCube), "◀ 10m辺 (580.5u) ▶", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x - halfCube, baseY, cubeCenter.z), "▲ 10m辺 (580.5u) ▼", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x + halfCube, baseY, cubeCenter.z), "▲ 10m辺 (580.5u) ▼", cyanCol);

        // 垂直柱に「高さ 5m」「高さ 10m」ラベルを表示
        drawWorldLabel(VGet(cubeCenter.x - halfCube, midY, cubeCenter.z - halfCube), "高さ 5.0m (290.3u)", yellowCol);
        drawWorldLabel(VGet(cubeCenter.x - halfCube, topY, cubeCenter.z - halfCube), "■ 高さ 10.0m (580.5u)", yellowCol);
        drawWorldLabel(VGet(cubeCenter.x + halfCube, topY, cubeCenter.z + halfCube), "■ 10m×10m×10m 頂点", yellowCol);

        // 中心から前後左右2m・4m地点の目盛りラベル
        for (int m : { -4, -2, 2, 4 })
        {
            char mBuf[24];
            snprintf(mBuf, sizeof(mBuf), "%+dm", m);
            drawWorldLabel(VGet(cubeCenter.x + m * unitPerMeter, baseY, cubeCenter.z), mBuf, GetColor(160, 240, 255));
            drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z + m * unitPerMeter), mBuf, GetColor(160, 240, 255));
        }
    }

    // 猫の1.8m身長ルーラーのラベル
    {
        float rulerX = pPos.x + player->GetBodyRadius() + 26.0f;
        float baseH  = Config::CAT_BASE_HEIGHT_UNITS;
        float curHm  = player->GetCurrentHeightMeters();
        char hBuf[64];
        if (curHm > Config::CAT_BASE_HEIGHT_METERS + 0.02f)
        {
            snprintf(hBuf, sizeof(hBuf), "猫身長 1.80m基準 (現在:%.2fm)", curHm);
        }
        else
        {
            snprintf(hBuf, sizeof(hBuf), "猫身長 1.80m (104.5u)");
        }
        drawWorldLabel(VGet(rulerX, pPos.y + baseH + 8.0f, pPos.z), hBuf, GetColor(255, 150, 235));
    }

    // Position.csv (Pos1〜Pos8) の3Dラベル
    std::vector<VECTOR> csvPositions = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    for (size_t i = 0; i < csvPositions.size(); ++i)
    {
        VECTOR wp = csvPositions[i];
        float gy = wp.y;
        stage.GetGroundHeight(VGet(wp.x, wp.y + 100.0f, wp.z), gy);
        char pLabel[48];
        snprintf(pLabel, sizeof(pLabel), "Pos%zu (%.0f, %.0f)", i + 1, wp.x, wp.z);
        drawWorldLabel(VGet(wp.x, gy + 125.0f, wp.z), pLabel, GetColor(130, 255, 130));
    }

    // 生存ネズミ頭上のスピード＆距離タグ
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (!mouse) continue;

            VECTOR mPos = mouse->GetPos();
            float dx = mPos.x - pPos.x;
            float dy = mPos.y - pPos.y;
            float dz = mPos.z - pPos.z;
            float distU = std::sqrt(dx * dx + dy * dy + dz * dz);
            float distM = distU / unitPerMeter;

            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            float mSpd = mouse->GetCurrentSpeed();
            char mTag[64];
            snprintf(mTag, sizeof(mTag), "%s SPD:%.2f (%.1fm)", isFast ? "Fast" : "Norm", mSpd, distM);
            drawWorldLabel(VGet(mPos.x, mPos.y + 42.0f, mPos.z), mTag, isFast ? GetColor(255, 120, 120) : GetColor(255, 220, 100));
        }
    }

    // =========================================================================
    // 2. 左側：詳細デバッグパネル（スピード数値・10m目測スケール・座標・ステージ＆CSV情報）
    // =========================================================================
    int panelX = 14;
    int panelY = 106;
    int panelW = 415;
    int panelH = 495;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(10, 16, 28), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 220, 200), FALSE);

    int ty = panelY + 8;
    int lx = panelX + 10;

    DrawStringToHandle(lx, ty, "■ [F1] 統合デバッグ情報 (速度・10m目測・マップ)", GetColor(80, 255, 230), font16);
    ty += 24;
    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (A) プレイヤー現在スピード数値セクション ---
    float cfgSpd    = player->GetCurrentSpeed();
    float actSpd    = player->GetActualMoveSpeed();
    float cfgUnitS  = cfgSpd * 60.0f;
    float cfgMeterS = cfgUnitS / unitPerMeter;
    float cfgKmH    = cfgMeterS * 3.6f;
    float actMeterS = (actSpd * 60.0f) / unitPerMeter;

    DrawStringToHandle(lx, ty, "【プレイヤー現在スピード (数値)】", GetColor(255, 230, 80), font13);
    ty += 18;

    char spdLine1[96];
    snprintf(spdLine1, sizeof(spdLine1), "・設定スピード : %.2f u/f  (%.1f u/s)", cfgSpd, cfgUnitS);
    DrawStringToHandle(lx, ty, spdLine1, GetColor(120, 255, 150), font16);
    ty += 21;

    char spdLine2[96];
    snprintf(spdLine2, sizeof(spdLine2), "・実移動スピード: %.2f u/f  (%.2f m/s)", actSpd, actMeterS);
    DrawStringToHandle(lx, ty, spdLine2, GetColor(180, 240, 255), font13);
    ty += 18;

    char spdLine3[96];
    snprintf(spdLine3, sizeof(spdLine3), "・1.8m換算速度 : 秒速 %.2f m/s (時速 %.1f km/h)", cfgMeterS, cfgKmH);
    DrawStringToHandle(lx, ty, spdLine3, GetColor(255, 210, 110), font13);
    ty += 18;

    // スピード視覚バー (0.0 〜 6.0 u/f)
    int barX = lx + 4;
    int barW = panelW - 28;
    int barH = 10;
    DrawBox(barX, ty, barX + barW, ty + barH, GetColor(22, 30, 48), TRUE);
    float spdRatio = MathHelper::Clamp(cfgSpd / 6.0f, 0.0f, 1.0f);
    float actRatio = MathHelper::Clamp(actSpd / 6.0f, 0.0f, 1.0f);
    DrawBox(barX, ty, barX + static_cast<int>(barW * spdRatio), ty + barH, GetColor(40, 180, 100), TRUE);
    DrawBox(barX, ty + 2, barX + static_cast<int>(barW * actRatio), ty + barH - 2, GetColor(120, 255, 160), TRUE);
    DrawBox(barX, ty, barX + barW, ty + barH, GetColor(80, 120, 150), FALSE);
    ty += 15;

    const auto& catCsv = CsvManager::GetInstance().GetCatStatus();
    {
        char csvCatBuf[96];
        snprintf(csvCatBuf, sizeof(csvCatBuf), "・Statu.csv[Cat]: 初速%.2f / +%.2f(1Lv) / 上限%.2f",
                 catCsv.baseSpeed, catCsv.speedGrowthPerUnit, catCsv.maxSpeed);
        DrawStringToHandle(lx, ty, csvCatBuf, GetColor(180, 205, 230), font13);
        ty += 17;
    }

    char skillSpdBuf[96];
    snprintf(skillSpdBuf, sizeof(skillSpdBuf), "・スキル突進速度: 飛びつき 14.0u/f | タックル 11.5u/f");
    DrawStringToHandle(lx, ty, skillSpdBuf, GetColor(180, 205, 230), font13);
    ty += 20;

    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (B) 10m×10m×10m 距離目測スケール ＆ 座標セクション ---
    const char* cubeModeNames[] = {
        "猫中心 10m×10m×10m ■ + 10mグリッド",
        "猫前方 10m×10m×10m ■ + 10mグリッド",
        "原点(0,0,0) 10m×10m×10m ■ + グリッド",
        "OFF (非表示)"
    };
    DrawStringToHandle(lx, ty, "【距離目測スケール (猫の身長=1.8m基準)】", GetColor(255, 230, 80), font13);
    ty += 18;

    char scaleBuf1[96];
    snprintf(scaleBuf1, sizeof(scaleBuf1), "・基準: 猫身長 1.80m = %.1fu (現在身長: %.2fm)",
             Config::CAT_BASE_HEIGHT_UNITS, player->GetCurrentHeightMeters());
    DrawStringToHandle(lx, ty, scaleBuf1, GetColor(255, 180, 235), font13);
    ty += 17;

    char scaleBuf2[96];
    snprintf(scaleBuf2, sizeof(scaleBuf2), "・1m = %.2fu  |  ■10m×10m×10m 1辺 = %.1fu", unitPerMeter, cubeSize);
    DrawStringToHandle(lx, ty, scaleBuf2, GetColor(100, 255, 230), font13);
    ty += 17;

    char modeBuf[112];
    snprintf(modeBuf, sizeof(modeBuf), "・[7] ■配置: %s", cubeModeNames[m_debugCubeMode % 4]);
    DrawStringToHandle(lx, ty, modeBuf, GetColor(255, 240, 130), font13);
    ty += 18;

    char posBuf[112];
    snprintf(posBuf, sizeof(posBuf), "・現在座標: X=%.1f, Y=%.1f, Z=%.1f (CSV用 %d:%d:%d)",
             pPos.x, pPos.y, pPos.z,
             static_cast<int>(std::round(pPos.x)),
             static_cast<int>(std::round(pPos.y)),
             static_cast<int>(std::round(pPos.z)));
    DrawStringToHandle(lx, ty, posBuf, GetColor(220, 240, 255), font13);
    ty += 18;

    VECTOR minB = stage.GetMinBounds();
    VECTOR maxB = stage.GetMaxBounds();
    float mapW_U = maxB.x - minB.x;
    float mapD_U = maxB.z - minB.z;
    float mapW_M = mapW_U / unitPerMeter;
    float mapD_M = mapD_U / unitPerMeter;
    const char* mapName = (m_selectedMap == ModelConfig::MapType::NewMap) ? "Map2(NewMap)" : "Map1(House)";

    char stageBuf[112];
    snprintf(stageBuf, sizeof(stageBuf), "・%s: %.0f×%.0fu (約 %.1fm × %.1fm) 壁:%zu枚",
             mapName, mapW_U, mapD_U, mapW_M, mapD_M, stage.GetWallPolygons().size());
    DrawStringToHandle(lx, ty, stageBuf, GetColor(190, 220, 250), font13);
    ty += 20;

    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (C) ネズミ(Mouse) CSV設定＆個体別スピード・距離一覧 ---
    DrawStringToHandle(lx, ty, "【ネズミ速度 (Statu.csv) & 生存個体との距離】", GetColor(255, 230, 80), font13);
    ty += 18;

    const auto& mouseCsv = CsvManager::GetInstance().GetMouseStatus();
    {
        char mCsvBuf[96];
        snprintf(mCsvBuf, sizeof(mCsvBuf), "・Statu.csv[Mouse]: 初速%.2f / +%.2f(1匹) / 上限%.2f",
                 mouseCsv.baseSpeed, mouseCsv.speedGrowthPerUnit, mouseCsv.maxSpeed);
        DrawStringToHandle(lx, ty, mCsvBuf, GetColor(200, 220, 240), font13);
        ty += 17;
    }

    int mouseIdx = 1;
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (!mouse) continue;

            VECTOR mPos = mouse->GetPos();
            float dx = mPos.x - pPos.x;
            float dy = mPos.y - pPos.y;
            float dz = mPos.z - pPos.z;
            float distU = std::sqrt(dx * dx + dy * dy + dz * dz);
            float distM = distU / unitPerMeter;
            float mSpd  = mouse->GetCurrentSpeed();
            float mSpdM = (mSpd * 60.0f) / unitPerMeter;
            bool isFast = (obj->GetType() == ObjectType::FastMouse);

            char mLine[112];
            snprintf(mLine, sizeof(mLine), " #%d [%s] 速度:%.2fu/f(%.1fm/s) | 距離:%.1fm(%.0fu)%s",
                     mouseIdx,
                     isFast ? "Fast" : "Norm",
                     mSpd, mSpdM, distM, distU,
                     mouse->IsStunned() ? " [スタン]" : "");
            DrawStringToHandle(lx, ty, mLine, isFast ? GetColor(255, 150, 150) : GetColor(240, 230, 160), font13);
            ty += 16;
            mouseIdx++;
            if (ty > panelY + panelH - 18) break;
        }
    }

    // =========================================================================
    // 3. 右上：2Dトップダウン・ミニマップ（壁形状・10mグリッド・Pos1〜8・ネズミ・10m×10m枠）
    // =========================================================================
    int mapBoxW = 345;
    int mapBoxH = 265;
    int mapBoxX = Config::SCREEN_WIDTH - mapBoxW - 16;
    int mapBoxY = 38;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(mapBoxX, mapBoxY, mapBoxX + mapBoxW, mapBoxY + mapBoxH, GetColor(10, 16, 28), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(mapBoxX, mapBoxY, mapBoxX + mapBoxW, mapBoxY + mapBoxH, GetColor(0, 220, 200), FALSE);

    DrawStringToHandle(mapBoxX + 10, mapBoxY + 6, "■ 2D マップ & 10m×10m レーダー", GetColor(80, 255, 230), font16);

    int drawAreaX = mapBoxX + 14;
    int drawAreaY = mapBoxY + 30;
    int drawAreaW = mapBoxW - 28;
    int drawAreaH = mapBoxH - 54;

    DrawBox(drawAreaX, drawAreaY, drawAreaX + drawAreaW, drawAreaY + drawAreaH, GetColor(6, 10, 18), TRUE);

    float spanX = (maxB.x - minB.x);
    float spanZ = (maxB.z - minB.z);
    if (spanX < 100.0f) spanX = 1000.0f;
    if (spanZ < 100.0f) spanZ = 1000.0f;

    float scaleX = static_cast<float>(drawAreaW - 12) / spanX;
    float scaleZ = static_cast<float>(drawAreaH - 12) / spanZ;
    float mapScale = (scaleX < scaleZ) ? scaleX : scaleZ;

    float centerWorldX = (minB.x + maxB.x) * 0.5f;
    float centerWorldZ = (minB.z + maxB.z) * 0.5f;
    int centerScreenX = drawAreaX + drawAreaW / 2;
    int centerScreenY = drawAreaY + drawAreaH / 2;

    auto worldToMinimap = [&](float wx, float wz, int& outSx, int& outSy)
    {
        outSx = centerScreenX + static_cast<int>((wx - centerWorldX) * mapScale);
        // 3D空間の+Zが上になるようにY座標を反転
        outSy = centerScreenY - static_cast<int>((wz - centerWorldZ) * mapScale);
    };

    // ミニマップ上の10mグリッド描画
    int minIdxX = static_cast<int>(std::floor(minB.x / cubeSize));
    int maxIdxX = static_cast<int>(std::ceil(maxB.x / cubeSize));
    int minIdxZ = static_cast<int>(std::floor(minB.z / cubeSize));
    int maxIdxZ = static_cast<int>(std::ceil(maxB.z / cubeSize));

    for (int ix = minIdxX; ix <= maxIdxX; ++ix)
    {
        float wx = static_cast<float>(ix) * cubeSize;
        if (wx < minB.x || wx > maxB.x) continue;
        int sx0, sy0, sx1, sy1;
        worldToMinimap(wx, minB.z, sx0, sy0);
        worldToMinimap(wx, maxB.z, sx1, sy1);
        DrawLine(sx0, sy0, sx1, sy1, (ix == 0) ? GetColor(60, 120, 150) : GetColor(28, 52, 70));
    }
    for (int iz = minIdxZ; iz <= maxIdxZ; ++iz)
    {
        float wz = static_cast<float>(iz) * cubeSize;
        if (wz < minB.z || wz > maxB.z) continue;
        int sx0, sy0, sx1, sy1;
        worldToMinimap(minB.x, wz, sx0, sy0);
        worldToMinimap(maxB.x, wz, sx1, sy1);
        DrawLine(sx0, sy0, sx1, sy1, (iz == 0) ? GetColor(60, 120, 150) : GetColor(28, 52, 70));
    }

    // ステージ外周バウンディングボックス
    int bMinX, bMinY, bMaxX, bMaxY;
    worldToMinimap(minB.x, maxB.z, bMinX, bMinY);
    worldToMinimap(maxB.x, minB.z, bMaxX, bMaxY);
    DrawBox(bMinX, bMinY, bMaxX, bMaxY, GetColor(70, 110, 150), FALSE);

    // ステージ壁ポリゴンの描画
    unsigned int wallMapCol = GetColor(90, 165, 220);
    for (const auto& wp : stage.GetWallPolygons())
    {
        int x0, y0, x1, y1;
        worldToMinimap(wp.v0.x, wp.v0.z, x0, y0);
        worldToMinimap(wp.v1.x, wp.v1.z, x1, y1);
        DrawLine(x0, y0, x1, y1, wallMapCol);
    }

    // Position.csv (Pos1〜Pos8) のミニマップピン
    for (size_t i = 0; i < csvPositions.size(); ++i)
    {
        int px, py;
        worldToMinimap(csvPositions[i].x, csvPositions[i].z, px, py);
        DrawCircle(px, py, 3, GetColor(80, 230, 100), TRUE);
        char pNum[8];
        snprintf(pNum, sizeof(pNum), "P%zu", i + 1);
        DrawStringToHandle(px + 4, py - 6, pNum, GetColor(130, 255, 140), font13);
    }

    // ミニマップ上の10m×10mキューブ領域（シアン枠）
    if (m_debugCubeMode != 3)
    {
        int cx0, cy0, cx1, cy1;
        worldToMinimap(cubeCenter.x - halfCube, cubeCenter.z + halfCube, cx0, cy0);
        worldToMinimap(cubeCenter.x + halfCube, cubeCenter.z - halfCube, cx1, cy1);
        DrawBox(cx0, cy0, cx1, cy1, GetColor(0, 255, 220), FALSE);
    }

    // 生存ネズミのミニマップ位置
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            VECTOR mPos = obj->GetPos();
            int mx, my;
            worldToMinimap(mPos.x, mPos.z, mx, my);
            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            DrawCircle(mx, my, 4, isFast ? GetColor(255, 70, 70) : GetColor(255, 210, 60), TRUE);
        }
    }

    // プレイヤー(猫)のミニマップ位置と向きベクトル
    int pMapX, pMapY;
    worldToMinimap(pPos.x, pPos.z, pMapX, pMapY);
    float rotY = player->GetRotY();
    int dirX = pMapX + static_cast<int>(std::sin(rotY) * 12.0f);
    int dirY = pMapY - static_cast<int>(std::cos(rotY) * 12.0f);
    DrawLine(pMapX, pMapY, dirX, dirY, GetColor(255, 255, 255));
    DrawCircle(pMapX, pMapY, 5, GetColor(0, 255, 180), TRUE);
    DrawCircle(pMapX, pMapY, 5, GetColor(255, 255, 255), FALSE);

    DrawBox(drawAreaX, drawAreaY, drawAreaX + drawAreaW, drawAreaY + drawAreaH, GetColor(60, 100, 130), FALSE);
    DrawStringToHandle(mapBoxX + 12, mapBoxY + mapBoxH - 20, "緑:猫  水色枠:10m×10m  赤/黄:鼠  P1-8:CSV座標", GetColor(170, 205, 230), font13);

    // =========================================================================
    // 4. 右側中段：F1 デバッグ＆チートショートカット操作パネル
    // =========================================================================
    int cheatW = 345;
    int cheatH = 212;
    int cheatX = Config::SCREEN_WIDTH - cheatW - 16;
    int cheatY = mapBoxY + mapBoxH + 8;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(cheatX, cheatY, cheatX + cheatW, cheatY + cheatH, GetColor(15, 18, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(cheatX, cheatY, cheatX + cheatW, cheatY + cheatH, GetColor(255, 220, 40), FALSE);

    int cy = cheatY + 8;
    DrawStringToHandle(cheatX + 10, cy, "★ F1 デバッグ＆目測・チート操作", GetColor(255, 240, 60), font16);
    cy += 23;
    DrawStringToHandle(cheatX + 12, cy, "[1] 通常ネズミ +1体追加スポーン", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[2] 高速ネズミ +1体追加スポーン", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[3] パンプアップ Lv +1 (速度・身長UP)", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[4] ネズミ時間停止: %s", m_objManager.IsTimeFrozen() ? "ON" : "OFF");
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[5] スキルCT無効  : %s", player->IsNoCooldown() ? "ON" : "OFF");
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[6] エフェクト表示: %s", EffectManager::GetInstance().IsEffectEnabled() ? "ON" : "OFF");
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[7] 10m×10m×10m ■表示モード切替", GetColor(100, 255, 220), font13);
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(255, 220, 100), font13,
                             "[8] 10m全体目測・広角カメラ: %s", m_debugWideCamera ? "ON (俯瞰)" : "OFF (通常)");
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[F1] デバッグオーバーレイ ON/OFF", GetColor(180, 195, 215), font13);
}
