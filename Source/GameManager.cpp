#include "GameManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "CustomEffectManager.h"
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
            float groundY = 0.0f;
            float gy = (m_selectedMap == ModelConfig::MapType::House) ? 5.0f : 200.0f;
            if (m_selectedMap == ModelConfig::MapType::House) m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), gy);
            player->SetPos(VGet(0.0f, gy, 0.0f));
            player->SetGroundY(gy);
            if (false)
            {
                // unused
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
    CustomEffectManager::GetInstance().Clear();

    // 選択中マップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);

    // タイトル画面：マッチョな猫を生成（Rep=6の堂々たる姿）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    player->AddRep(6); // Lv.6のマッチョ体格
    player->SetMapType(m_selectedMap);

    // Map1は床(Y≈5)、Map2は中央の円柱台座上(Y=200)に配置
    float groundY = (m_selectedMap == ModelConfig::MapType::House) ? 5.0f : 200.0f;
    if (m_selectedMap == ModelConfig::MapType::House) m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), groundY);
    player->SetPos(VGet(0.0f, groundY, 0.0f));
    player->SetGroundY(groundY);
    if (false)
    {
        // unused
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
    CustomEffectManager::GetInstance().Clear();
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
    m_remainingTimeSeconds = Config::TIME_LIMIT_SECONDS;
    m_isTimeUp = false;
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
        VECTOR catPos = (player) ? player->GetPos() : VGet(0.0f, 0.0f, 0.0f);
        VECTOR center = VGet(catPos.x, catY + 20.0f, catPos.z);
        VECTOR camPos = VGet(
            catPos.x + std::sin(m_titleCameraAngle) * camDist,
            camHeight,
            catPos.z + std::cos(m_titleCameraAngle) * camDist
        );
        SetCameraPositionAndTarget_UpVecY(camPos, center);

        // 猫のアニメーション：定期的にスクワット運動（タイトル画面ではエフェクトは出さない）
        if (player)
        {
            // 2.5秒ごとにスクワットと待機を切り替え
            bool isSquatting = ((m_titleAnimTimer / 140) % 2 == 0);
            player->UpdateTitleAnimation(isSquatting);
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
            if (mx < btnX + 60)
            {
                SwitchSelectedMap(ModelConfig::MapType::House);
            }
            else if (mx > btnX + btnW - 60)
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
                CustomEffectManager::GetInstance().SetEffectEnabled(EffectManager::GetInstance().IsEffectEnabled());
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

        // クリア判定・制限時間（120秒）タイムアップ判定
        float elapsedSec = (GetNowCount() - m_startCount) / 1000.0f;
        float remainSec  = Config::TIME_LIMIT_SECONDS - elapsedSec;
        if (remainSec < 0.0f) remainSec = 0.0f;

        if (m_objManager.GetRemainingMouseCount() == 0 || (Config::USE_TIME_LIMIT_MODE && remainSec <= 0.0f))
        {
            m_state = GameState::GameClear;
            m_clearCount = GetNowCount();
            m_clearTimeSeconds = elapsedSec;
            m_remainingTimeSeconds = remainSec;
            m_isTimeUp = (Config::USE_TIME_LIMIT_MODE && m_objManager.GetRemainingMouseCount() > 0 && remainSec <= 0.0f);
            m_clearAnimTimer = 0;
            m_clearSelectedBtn = 0;
            m_clearCamStartPos = m_camera.GetCameraPos();
            m_clearCamStartTarget = m_camera.GetTargetPos();
            m_prevKeyEnter = true;
            m_prevKeySpace = true;
            m_prevKeyR = true;
            m_prevKeyT = true;
            m_prevKeyM = true;
            m_prevMouseLeft = true;
        }
    }
    
    else if (m_state == GameState::GameClear)
    {
        SetMouseDispFlag(TRUE); // クリア画面ではカーソル表示
        m_clearAnimTimer++;

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            // 背景で猫がバーベルを持って筋トレ（スクワット）し続ける演出
            player->UpdateTitleAnimation(true);

            VECTOR pPos = player->GetPos();
            float pRot = player->GetRotY();

            // 猫の斜め前方へ回り込みながらズームインし、全身（頭から足元・バーベルまで）が綺麗に収まる距離に引く
            int orbitFrames = (m_clearAnimTimer > CLEAR_ZOOM_DURATION) ? (m_clearAnimTimer - CLEAR_ZOOM_DURATION) : 0;
            float frontAngle = pRot + MathHelper::PI * 0.82f + std::sin(orbitFrames * 0.012f) * 0.22f;
            float effRep = static_cast<float>(player->GetEffectiveRep());
            float zoomDist = 235.0f + effRep * 3.0f;   // 少し引いて全身・バーベルが見切れない距離
            float zoomHeight = 68.0f + effRep * 1.2f;  // 猫の胸〜顔の高さに合わせてカメラ高さを調整

            VECTOR goalCamPos = VGet(
                pPos.x - std::sin(frontAngle) * zoomDist,
                pPos.y + zoomHeight,
                pPos.z - std::cos(frontAngle) * zoomDist
            );
            VECTOR goalTarget = VGet(pPos.x, pPos.y + 52.0f + effRep * 1.0f, pPos.z); // 猫の体の中央（胸付近）を注視

            // ステージ壁による遮蔽チェック（猫とカメラの間に壁がある場合は手前に寄せる）
            int stageModel = m_objManager.GetStage().GetModelHandle();
            if (stageModel != -1)
            {
                MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageModel, -1, goalTarget, goalCamPos);
                if (hit.HitFlag == 1 && std::abs(hit.Normal.y) < 0.85f)
                {
                    VECTOR dir = VSub( hit.HitPosition, goalTarget );
                    float len = VSize(dir);
                    if (len > 38.0f)
                    {
                        goalCamPos = VAdd(goalTarget, VScale(VNorm(dir), len - 10.0f));
                    }
                }
            }

            // イーズアウト補間でクリア直後のカメラ位置から猫へ滑らかにズームイン
            float t = MathHelper::Clamp(static_cast<float>(m_clearAnimTimer) / static_cast<float>(CLEAR_ZOOM_DURATION), 0.0f, 1.0f);
            float smoothT = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

            VECTOR curCamPos = VGet(
                MathHelper::Lerp(m_clearCamStartPos.x, goalCamPos.x, smoothT),
                MathHelper::Lerp(m_clearCamStartPos.y, goalCamPos.y, smoothT),
                MathHelper::Lerp(m_clearCamStartPos.z, goalCamPos.z, smoothT)
            );
            VECTOR curTarget = VGet(
                MathHelper::Lerp(m_clearCamStartTarget.x, goalTarget.x, smoothT),
                MathHelper::Lerp(m_clearCamStartTarget.y, goalTarget.y, smoothT),
                MathHelper::Lerp(m_clearCamStartTarget.z, goalTarget.z, smoothT)
            );

            SetCameraNearFar(5.0f, 90000.0f);
            SetCameraPositionAndTarget_UpVecY(curCamPos, curTarget);

            // 筋トレ燃焼オーラエフェクトも継続更新
            CustomEffectManager::GetInstance().UpdateWorkoutFlame(pPos, true, player->GetRepCount(), 0.7f, true);
            EffectManager::GetInstance().UpdateMuscleAura(pPos, true, player->GetRepCount());
        }

        // マウス入力
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
        bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
        m_prevMouseLeft = isLeftClick;

        // キー入力
        bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
        bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0 || CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
        bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        bool keyR     = (CheckHitKey(KEY_INPUT_R) != 0);
        bool keyT     = (CheckHitKey(KEY_INPUT_T) != 0);
        bool keyM     = (CheckHitKey(KEY_INPUT_M) != 0);

        bool triggerLeft  = keyLeft && !m_prevKeyLeft;
        bool triggerRight = keyRight && !m_prevKeyRight;
        bool triggerEnter = isEnter && !m_prevKeyEnter;
        bool triggerSpace = isSpace && !m_prevKeySpace;
        bool triggerR     = keyR && !m_prevKeyR;
        bool triggerT     = keyT && !m_prevKeyT;
        bool triggerM     = keyM && !m_prevKeyM;

        m_prevKeyLeft  = keyLeft;
        m_prevKeyRight = keyRight;
        m_prevKeyEnter = isEnter;
        m_prevKeySpace = isSpace;
        m_prevKeyR     = keyR;
        m_prevKeyT     = keyT;
        m_prevKeyM     = keyM;

        // ズームイン演出中（CLEAR_ZOOM_DURATION フレームまで）はリザルトUIのボタン操作を待機（エフェクト更新は止めない）
        if (m_clearAnimTimer > CLEAR_ZOOM_DURATION)
        {
            if (triggerLeft)  m_clearSelectedBtn = 0;
            if (triggerRight) m_clearSelectedBtn = 1;

            // アクションボタン当たり判定座標
            int btnW = 340;
            int btnH = 66;
            int btnGap = 50;
            int totalBtnsW = btnW * 2 + btnGap;
            int btn1X = (Config::SCREEN_WIDTH - totalBtnsW) / 2;
            int btn2X = btn1X + btnW + btnGap;
            int btnY  = 618;

            bool hoverRetry = (mx >= btn1X && mx <= btn1X + btnW && my >= btnY && my <= btnY + btnH);
            bool hoverMenu  = (mx >= btn2X && mx <= btn2X + btnW && my >= btnY && my <= btnY + btnH);

            if (hoverRetry) m_clearSelectedBtn = 0;
            if (hoverMenu)  m_clearSelectedBtn = 1;

            // リトライ判定（[R]キー、または RETRY ボタンクリック / 選択決定）
            if (triggerR || (hoverRetry && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_clearSelectedBtn == 0))
            {
                StartGame();
                return;
            }

            // メインメニュー（タイトル）判定（[M]/[T]キー、または MAIN MENU ボタンクリック / 選択決定）
            if (triggerM || triggerT || (hoverMenu && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_clearSelectedBtn == 1))
            {
                SetupTitle();
                return;
            }
        }
    }

    // 3Dエフェクトの更新（タイトル画面・ポーズ中は更新しない）
    if (m_state != GameState::Title && !m_isPaused)
    {
        EffectManager::GetInstance().Update();
        CustomEffectManager::GetInstance().Update();
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

    // Effekseer 3Dエフェクトおよびカスタム燃焼エフェクト描画（タイトル画面では非表示）
    if (m_state != GameState::Title)
    {
        EffectManager::GetInstance().SyncCamera();
        EffectManager::GetInstance().Draw3D();
        CustomEffectManager::GetInstance().Draw3D();
    }

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
        // =========================================================================
        // ゲームプレイHUD（背景を邪魔しない、斜めカッティングのコミック・アクションHUD）
        // =========================================================================
        unsigned int colYellow  = GetColor(255, 230, 0);
        unsigned int colOrange  = GetColor(255, 135, 0);
        unsigned int colBlack   = GetColor(0, 0, 0);
        unsigned int colWhite   = GetColor(255, 255, 255);
        unsigned int colGold    = GetColor(255, 205, 0);
        unsigned int colNeonRed = GetColor(245, 25, 65);
        unsigned int colDark    = GetColor(20, 23, 34);

        // 斜め平行四辺形プレート描画ヘルパー（外枠＋ハードシャドウ付き）
        auto DrawSlantedPlate = [&](int x, int y, int w, int h, int skew, int shadowOff, unsigned int fillCol, unsigned int borderCol, int borderThick)
        {
            auto FillQuad = [](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int col)
            {
                DrawTriangle(x1, y1, x2, y2, x3, y3, col, TRUE);
                DrawTriangle(x1, y1, x3, y3, x4, y4, col, TRUE);
            };
            // ハードシャドウ
            if (shadowOff > 0)
            {
                FillQuad(x + skew + shadowOff, y + shadowOff,
                         x + w + skew + shadowOff, y + shadowOff,
                         x + w + shadowOff, y + h + shadowOff,
                         x + shadowOff, y + h + shadowOff, colBlack);
            }
            // ブラック極太外枠
            if (borderThick > 0)
            {
                int b = borderThick;
                FillQuad(x + skew - b, y - b,
                         x + w + skew + b, y - b,
                         x + w + b, y + h + b,
                         x - b, y + h + b, colBlack);
            }
            // アクセント内枠
            if (borderCol != colBlack)
            {
                FillQuad(x + skew - 2, y - 2,
                         x + w + skew + 2, y - 2,
                         x + w + 2, y + h + 2,
                         x - 2, y + h + 2, borderCol);
            }
            // 本体塗り
            FillQuad(x + skew, y,
                     x + w + skew, y,
                     x + w, y + h,
                     x, y + h, fillCol);
        };

        // 黒縁取りテキスト描画ヘルパー
        auto DrawBoldText = [&](int x, int y, const char* str, unsigned int col, int fontHandle, int outline = 2)
        {
            for (int dy = -outline; dy <= outline; ++dy)
            {
                for (int dx = -outline; dx <= outline; ++dx)
                {
                    if (dx != 0 || dy != 0)
                    {
                        DrawStringToHandle(x + dx, y + dy, str, colBlack, fontHandle);
                    }
                }
            }
            DrawStringToHandle(x + outline, y + outline, str, colBlack, fontHandle);
            DrawStringToHandle(x, y, str, col, fontHandle);
        };

        float currentSec = (m_isPaused ? (m_pauseStartTime - m_startCount) : (GetNowCount() - m_startCount)) / 1000.0f;
        auto player = m_objManager.GetPlayer();
        int reps = player ? player->GetRepCount() : 0;
        MuscleState ms = player ? player->GetMuscleState() : MuscleState::Normal;
        bool isGod = (reps >= Player3D::MAX_EFFECTIVE_REP);

        // =========================================================================
        // 1. 画面上部中央：アーケード風タイムカウンター（コンテナ枠なしでスッキリ配置）
        // =========================================================================
        {
            float displaySec = currentSec;
            bool isUrgent = false;
            if (Config::USE_TIME_LIMIT_MODE)
            {
                displaySec = Config::TIME_LIMIT_SECONDS - currentSec;
                if (displaySec < 0.0f) displaySec = 0.0f;
                isUrgent = (displaySec <= 20.0f);
            }

            int tPlateW = 164;
            int tPlateH = 54;
            int tPlateX = (Config::SCREEN_WIDTH - tPlateW) / 2;
            int tPlateY = 18;

            DrawSlantedPlate(tPlateX, tPlateY, tPlateW, tPlateH, 12, 5, colDark, isUrgent ? colNeonRed : colYellow, 4);

            // 上部ミニラベルプレート
            int lblW = 86;
            int lblX = (Config::SCREEN_WIDTH - lblW) / 2 + 4;
            DrawSlantedPlate(lblX, tPlateY - 10, lblW, 18, 6, 2, isUrgent ? colNeonRed : colYellow, colBlack, 2);
            const char* timeLbl = Config::USE_TIME_LIMIT_MODE ? "TIME LEFT" : "TIME";
            int tlW = GetDrawStringWidthToHandle(timeLbl, static_cast<int>(strlen(timeLbl)), font13);
            DrawStringToHandle(lblX + 3 + (lblW - tlW) / 2, tPlateY - 8, timeLbl, isUrgent ? colWhite : colBlack, font13);

            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "%.1f", displaySec);
            int tw = GetDrawStringWidthToHandle(timeBuf, static_cast<int>(strlen(timeBuf)), font36);
            unsigned int tCol = (isUrgent && (GetNowCount() / 150) % 2 == 0) ? colNeonRed : colWhite;
            DrawBoldText(tPlateX + 6 + (tPlateW - tw) / 2, tPlateY + 11, timeBuf, tCol, font36, 2);
        }

        // =========================================================================
        // 2. 画面左上：マッスルレベル＆減衰ゲージ ＋ 残りネズミ（独立した斜めプレート）
        // =========================================================================
        {
            // A. マッスルレベル・斜めスタンプ
            int lvX = 22;
            int lvY = 20;
            int lvW = 96;
            int lvH = 62;
            unsigned int lvAccent = isGod ? colNeonRed : (reps >= 4 ? colYellow : colOrange);

            DrawSlantedPlate(lvX, lvY, lvW, lvH, 12, 6, lvAccent, colBlack, 4);
            DrawStringToHandle(lvX + 18, lvY + 5, isGod ? "MAX LV" : "MUSCLE", colBlack, font13);

            char lvStr[16];
            snprintf(lvStr, sizeof(lvStr), "Lv.%d", reps);
            int lvw = GetDrawStringWidthToHandle(lvStr, static_cast<int>(strlen(lvStr)), font24);
            DrawBoldText(lvX + 4 + (lvW - lvw) / 2, lvY + 24, lvStr, colWhite, font24, 2);

            // B. レベルスタンプ右側から伸びる斜め減衰ゲージバー
            int barX = lvX + lvW + 10;
            int barY = lvY + 8;
            int barW = 190;
            int barH = 22;

            DrawSlantedPlate(barX, barY, barW, barH, 10, 4, colDark, colBlack, 3);

            if (ms == MuscleState::Soreness)
            {
                DrawSlantedPlate(barX + 2, barY + 2, barW - 4, barH - 4, 8, 0, colNeonRed, colBlack, 0);
                DrawBoldText(barX + 48, barY + 3, "筋肉痛で停止中!", colWhite, font13, 1);
            }
            else if (player && reps > 0)
            {
                float decaySec = player->GetPumpDecayRemainingSeconds();
                float ratio = MathHelper::Clamp(decaySec / 15.0f, 0.0f, 1.0f);
                int fillW = static_cast<int>((barW - 6) * ratio);
                if (fillW > 4)
                {
                    unsigned int gCol = (decaySec <= 4.0f && (GetNowCount() / 150) % 2 == 0) ? colNeonRed : colYellow;
                    DrawSlantedPlate(barX + 3, barY + 3, fillW, barH - 6, 8, 0, gCol, colBlack, 0);
                }
                char decBuf[32];
                snprintf(decBuf, sizeof(decBuf), "KEEP %.1fs", decaySec);
                DrawBoldText(barX + 14, barY + 4, decBuf, colWhite, font13, 1);
            }
            else
            {
                DrawStringToHandle(barX + 16, barY + 4, "SPACE長押しで筋トレ", GetColor(160, 175, 200), font13);
            }

            // C. 残りネズミプレート（ゲージの下に斜め配置）
            int mX = barX - 4;
            int mY = barY + barH + 10;
            int mW = 165;
            int mH = 32;
            DrawSlantedPlate(mX, mY, mW, mH, 8, 4, colDark, colOrange, 3);

            int remainMice = m_objManager.GetRemainingMouseCount();
            DrawStringToHandle(mX + 14, mY + 9, "残りネズミ", colYellow, font13);

            char mBuf[24];
            snprintf(mBuf, sizeof(mBuf), "%d匹", remainMice);
            int mw = GetDrawStringWidthToHandle(mBuf, static_cast<int>(strlen(mBuf)), font24);
            DrawBoldText(mX + mW - mw - 12, mY + 3, mBuf, colWhite, font24, 2);
        }

        // =========================================================================
        // 3. 画面右下：斜めカットのアクションスキルバッジ（外枠コンテナを廃止）
        // =========================================================================
        {
            auto DrawSlantedSkill = [&](int sx, int sy, int sw, int sh, const char* keyStr, const char* nameStr, bool isReady, unsigned int readyCol)
            {
                unsigned int bgCol = isReady ? colDark : GetColor(16, 18, 24);
                unsigned int brdCol = isReady ? readyCol : GetColor(70, 75, 90);
                DrawSlantedPlate(sx, sy, sw, sh, 10, 5, bgCol, brdCol, 3);

                // 左上に重なる小さな斜めキーキャップ
                int kw = GetDrawStringWidthToHandle(keyStr, static_cast<int>(strlen(keyStr)), font13) + 16;
                int kh = 18;
                DrawSlantedPlate(sx + 6, sy - 10, kw, kh, 5, 2, isReady ? readyCol : GetColor(60, 65, 80), colBlack, 2);
                DrawStringToHandle(sx + 14, sy - 8, keyStr, isReady ? colBlack : GetColor(170, 175, 190), font13);

                // スキル名
                int nw = GetDrawStringWidthToHandle(nameStr, static_cast<int>(strlen(nameStr)), font16);
                int nx = sx + 4 + (sw - nw) / 2;
                int ny = sy + (sh - 16) / 2 + 2;
                if (isReady)
                {
                    DrawBoldText(nx, ny, nameStr, colWhite, font16, 2);
                }
                else
                {
                    DrawStringToHandle(nx, ny, nameStr, GetColor(115, 125, 145), font16);
                }
            };

            int sY = Config::SCREEN_HEIGHT - 74;
            int sH = 38;
            int sW = 148;
            int gap = 14;
            int startX = Config::SCREEN_WIDTH - (sW * 3 + gap * 2) - 28;

            float pounceCd = player ? static_cast<float>(player->GetPounceCooldown()) / 60.0f : 0.0f;
            float tackleCd = player ? static_cast<float>(player->GetTackleCooldown()) / 60.0f : 0.0f;

            char pStr[32], tStr[32];
            if (reps >= 4)
            {
                if (pounceCd > 0.05f) snprintf(pStr, sizeof(pStr), "飛びつき %.1fs", pounceCd);
                else                  snprintf(pStr, sizeof(pStr), "飛びつき");

                if (tackleCd > 0.05f) snprintf(tStr, sizeof(tStr), "スタン %.1fs", tackleCd);
                else                  snprintf(tStr, sizeof(tStr), "スタン衝撃波");

                DrawSlantedSkill(startX,                sY, sW, sH, "Lクリック", pStr, (pounceCd <= 0.05f), colYellow);
                DrawSlantedSkill(startX + sW + gap,     sY, sW, sH, "Rクリック", tStr, (tackleCd <= 0.05f), colOrange);
            }
            else
            {
                DrawSlantedSkill(startX,                sY, sW, sH, "Lクリック", "飛びつき (Lv.4)", false, colYellow);
                DrawSlantedSkill(startX + sW + gap,     sY, sW, sH, "Rクリック", "スタン (Lv.4)", false, colOrange);
            }
            DrawSlantedSkill(startX + (sW + gap) * 2, sY, sW, sH, "SPACE", "筋トレ", true, colGold);

            // 下部ガイドは背景ボックスなしでスッキリ縁取り文字のみ
            const char* ctrlHint = "[WASD] 移動   [マウス] カメラ   [F] 視点リセット   [ESC] ポーズ";
            int chW = GetDrawStringWidthToHandle(ctrlHint, static_cast<int>(strlen(ctrlHint)), font13);
            DrawBoldText(Config::SCREEN_WIDTH - chW - 24, Config::SCREEN_HEIGHT - 24, ctrlHint, GetColor(220, 230, 245), font13, 1);
        }

        // =========================================================================
        // 4. F1 デバッグ情報＆ミニマップ＆10m距離目測オーバーレイ
        // =========================================================================
        if (m_cheatEnabled)
        {
            DrawDebug2DOverlay();
        }
        else
        {
            const char* f1Hint = "[F1] デバッグ・マップ表示";
            int f1W = GetDrawStringWidthToHandle(f1Hint, static_cast<int>(strlen(f1Hint)), font13);
            DrawBoldText(Config::SCREEN_WIDTH - f1W - 20, 18, f1Hint, GetColor(200, 215, 235), font13, 1);
        }

        // プレイ中の感度変更通知ポップアップ
        if (m_sensitivityNoticeTimer > 0 && !m_isPaused)
        {
            float alphaRatio = (m_sensitivityNoticeTimer < 20) ? (m_sensitivityNoticeTimer / 20.0f) : 1.0f;
            int alpha = static_cast<int>(240 * alphaRatio);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

            int notW = 240;
            int notH = 36;
            int notX = (Config::SCREEN_WIDTH - notW) / 2;
            int notY = 86;

            DrawSlantedPlate(notX, notY, notW, notH, 8, 4, colDark, colYellow, 3);

            char notBuf[64];
            snprintf(notBuf, sizeof(notBuf), "マウス感度: %.1fx", m_camera.GetMouseSensitivity());
            int nW = GetDrawStringWidthToHandle(notBuf, static_cast<int>(strlen(notBuf)), font16);
            DrawBoldText((Config::SCREEN_WIDTH - nW) / 2 + 4, notY + 10, notBuf, colYellow, font16, 1);

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
        DrawGameClearScreen();
    }

    if (CheckHitKey(KEY_INPUT_F12))
    {
        SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "d:\\MuscleCat\\screen_shot.png");
    }
}

// ============================================================================
// ステージクリア（リザルト）画面の描画（アメコミ・カートゥーン風ポップUIデザイン）
// ============================================================================
void GameManager::DrawGameClearScreen()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font36 = fm.GetFont36();
    int font48 = fm.GetFont48();
    int font60 = fm.GetFont60();
    int font96 = fm.GetFont96();

    // カラーパレット定義（イエロー、オレンジ、ブラック、ホワイト＋ゴールド＆ネオンレッド）
    unsigned int colYellow  = GetColor(255, 235, 0);   // #FFEB00
    unsigned int colOrange  = GetColor(255, 140, 0);   // #FF8C00
    unsigned int colBlack   = GetColor(0, 0, 0);       // #000000
    unsigned int colWhite   = GetColor(255, 255, 255); // #FFFFFF
    unsigned int colGold    = GetColor(255, 215, 0);   // #FFD700
    unsigned int colNeonRed = GetColor(255, 0, 68);    // #FF0044

    float animSec = m_clearAnimTimer / 60.0f;

    // =========================================================================
    // ズームイン演出中（開始〜CLEAR_ZOOM_DURATION フレーム）：
    // まず猫が筋トレ（スクワット）している姿へカメラがズームし、その後リザルトUIを出す
    // =========================================================================
    if (m_clearAnimTimer <= CLEAR_ZOOM_DURATION)
    {
        // 上下のシネマティック黒帯＆「★ STAGE CLEAR!! ★」ロゴだけ先行表示して猫の筋トレを強調
        float barRatio = MathHelper::Clamp(m_clearAnimTimer / 18.0f, 0.0f, 1.0f);
        int barH = static_cast<int>(90 * barRatio);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(0, 0, Config::SCREEN_WIDTH, barH, colBlack, TRUE);
        DrawBox(0, Config::SCREEN_HEIGHT - barH, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colBlack, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(0, 0, Config::SCREEN_WIDTH, 8, colYellow, TRUE);
        DrawBox(0, Config::SCREEN_HEIGHT - 8, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colYellow, TRUE);

        float headerBounce = std::abs(std::sin(animSec * 5.0f)) * -8.0f;
        int headerY = 20 + static_cast<int>(headerBounce);
        const char* clearLogo = m_isTimeUp ? "TIME UP!" : "STAGE CLEAR!";
        int logoW = GetDrawStringWidthToHandle(clearLogo, static_cast<int>(strlen(clearLogo)), font60);
        int logoX = (Config::SCREEN_WIDTH - logoW) / 2;

        for (int d = 4; d <= 8; ++d)
        {
            DrawStringToHandle(logoX + d, headerY + d, clearLogo, colBlack, font60);
        }
        for (int dy = -4; dy <= 4; ++dy)
        {
            for (int dx = -4; dx <= 4; ++dx)
            {
                if (dx != 0 || dy != 0)
                {
                    DrawStringToHandle(logoX + dx, headerY + dy, clearLogo, colBlack, font60);
                }
            }
        }
        DrawStringToHandle(logoX, headerY + 2, clearLogo, m_isTimeUp ? colNeonRed : colOrange, font60);
        DrawStringToHandle(logoX, headerY,     clearLogo, colYellow, font60);
        DrawStringToHandle(logoX, headerY - 2, clearLogo, GetColor(255, 255, 190), font60);
        return;
    }

    int uiTimer = m_clearAnimTimer - CLEAR_ZOOM_DURATION;
    float uiSec = uiTimer / 60.0f;

    // =========================================================================
    // 0. 背景オーバーレイ（背後の筋トレ中の猫がよく見える軽めの半透明＋ドット＋集中線）
    // =========================================================================
    // ベース半透明ダークレイヤー（背景の筋トレ中の猫がくっきり透けて見えるよう105に調整）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 105);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 12, 22), TRUE);

    // 上下端をさらに引き締めるビネット風グラデーション帯
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
    DrawBox(0, 0, Config::SCREEN_WIDTH, 135, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 120, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colBlack, TRUE);

    // アメコミ風ハーフトーン・ドットパターン（四隅・上下エリアにドット網点を配置）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 38);
    int dotStep = 28;
    for (int y = dotStep / 2; y < Config::SCREEN_HEIGHT; y += dotStep)
    {
        int rowOffset = ((y / dotStep) % 2) * (dotStep / 2);
        for (int x = rowOffset; x < Config::SCREEN_WIDTH; x += dotStep)
        {
            float dx = static_cast<float>(x - Config::SCREEN_WIDTH / 2) / (Config::SCREEN_WIDTH * 0.5f);
            float dy = static_cast<float>(y - Config::SCREEN_HEIGHT / 2) / (Config::SCREEN_HEIGHT * 0.5f);
            float distSq = dx * dx + dy * dy;
            if (distSq > 0.32f)
            {
                int radius = static_cast<int>(MathHelper::Clamp((distSq - 0.25f) * 6.5f, 2.0f, 6.0f));
                DrawCircle(x, y, radius, colOrange, TRUE);
            }
        }
    }

    // 漫画風の放射集中線エフェクト（画面中央から外周へ向かうダイナミックなスピードライン）
    int centerX = Config::SCREEN_WIDTH / 2;
    int centerY = Config::SCREEN_HEIGHT / 2 - 20;
    float rotBase = animSec * 0.18f;
    int numSpeedLines = 28;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
    for (int i = 0; i < numSpeedLines; ++i)
    {
        float baseAngle = rotBase + (MathHelper::PI * 2.0f * i) / numSpeedLines;
        float halfWidth = 0.018f + (i % 3) * 0.008f;
        float innerR = 340.0f + ((i * 37) % 110) + std::sin(animSec * 6.0f + i) * 20.0f;
        float outerR = 920.0f;

        int x1 = centerX + static_cast<int>(std::cos(baseAngle) * innerR);
        int y1 = centerY + static_cast<int>(std::sin(baseAngle) * innerR);
        int x2 = centerX + static_cast<int>(std::cos(baseAngle - halfWidth) * outerR);
        int y2 = centerY + static_cast<int>(std::sin(baseAngle - halfWidth) * outerR);
        int x3 = centerX + static_cast<int>(std::cos(baseAngle + halfWidth) * outerR);
        int y3 = centerY + static_cast<int>(std::sin(baseAngle + halfWidth) * outerR);

        unsigned int rayCol = (i % 2 == 0) ? colYellow : colWhite;
        DrawTriangle(x1, y1, x2, y2, x3, y3, rayCol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // アメコミ風の上下イエロー＆ブラックストライプフレーム
    DrawBox(0, 0, Config::SCREEN_WIDTH, 10, colYellow, TRUE);
    DrawBox(0, 10, Config::SCREEN_WIDTH, 14, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 14, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 10, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 10, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colYellow, TRUE);

    // =========================================================================
    // 1. ヘッダー（上部）：「★ STAGE CLEAR!! ★」立体ロゴ ＆ 大小の星エフェクト
    // =========================================================================
    float headerBounce = std::abs(std::sin(animSec * 4.5f)) * -8.0f;
    int headerY = 28 + static_cast<int>(headerBounce);

    // 背面の斜めスピードバナー帯（アメコミ風ブラック＆ネオンレッド枠）
    int bannerLeft  = 210;
    int bannerRight = Config::SCREEN_WIDTH - 210;
    int bannerTop   = 22;
    int bannerBot   = 112;
    DrawTriangle(bannerLeft - 25, bannerBot + 6, bannerLeft + 15, bannerTop + 6, bannerRight + 25, bannerTop + 6, colBlack, TRUE);
    DrawTriangle(bannerLeft - 25, bannerBot + 6, bannerRight + 25, bannerTop + 6, bannerRight - 15, bannerBot + 6, colBlack, TRUE);

    DrawTriangle(bannerLeft - 20, bannerBot, bannerLeft + 20, bannerTop, bannerRight + 20, bannerTop, colNeonRed, TRUE);
    DrawTriangle(bannerLeft - 20, bannerBot, bannerRight + 20, bannerTop, bannerRight - 20, bannerBot, colNeonRed, TRUE);

    DrawTriangle(bannerLeft - 12, bannerBot - 5, bannerLeft + 24, bannerTop + 5, bannerRight + 12, bannerTop + 5, GetColor(22, 24, 38), TRUE);
    DrawTriangle(bannerLeft - 12, bannerBot - 5, bannerRight + 12, bannerTop + 5, bannerRight - 24, bannerBot - 5, GetColor(22, 24, 38), TRUE);

    const char* clearLogo = m_isTimeUp ? "TIME UP!" : "STAGE CLEAR!";
    int logoW = GetDrawStringWidthToHandle(clearLogo, static_cast<int>(strlen(clearLogo)), font60);
    int logoX = (Config::SCREEN_WIDTH - logoW) / 2;

    // 3D押し出しシャドウ（右下ブラック）
    for (int d = 4; d <= 8; ++d)
    {
        DrawStringToHandle(logoX + d, headerY + d, clearLogo, colBlack, font60);
    }
    // 極太ブラックアウトライン
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(logoX + dx, headerY + dy, clearLogo, colBlack, font60);
            }
        }
    }
    // 内側オレンジ立体層 → メインイエロー → ゴールド＆ホワイトハイライト
    DrawStringToHandle(logoX, headerY + 2, clearLogo, m_isTimeUp ? colNeonRed : colOrange, font60);
    DrawStringToHandle(logoX, headerY,     clearLogo, colYellow, font60);
    DrawStringToHandle(logoX, headerY - 2, clearLogo, GetColor(255, 255, 190), font60);

    // 大小の星エフェクト（パルスしながら周囲に輝く）
    int starPulse1 = static_cast<int>(std::sin(animSec * 6.0f) * 6.0f);
    int starPulse2 = static_cast<int>(std::cos(animSec * 6.0f) * 6.0f);
    DrawStringToHandle(bannerLeft - 68, 30 + starPulse1, "★", colGold, font36);
    DrawStringToHandle(bannerLeft - 28, 72 + starPulse2, "✦", colWhite, font24);
    DrawStringToHandle(bannerLeft - 92, 68 - starPulse1, "★", colOrange, font18);

    DrawStringToHandle(bannerRight + 32, 28 + starPulse2, "★", colGold, font36);
    DrawStringToHandle(bannerRight + 10, 74 + starPulse1, "✦", colWhite, font24);
    DrawStringToHandle(bannerRight + 74, 66 - starPulse2, "★", colOrange, font18);

    // =========================================================================
    // 評価ランク・スコアの算出（制限時間モード：残り時間に応じたランク＆スコア算出）
    // =========================================================================
    bool isStage2 = (m_selectedMap == ModelConfig::MapType::NewMap);
    const char* rankLetter = "C";
    const char* rankTitle  = "ROOKIE CAT";
    unsigned int rankMainCol = GetColor(100, 215, 255);
    unsigned int rankSubCol  = GetColor(30, 130, 200);
    char critBuf[96];

    if (Config::USE_TIME_LIMIT_MODE)
    {
        // 120秒制限時間制：残り時間基準（S: 残り90秒以上[Stage2は60秒] / A: 残り75秒以上[45秒] / B: 残り60秒以上[30秒]）
        float sRemain = isStage2 ? 60.0f : 90.0f;
        float aRemain = isStage2 ? 45.0f : 75.0f;
        float bRemain = isStage2 ? 30.0f : 60.0f;

        if (m_isTimeUp)
        {
            rankLetter  = "C";
            rankTitle   = "TIME UP...";
            rankMainCol = GetColor(160, 175, 200);
            rankSubCol  = GetColor(80, 90, 115);
        }
        else if (m_remainingTimeSeconds >= sRemain)
        {
            rankLetter  = "S";
            rankTitle   = "GOD MUSCLE CAT!!";
            rankMainCol = colGold;
            rankSubCol  = colOrange;
        }
        else if (m_remainingTimeSeconds >= aRemain)
        {
            rankLetter  = "A";
            rankTitle   = "GREAT MUSCLE!";
            rankMainCol = colOrange;
            rankSubCol  = colNeonRed;
        }
        else if (m_remainingTimeSeconds >= bRemain)
        {
            rankLetter  = "B";
            rankTitle   = "NICE PUMP!";
            rankMainCol = colYellow;
            rankSubCol  = GetColor(180, 140, 0);
        }

        snprintf(critBuf, sizeof(critBuf), "残り時間基準: S ≧ %.0fs / A ≧ %.0fs / B ≧ %.0fs", sRemain, aRemain, bRemain);
    }
    else
    {
        // 従来の経過タイム制（Config::USE_TIME_LIMIT_MODE = false でいつでも戻せます）
        float sLimit = isStage2 ? 60.0f : 30.0f;
        float aLimit = isStage2 ? 75.0f : 45.0f;
        float bLimit = isStage2 ? 90.0f : 60.0f;

        if (m_clearTimeSeconds <= sLimit)
        {
            rankLetter  = "S";
            rankTitle   = "GOD MUSCLE CAT!!";
            rankMainCol = colGold;
            rankSubCol  = colOrange;
        }
        else if (m_clearTimeSeconds <= aLimit)
        {
            rankLetter  = "A";
            rankTitle   = "GREAT MUSCLE!";
            rankMainCol = colOrange;
            rankSubCol  = colNeonRed;
        }
        else if (m_clearTimeSeconds <= bLimit)
        {
            rankLetter  = "B";
            rankTitle   = "NICE PUMP!";
            rankMainCol = colYellow;
            rankSubCol  = GetColor(180, 140, 0);
        }

        snprintf(critBuf, sizeof(critBuf), "S ≦ %.0fs  /  A ≦ %.0fs  /  B ≦ %.0fs", sLimit, aLimit, bLimit);
    }

    // =========================================================================
    // 2. メインコンテンツ（中央左側：ランク表示 ＆ アメコミ風爆発・閃光エフェクト）
    // =========================================================================
    int rankCX = 310;
    int rankCY = 355;

    // スタンプが「ドカン！」と叩きつけられるアニメーション（ズーム完了後に拡大→着地バウンス）
    float stampProgress = MathHelper::Clamp(uiTimer / 14.0f, 0.0f, 1.0f);
    float stampBounce = (uiTimer >= 14 && uiTimer < 26)
        ? std::sin((uiTimer - 14) * 0.5f) * 0.08f
        : 0.0f;
    float burstScale = stampProgress + stampBounce;

    // アメコミ風ギザギザ爆発・閃光エフェクト（Boom / Flash ポリゴン）
    // ※ 頂点ごとに半径が食い違って「ひげ線」が出ないよう、各頂点の半径を配列で事前確定してから隣接頂点同士を結合
    constexpr int NUM_VERTICES = 28; // 14本のトゲ（偶数=外トゲ、奇数=内谷）
    for (int layer = 0; layer < 4; ++layer)
    {
        float rOuterBase = 200.0f;
        float rInnerBase = 132.0f;
        unsigned int layerCol = colBlack;
        float angleOffset = animSec * 0.22f;

        if (layer == 0)
        {
            // ドロップシャドウ＆極太黒縁
            rOuterBase = 208.0f * burstScale;
            rInnerBase = 140.0f * burstScale;
            layerCol = colBlack;
        }
        else if (layer == 1)
        {
            // 外側：ネオンレッド〜オレンジの爆発ギザギザ
            rOuterBase = 196.0f * burstScale;
            rInnerBase = 128.0f * burstScale;
            layerCol = (rankLetter[0] == 'S') ? colNeonRed : colOrange;
        }
        else if (layer == 2)
        {
            // 中間：イエローの閃光バースト（逆回転オフセット）
            rOuterBase = 168.0f * burstScale;
            rInnerBase = 108.0f * burstScale;
            layerCol = colYellow;
            angleOffset = -animSec * 0.30f + 0.22f;
        }
        else
        {
            // 内側：ホワイトのフラッシュコア
            rOuterBase = 132.0f * burstScale;
            rInnerBase = 90.0f * burstScale;
            layerCol = colWhite;
            angleOffset = animSec * 0.15f;
        }

        int vx[NUM_VERTICES];
        int vy[NUM_VERTICES];
        int ox = (layer == 0) ? 6 : 0;
        int oy = (layer == 0) ? 6 : 0;

        for (int i = 0; i < NUM_VERTICES; ++i)
        {
            float angle = angleOffset + (MathHelper::PI * 2.0f * i) / NUM_VERTICES;
            float r = (i % 2 == 0) ? rOuterBase : rInnerBase;
            if (i % 4 == 0)
            {
                r *= 1.07f; // 1つおきに長いトゲを作る（前後の三角形で同一座標を共有するため段差線が一切出ない）
            }
            vx[i] = rankCX + ox + static_cast<int>(std::cos(angle) * r);
            vy[i] = rankCY + oy + static_cast<int>(std::sin(angle) * r);
        }

        for (int i = 0; i < NUM_VERTICES; ++i)
        {
            int next = (i + 1) % NUM_VERTICES;
            DrawTriangle(rankCX + ox, rankCY + oy, vx[i], vy[i], vx[next], vy[next], layerCol, TRUE);
        }
    }

    // スタンププレート（「ドカン！」と押し出された立体ハンコ枠）
    int stampW = static_cast<int>(220 * burstScale);
    int stampH = static_cast<int>(210 * burstScale);
    int stampX = rankCX - stampW / 2;
    int stampY = rankCY - stampH / 2;

    // スタンプの立体影＋極太ブラック枠＋ダーク背景
    DrawBox(stampX + 8, stampY + 8, stampX + stampW + 8, stampY + stampH + 8, colBlack, TRUE);
    DrawBox(stampX - 5, stampY - 5, stampX + stampW + 5, stampY + stampH + 5, colBlack, TRUE);
    DrawBox(stampX, stampY, stampX + stampW, stampY + stampH, GetColor(20, 22, 34), TRUE);

    // スタンプ内側の二重枠（ゴールドまたはランクカラー）
    DrawBox(stampX + 6, stampY + 6, stampX + stampW - 6, stampY + stampH - 6, rankMainCol, FALSE);
    DrawBox(stampX + 7, stampY + 7, stampX + stampW - 7, stampY + stampH - 7, rankMainCol, FALSE);
    DrawBox(stampX + 8, stampY + 8, stampX + stampW - 8, stampY + stampH - 8, rankMainCol, FALSE);

    // 左上：「BOOM!!」アメコミ擬音バッジ
    int boomX = stampX - 28;
    int boomY = stampY - 22;
    DrawBox(boomX + 4, boomY + 4, boomX + 114, boomY + 36, colBlack, TRUE);
    DrawBox(boomX - 2, boomY - 2, boomX + 110, boomY + 32, colBlack, TRUE);
    DrawBox(boomX, boomY, boomX + 108, boomY + 30, colNeonRed, TRUE);
    DrawStringToHandle(boomX + 10, boomY + 4, "BOOM!!", colYellow, font24);

    // 特大ランク文字（S / A / B / C）の3D立体描画
    int rLetterW = GetDrawStringWidthToHandle(rankLetter, static_cast<int>(strlen(rankLetter)), font96);
    int rLetterX = rankCX - rLetterW / 2;
    int rLetterY = stampY + 18;

    // 3D押し出し影
    for (int d = 4; d <= 8; ++d)
    {
        DrawStringToHandle(rLetterX + d, rLetterY + d, rankLetter, colBlack, font96);
    }
    // 極太ブラック縁取り
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(rLetterX + dx, rLetterY + dy, rankLetter, colBlack, font96);
            }
        }
    }
    // 本体カラー＆ハイライト
    DrawStringToHandle(rLetterX, rLetterY + 3, rankLetter, rankSubCol, font96);
    DrawStringToHandle(rLetterX, rLetterY,     rankLetter, rankMainCol, font96);
    DrawStringToHandle(rLetterX, rLetterY - 2, rankLetter, colWhite, font96);

    // 「S RANK」スタンプ下部バー
    char rankLabelBuf[32];
    snprintf(rankLabelBuf, sizeof(rankLabelBuf), "%s RANK", rankLetter);
    int rLabelW = GetDrawStringWidthToHandle(rankLabelBuf, static_cast<int>(strlen(rankLabelBuf)), font36);
    int rLabelX = rankCX - rLabelW / 2;
    int rLabelY = stampY + stampH - 64;

    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(rLabelX + dx, rLabelY + dy, rankLabelBuf, colBlack, font36);
            }
        }
    }
    DrawStringToHandle(rLabelX + 3, rLabelY + 3, rankLabelBuf, colBlack, font36);
    DrawStringToHandle(rLabelX, rLabelY, rankLabelBuf, colWhite, font36);

    // ランク称号リボン（スタンプ直下）
    int titleRibbonW = 260;
    int titleRibbonH = 34;
    int titleRibbonX = rankCX - titleRibbonW / 2;
    int titleRibbonY = stampY + stampH + 18;

    DrawBox(titleRibbonX + 4, titleRibbonY + 4, titleRibbonX + titleRibbonW + 4, titleRibbonY + titleRibbonH + 4, colBlack, TRUE);
    DrawBox(titleRibbonX - 2, titleRibbonY - 2, titleRibbonX + titleRibbonW + 2, titleRibbonY + titleRibbonH + 2, colBlack, TRUE);
    DrawBox(titleRibbonX, titleRibbonY, titleRibbonX + titleRibbonW, titleRibbonY + titleRibbonH, rankMainCol, TRUE);

    int rTitleW = GetDrawStringWidthToHandle(rankTitle, static_cast<int>(strlen(rankTitle)), font18);
    DrawStringToHandle(rankCX - rTitleW / 2, titleRibbonY + 8, rankTitle, colBlack, font18);

    // ランク基準目安テキスト
    int critW = GetDrawStringWidthToHandle(critBuf, static_cast<int>(strlen(critBuf)), font13);
    DrawBox(rankCX - critW / 2 - 10, titleRibbonY + 42, rankCX + critW / 2 + 10, titleRibbonY + 62, colBlack, TRUE);
    DrawStringToHandle(rankCX - critW / 2, titleRibbonY + 46, critBuf, colYellow, font13);

    // =========================================================================
    // 3. サブコンテンツ（中央右側：斜めカッティングのコミック・リザルトプレート）
    // =========================================================================
    auto DrawSlantedBox = [&](int x, int y, int w, int h, int skew, int shadowOff, unsigned int fillCol, unsigned int borderCol, int borderThick)
    {
        auto FillQuad = [](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int col)
        {
            DrawTriangle(x1, y1, x2, y2, x3, y3, col, TRUE);
            DrawTriangle(x1, y1, x3, y3, x4, y4, col, TRUE);
        };
        if (shadowOff > 0)
        {
            FillQuad(x + skew + shadowOff, y + shadowOff,
                     x + w + skew + shadowOff, y + shadowOff,
                     x + w + shadowOff, y + h + shadowOff,
                     x + shadowOff, y + h + shadowOff, colBlack);
        }
        if (borderThick > 0)
        {
            int b = borderThick;
            FillQuad(x + skew - b, y - b,
                     x + w + skew + b, y - b,
                     x + w + b, y + h + b,
                     x - b, y + h + b, colBlack);
        }
        if (borderCol != colBlack)
        {
            FillQuad(x + skew - 2, y - 2,
                     x + w + skew + 2, y - 2,
                     x + w + 2, y + h + 2,
                     x - 2, y + h + 2, borderCol);
        }
        FillQuad(x + skew, y,
                 x + w + skew, y,
                 x + w, y + h,
                 x, y + h, fillCol);
    };

    int panelX = 556;
    int panelY = 148;
    int panelW = 644;
    int panelH = 428;

    // メイン外枠（斜めカッティング＋ハードブラックシャドウ）
    DrawSlantedBox(panelX, panelY, panelW, panelH, 14, 10, GetColor(20, 23, 35), colYellow, 5);

    // パネル上部の斜めヘッダー帯
    DrawSlantedBox(panelX + 10, panelY - 6, 260, 42, 10, 4, colOrange, colBlack, 3);
    DrawStringToHandle(panelX + 34, panelY + 6, "RESULT", colBlack, font24);
    DrawStringToHandle(panelX + 32, panelY + 4, "RESULT", colWhite, font24);
    DrawStringToHandle(panelX + 138, panelY + 11, "リザルト成績", colBlack, font16);

    // プレイヤー・捕獲情報・各ボーナスポイントの取得
    auto player = m_objManager.GetPlayer();
    int achievedLv = player ? player->GetRepCount() : 0;
    int totalMice  = Config::NORMAL_MOUSE_COUNT + Config::FAST_MOUSE_COUNT;
    int caughtMice = m_objManager.GetCaughtCount();
    if (!m_isTimeUp && caughtMice < totalMice) caughtMice = totalMice; // クリア時は全捕獲
    int caughtPct  = (totalMice > 0) ? (caughtMice * 100 / totalMice) : 100;

    // 各ボーナススコアの算出（残り時間によって大きくスコアが変動）
    int dPts = achievedLv * 150 + 300;
    int cPts = caughtMice * 200;
    int sPts = Config::USE_TIME_LIMIT_MODE
        ? static_cast<int>(m_remainingTimeSeconds * 30.0f)
        : ((rankLetter[0] == 'S') ? 1500 : (rankLetter[0] == 'A' ? 1000 : (rankLetter[0] == 'B' ? 600 : 300)));
    int totalScore = dPts + cPts + sPts;

    // パネル右上：TOTAL SCORE 斜めタグ
    char scoreHeaderBuf[64];
    snprintf(scoreHeaderBuf, sizeof(scoreHeaderBuf), "SCORE  %d", totalScore);
    int shW = GetDrawStringWidthToHandle(scoreHeaderBuf, static_cast<int>(strlen(scoreHeaderBuf)), font24);
    int shPlateW = shW + 36;
    int shPlateX = panelX + panelW - shPlateW - 12;
    DrawSlantedBox(shPlateX, panelY - 6, shPlateW, 42, 10, 4, colYellow, colBlack, 3);
    DrawStringToHandle(shPlateX + 22, panelY + 5, scoreHeaderBuf, colBlack, font24);

    // 表示対象タイムを MM:SS.ms に変換
    float displaySeconds = Config::USE_TIME_LIMIT_MODE ? m_remainingTimeSeconds : m_clearTimeSeconds;
    int totalCentisec = static_cast<int>(displaySeconds * 100.0f + 0.5f);
    int minutes  = totalCentisec / 6000;
    int seconds  = (totalCentisec / 100) % 60;
    int centisec = totalCentisec % 100;

    int rowX = panelX + 26;
    int rowW = panelW - 56;

    // --- 項目1: タイム表示 ---
    int row1Y = panelY + 54;
    int row1H = 72;
    DrawSlantedBox(rowX + 4, row1Y, rowW, row1H, 10, 5, GetColor(12, 14, 22), colYellow, 3);

    if (Config::USE_TIME_LIMIT_MODE)
    {
        DrawStringToHandle(rowX + 26, row1Y + 12, "TIME LEFT", colYellow, font18);
        char subTimeBuf[64];
        snprintf(subTimeBuf, sizeof(subTimeBuf), "残り時間  (経過 %.2f秒 / 120秒)", m_clearTimeSeconds);
        DrawStringToHandle(rowX + 26, row1Y + 38, subTimeBuf, GetColor(175, 190, 215), font13);
    }
    else
    {
        DrawStringToHandle(rowX + 26, row1Y + 12, "CLEAR TIME", colYellow, font18);
        DrawStringToHandle(rowX + 26, row1Y + 38, "クリアタイム", GetColor(175, 190, 215), font13);
    }

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d.%02d", minutes, seconds, centisec);
    int timeW = GetDrawStringWidthToHandle(timeStr, static_cast<int>(strlen(timeStr)), font48);
    int timeX = rowX + rowW - timeW - 22;
    int timeY = row1Y + 12;
    DrawStringToHandle(timeX + 3, timeY + 3, timeStr, colBlack, font48);
    DrawStringToHandle(timeX, timeY, timeStr, colWhite, font48);

    // --- 項目2: 到達マッスルレベル ---
    int row2Y = row1Y + row1H + 12;
    int row2H = 60;
    DrawSlantedBox(rowX + 2, row2Y, rowW, row2H, 8, 5, GetColor(12, 14, 22), colOrange, 3);

    DrawStringToHandle(rowX + 24, row2Y + 10, "MUSCLE LEVEL", colOrange, font18);
    DrawStringToHandle(rowX + 24, row2Y + 34, "到達マッスルレベル", GetColor(175, 190, 215), font13);

    char lvStr[32];
    snprintf(lvStr, sizeof(lvStr), "Lv.%d", achievedLv);
    int lvW = GetDrawStringWidthToHandle(lvStr, static_cast<int>(strlen(lvStr)), font36);
    int lvX = rowX + rowW - lvW - 24;
    int lvY = row2Y + 12;
    DrawStringToHandle(lvX + 3, lvY + 3, lvStr, colBlack, font36);
    DrawStringToHandle(lvX, lvY, lvStr, colGold, font36);

    // --- 項目3: 捕獲したネズミ ---
    int row3Y = row2Y + row2H + 12;
    int row3H = 60;
    DrawSlantedBox(rowX, row3Y, rowW, row3H, 8, 5, GetColor(12, 14, 22), colNeonRed, 3);

    DrawStringToHandle(rowX + 22, row3Y + 10, "CAUGHT MICE", GetColor(255, 110, 140), font18);
    DrawStringToHandle(rowX + 22, row3Y + 34, "捕獲したネズミ", GetColor(175, 190, 215), font13);

    char caughtStr[48];
    snprintf(caughtStr, sizeof(caughtStr), "%d / %d (%d%%)", caughtMice, totalMice, caughtPct);
    int caughtW = GetDrawStringWidthToHandle(caughtStr, static_cast<int>(strlen(caughtStr)), font36);
    int caughtX = rowX + rowW - caughtW - 24;
    int caughtY = row3Y + 12;
    DrawStringToHandle(caughtX + 3, caughtY + 3, caughtStr, colBlack, font36);
    DrawStringToHandle(caughtX, caughtY, caughtStr, colWhite, font36);

    // --- 項目4: ボーナス内訳（3つの斜めミニプレート） ---
    int iconCardW = 184;
    int iconCardH = 88;
    int iconGap   = 18;
    int iconStartX = rowX;
    int iconCardY  = row3Y + row3H + 18;

    // [アイコン1: ダンベル]
    {
        int icX = iconStartX;
        DrawSlantedBox(icX, iconCardY, iconCardW, iconCardH, 8, 4, GetColor(15, 18, 28), colOrange, 3);

        int dbCX = icX + 40;
        int dbCY = iconCardY + 44;
        DrawBox(dbCX - 22, dbCY - 4, dbCX + 22, dbCY + 4, colBlack, TRUE);
        DrawBox(dbCX - 20, dbCY - 2, dbCX + 20, dbCY + 2, GetColor(200, 210, 225), TRUE);
        DrawBox(dbCX - 24, dbCY - 18, dbCX - 12, dbCY + 18, colBlack, TRUE);
        DrawBox(dbCX - 22, dbCY - 16, dbCX - 14, dbCY + 16, colOrange, TRUE);
        DrawBox(dbCX - 29, dbCY - 12, dbCX - 23, dbCY + 12, colYellow, TRUE);
        DrawBox(dbCX + 12, dbCY - 18, dbCX + 24, dbCY + 18, colBlack, TRUE);
        DrawBox(dbCX + 14, dbCY - 16, dbCX + 22, dbCY + 16, colOrange, TRUE);
        DrawBox(dbCX + 23, dbCY - 12, dbCX + 29, dbCY + 12, colYellow, TRUE);

        DrawStringToHandle(icX + 78, iconCardY + 16, "筋トレボーナス", colOrange, font13);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d", dPts);
        DrawStringToHandle(icX + 80, iconCardY + 38, ptBuf, colBlack, font24);
        DrawStringToHandle(icX + 78, iconCardY + 36, ptBuf, colWhite, font24);
    }

    // [アイコン2: 猫缶]
    {
        int icX = iconStartX + iconCardW + iconGap;
        DrawSlantedBox(icX, iconCardY, iconCardW, iconCardH, 8, 4, GetColor(15, 18, 28), colYellow, 3);

        int canCX = icX + 40;
        int canCY = iconCardY + 46;
        DrawBox(canCX - 22, canCY - 18, canCX + 22, canCY + 18, colBlack, TRUE);
        DrawBox(canCX - 20, canCY - 16, canCX + 20, canCY + 16, colNeonRed, TRUE);
        DrawBox(canCX - 20, canCY - 6,  canCX + 20, canCY + 8,  colYellow, TRUE);
        DrawOval(canCX, canCY - 16, 20, 6, colBlack, FALSE);
        DrawOval(canCX, canCY - 16, 19, 5, colGold, TRUE);

        DrawStringToHandle(icX + 78, iconCardY + 16, "捕獲ボーナス", colYellow, font13);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d", cPts);
        DrawStringToHandle(icX + 80, iconCardY + 38, ptBuf, colBlack, font24);
        DrawStringToHandle(icX + 78, iconCardY + 36, ptBuf, colWhite, font24);
    }

    // [アイコン3: スター]
    {
        int icX = iconStartX + (iconCardW + iconGap) * 2;
        DrawSlantedBox(icX, iconCardY, iconCardW, iconCardH, 8, 4, GetColor(15, 18, 28), colGold, 3);

        int stCX = icX + 40;
        int stCY = iconCardY + 44;
        DrawCircle(stCX, stCY, 22, colBlack, TRUE);
        DrawCircle(stCX, stCY, 19, colNeonRed, TRUE);
        DrawStringToHandle(stCX - 17, stCY - 18, "★", colBlack, font36);
        DrawStringToHandle(stCX - 18, stCY - 19, "★", colGold, font36);

        DrawStringToHandle(icX + 78, iconCardY + 16, "タイムボーナス", colGold, font13);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d", sPts);
        DrawStringToHandle(icX + 80, iconCardY + 38, ptBuf, colBlack, font24);
        DrawStringToHandle(icX + 78, iconCardY + 36, ptBuf, colYellow, font24);
    }

    // =========================================================================
    // 4. アクションボタン（下部：斜めカッティングのコミック・アーケードボタン）
    // =========================================================================
    int btnW = 340;
    int btnH = 60;
    int btnGap = 50;
    int totalBtnsW = btnW * 2 + btnGap;
    int btn1X = (Config::SCREEN_WIDTH - totalBtnsW) / 2;
    int btn2X = btn1X + btnW + btnGap;
    int btnY  = 620;

    auto DrawArcadeButton = [&](int bx, int by, bool isSelected, const char* label, const char* keyBadge, unsigned int activeMainCol)
    {
        int shiftY = isSelected ? -4 : 0;
        unsigned int bgCol  = isSelected ? activeMainCol : GetColor(24, 28, 42);
        unsigned int brdCol = isSelected ? colWhite : colYellow;

        DrawSlantedBox(bx, by + shiftY, btnW, btnH, 14, 7, bgCol, brdCol, 4);

        // 左上の斜めキーバッジ
        int kw = 58;
        int kh = 28;
        int kx = bx + 18;
        int ky = by + shiftY + (btnH - kh) / 2;
        DrawSlantedBox(kx, ky, kw, kh, 6, 3, isSelected ? colBlack : colNeonRed, colBlack, 2);
        int ktw = GetDrawStringWidthToHandle(keyBadge, static_cast<int>(strlen(keyBadge)), font16);
        DrawStringToHandle(kx + 3 + (kw - ktw) / 2, ky + 6, keyBadge, colWhite, font16);

        // メインラベル
        int labelAreaX = kx + kw + 10;
        int labelAreaW = (bx + btnW) - labelAreaX - 10;
        int lbTextW = GetDrawStringWidthToHandle(label, static_cast<int>(strlen(label)), font24);
        int lbX = labelAreaX + (labelAreaW - lbTextW) / 2;
        int lbY = by + shiftY + (btnH - 24) / 2;

        if (isSelected)
        {
            DrawStringToHandle(lbX + 2, lbY + 2, label, colWhite, font24);
            DrawStringToHandle(lbX,     lbY,     label, colBlack, font24);
        }
        else
        {
            DrawStringToHandle(lbX + 2, lbY + 2, label, colBlack, font24);
            DrawStringToHandle(lbX,     lbY,     label, colWhite, font24);
        }
    };

    DrawArcadeButton(btn1X, btnY, (m_clearSelectedBtn == 0), "もういちど遊ぶ", "R", colYellow);
    DrawArcadeButton(btn2X, btnY, (m_clearSelectedBtn == 1), "タイトルへ戻る", "M", colOrange);
}

// ============================================================================
// タイトル画面の描画（背景の3D空間を活かし、斜めプレートで魅せるコミックUI）
// ============================================================================
void GameManager::DrawTitleScreen()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font60 = fm.GetFont60();

    unsigned int colYellow  = GetColor(255, 230, 0);
    unsigned int colOrange  = GetColor(255, 135, 0);
    unsigned int colBlack   = GetColor(0, 0, 0);
    unsigned int colWhite   = GetColor(255, 255, 255);
    unsigned int colGold    = GetColor(255, 205, 0);
    unsigned int colNeonRed = GetColor(245, 25, 65);
    unsigned int colDarkBg  = GetColor(18, 21, 32);

    auto DrawSlantedPlate = [&](int x, int y, int w, int h, int skew, int shadowOff, unsigned int fillCol, unsigned int borderCol, int borderThick)
    {
        auto FillQuad = [](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int col)
        {
            DrawTriangle(x1, y1, x2, y2, x3, y3, col, TRUE);
            DrawTriangle(x1, y1, x3, y3, x4, y4, col, TRUE);
        };
        if (shadowOff > 0)
        {
            FillQuad(x + skew + shadowOff, y + shadowOff,
                     x + w + skew + shadowOff, y + shadowOff,
                     x + w + shadowOff, y + h + shadowOff,
                     x + shadowOff, y + h + shadowOff, colBlack);
        }
        if (borderThick > 0)
        {
            int b = borderThick;
            FillQuad(x + skew - b, y - b,
                     x + w + skew + b, y - b,
                     x + w + b, y + h + b,
                     x - b, y + h + b, colBlack);
        }
        if (borderCol != colBlack)
        {
            FillQuad(x + skew - 2, y - 2,
                     x + w + skew + 2, y - 2,
                     x + w + 2, y + h + 2,
                     x - 2, y + h + 2, borderCol);
        }
        FillQuad(x + skew, y,
                 x + w + skew, y,
                 x + w, y + h,
                 x, y + h, fillCol);
    };

    auto DrawBoldText = [&](int x, int y, const char* str, unsigned int col, int fontHandle, int outline = 2)
    {
        for (int dy = -outline; dy <= outline; ++dy)
        {
            for (int dx = -outline; dx <= outline; ++dx)
            {
                if (dx != 0 || dy != 0)
                {
                    DrawStringToHandle(x + dx, y + dy, str, colBlack, fontHandle);
                }
            }
        }
        DrawStringToHandle(x + outline, y + outline, str, colBlack, fontHandle);
        DrawStringToHandle(x, y, str, col, fontHandle);
    };

    // 1. 四隅のさりげないハーフトーン・ドット模様（背景3Dを邪魔しない控えめな演出）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 55);
    int dotStep = 28;
    for (int dy = 20; dy < Config::SCREEN_HEIGHT - 20; dy += dotStep)
    {
        int rowOffset = ((dy / dotStep) % 2) * (dotStep / 2);
        for (int dx = 0; dx < Config::SCREEN_WIDTH; dx += dotStep)
        {
            int px = dx + rowOffset;
            float distCornerTL = std::sqrt(static_cast<float>(px * px + dy * dy));
            float distCornerBR = std::sqrt(static_cast<float>((Config::SCREEN_WIDTH - px) * (Config::SCREEN_WIDTH - px) + (Config::SCREEN_HEIGHT - dy) * (Config::SCREEN_HEIGHT - dy)));
            float minCorner = (distCornerTL < distCornerBR) ? distCornerTL : distCornerBR;
            if (minCorner < 280.0f)
            {
                int radius = static_cast<int>((1.0f - minCorner / 280.0f) * 5.5f);
                if (radius >= 2)
                {
                    DrawCircle(px, dy, radius, colOrange, TRUE);
                }
            }
        }
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. ロゴ背面のサンバースト（淡い放射光）
    int sunburstCX = Config::SCREEN_WIDTH / 2;
    int sunburstCY = 140;
    float sunAngle = GetNowCount() * 0.0005f;
    int rayCount = 12;
    float rayRadius = 420.0f;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 42);
    for (int r = 0; r < rayCount; ++r)
    {
        float a1 = sunAngle + static_cast<float>(r * 2) * (MathHelper::PI / rayCount);
        float a2 = a1 + (MathHelper::PI / rayCount) * 0.7f;
        int p1x = sunburstCX + static_cast<int>(std::cos(a1) * rayRadius);
        int p1y = sunburstCY + static_cast<int>(std::sin(a1) * rayRadius);
        int p2x = sunburstCX + static_cast<int>(std::cos(a2) * rayRadius);
        int p2y = sunburstCY + static_cast<int>(std::sin(a2) * rayRadius);
        DrawTriangle(sunburstCX, sunburstCY, p1x, p1y, p2x, p2y, colYellow, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 3. タイトルロゴ背面の斜めコミックプレート＆「MUSCLE CAT 3D」ロゴ
    float t = GetNowCount() * 0.004f;
    float bounceOffset = std::abs(std::sin(t)) * -7.0f;
    int logoBaseY = static_cast<int>(82.0f + bounceOffset);

    int plateW = 720;
    int plateH = 124;
    int plateX = (Config::SCREEN_WIDTH - plateW) / 2 - 10;
    int plateY = 62;
    DrawSlantedPlate(plateX, plateY, plateW, plateH, 24, 9, colDarkBg, colYellow, 5);

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

    // 3D押し出しブラックシャドウ
    for (int d = 8; d >= 4; --d)
    {
        DrawStringToHandle(xMuscle + d, logoBaseY + d, txtMuscle, colBlack, font60);
        DrawStringToHandle(xCat + d,    logoBaseY + d, txtCat,    colBlack, font60);
        DrawStringToHandle(x3D + d,     logoBaseY + d, txt3D,     colBlack, font60);
    }
    // 極太ブラックアウトライン
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(xMuscle + dx, logoBaseY + dy, txtMuscle, colBlack, font60);
                DrawStringToHandle(xCat + dx,    logoBaseY + dy, txtCat,    colBlack, font60);
                DrawStringToHandle(x3D + dx,     logoBaseY + dy, txt3D,     colBlack, font60);
            }
        }
    }

    // ロゴ本体カラー
    DrawStringToHandle(xMuscle + 2, logoBaseY + 2, txtMuscle, colOrange, font60);
    DrawStringToHandle(xMuscle,     logoBaseY,     txtMuscle, colYellow, font60);
    DrawStringToHandle(xCat + 2,    logoBaseY + 2, txtCat,    colOrange, font60);
    DrawStringToHandle(xCat,        logoBaseY,     txtCat,    colWhite,  font60);
    DrawStringToHandle(x3D + 2,     logoBaseY + 2, txt3D,     colBlack,   font60);
    DrawStringToHandle(x3D,         logoBaseY,     txt3D,     colNeonRed, font60);

    // 右下に重なる日本語サブタイトル斜めタグ（「マッスルねこ」）
    int subPlateW = 210;
    int subPlateH = 34;
    int subPlateX = plateX + plateW - subPlateW - 14;
    int subPlateY = plateY + plateH - 18;
    DrawSlantedPlate(subPlateX, subPlateY, subPlateW, subPlateH, 10, 4, colNeonRed, colYellow, 3);
    const char* jpSub = "マッスルねこ";
    int jpW = GetDrawStringWidthToHandle(jpSub, static_cast<int>(strlen(jpSub)), font18);
    DrawBoldText(subPlateX + 5 + (subPlateW - jpW) / 2, subPlateY + 8, jpSub, colWhite, font18, 2);

    // 4. 斜めカッティングのメニューボタン（3項目）
    int btnW = 420;
    int btnH = 52;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int startBtnY = 356;
    int stageBtnY = 420;
    int howBtnY   = 484;

    auto DrawTitleSlantedButton = [&](int bx, int by, int bw, int bh, const char* mainLabel, const char* subLabel, bool isSelected, unsigned int activeCol, bool showArrows = false)
    {
        int shiftX = isSelected ? 6 : 0;
        int shiftY = isSelected ? -2 : 0;
        int dx = bx + shiftX;
        int dy = by + shiftY;

        unsigned int fillCol = isSelected ? activeCol : colDarkBg;
        unsigned int brdCol  = isSelected ? colWhite  : colYellow;

        DrawSlantedPlate(dx, dy, bw, bh, 14, 7, fillCol, brdCol, 4);

        // 選択時は左端に小さな斜めアクセントタグを表示
        if (isSelected && !showArrows)
        {
            DrawSlantedPlate(dx - 18, dy + 10, 26, bh - 20, 8, 3, colNeonRed, colBlack, 2);
        }

        if (showArrows)
        {
            // ステージ選択用の左右矢印キャップ
            DrawSlantedPlate(dx + 10, dy + 9, 36, bh - 18, 6, 2, isSelected ? colBlack : colOrange, colBlack, 2);
            DrawStringToHandle(dx + 22, dy + 17, "◀", colWhite, font16);

            DrawSlantedPlate(dx + bw - 46, dy + 9, 36, bh - 18, 6, 2, isSelected ? colBlack : colOrange, colBlack, 2);
            DrawStringToHandle(dx + bw - 34, dy + 17, "▶", colWhite, font16);
        }

        int lw = GetDrawStringWidthToHandle(mainLabel, static_cast<int>(strlen(mainLabel)), font24);
        int lx = dx + 7 + (bw - lw) / 2;
        int ly = (subLabel != nullptr) ? (dy + 6) : (dy + (bh - 24) / 2);

        if (isSelected)
        {
            DrawStringToHandle(lx + 2, ly + 2, mainLabel, colWhite, font24);
            DrawStringToHandle(lx,     ly,     mainLabel, colBlack, font24);
        }
        else
        {
            DrawBoldText(lx, ly, mainLabel, colWhite, font24, 2);
        }

        if (subLabel != nullptr)
        {
            int sw = GetDrawStringWidthToHandle(subLabel, static_cast<int>(strlen(subLabel)), font13);
            int sx = dx + 5 + (bw - sw) / 2;
            DrawStringToHandle(sx, dy + 33, subLabel, isSelected ? colBlack : colYellow, font13);
        }
    };

    // ボタン1: ゲームスタート
    DrawTitleSlantedButton(btnX, startBtnY, btnW, btnH, "GAME START", "ゲームをはじめる", (m_selectedMenuItem == 0), colYellow, false);

    // ボタン2: ステージ選択
    const char* stageMain = (m_selectedMap == ModelConfig::MapType::House) ? "STAGE 1 : HOUSE" : "STAGE 2 : NEW MAP";
    const char* stageSub  = (m_selectedMap == ModelConfig::MapType::House) ? "通常の部屋ステージ" : "広めの新ステージ";
    DrawTitleSlantedButton(btnX, stageBtnY, btnW, btnH, stageMain, stageSub, (m_selectedMenuItem == 1), colGold, true);

    // ボタン3: あそびかた
    DrawTitleSlantedButton(btnX, howBtnY, btnW, btnH, "HOW TO PLAY", "ルール・操作方法", (m_selectedMenuItem == 2), colOrange, false);

    // 5. 画面下部の操作ガイド（ボックスで覆わずスッキリ縁取りテキストで表示）
    const char* helpStr = "[↑/↓] 選択    [←/→] ステージ切替    [ENTER / SPACE] 決定";
    int helpW = GetDrawStringWidthToHandle(helpStr, static_cast<int>(strlen(helpStr)), font16);
    DrawBoldText((Config::SCREEN_WIDTH - helpW) / 2, Config::SCREEN_HEIGHT - 38, helpStr, colWhite, font16, 2);

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
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colWhite, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// ============================================================================
// ゲーム説明モーダルダイアログの描画（斜めヘッダー＆コミックパネル）
// ============================================================================
void GameManager::DrawHowToPlayModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    unsigned int colYellow  = GetColor(255, 230, 0);
    unsigned int colOrange  = GetColor(255, 135, 0);
    unsigned int colBlack   = GetColor(0, 0, 0);
    unsigned int colWhite   = GetColor(255, 255, 255);
    unsigned int colGold    = GetColor(255, 205, 0);
    unsigned int colNeonRed = GetColor(245, 25, 65);
    unsigned int colDarkBg  = GetColor(18, 21, 32);
    unsigned int colCardBg  = GetColor(25, 30, 45);

    auto DrawSlantedPlate = [&](int x, int y, int w, int h, int skew, int shadowOff, unsigned int fillCol, unsigned int borderCol, int borderThick)
    {
        auto FillQuad = [](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int col)
        {
            DrawTriangle(x1, y1, x2, y2, x3, y3, col, TRUE);
            DrawTriangle(x1, y1, x3, y3, x4, y4, col, TRUE);
        };
        if (shadowOff > 0)
        {
            FillQuad(x + skew + shadowOff, y + shadowOff,
                     x + w + skew + shadowOff, y + shadowOff,
                     x + w + shadowOff, y + h + shadowOff,
                     x + shadowOff, y + h + shadowOff, colBlack);
        }
        if (borderThick > 0)
        {
            int b = borderThick;
            FillQuad(x + skew - b, y - b,
                     x + w + skew + b, y - b,
                     x + w + b, y + h + b,
                     x - b, y + h + b, colBlack);
        }
        if (borderCol != colBlack)
        {
            FillQuad(x + skew - 2, y - 2,
                     x + w + skew + 2, y - 2,
                     x + w + 2, y + h + 2,
                     x - 2, y + h + 2, borderCol);
        }
        FillQuad(x + skew, y,
                 x + w + skew, y,
                 x + w, y + h,
                 x, y + h, fillCol);
    };

    // 1. 全画面暗転レイヤー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 205);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(6, 8, 14), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ本体
    int modalW = 860;
    int modalH = 520;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    DrawSlantedPlate(modalX, modalY, modalW, modalH, 12, 10, colDarkBg, colYellow, 5);

    // 左上に重なる斜めタイトルタグ
    DrawSlantedPlate(modalX + 18, modalY - 12, 320, 46, 10, 5, colOrange, colBlack, 3);
    DrawStringToHandle(modalX + 42, modalY + 1, "HOW TO PLAY", colBlack, font24);
    DrawStringToHandle(modalX + 40, modalY - 1, "HOW TO PLAY", colWhite, font24);
    DrawStringToHandle(modalX + 225, modalY + 5, "あそびかた", colBlack, font16);

    // 3. 上段：3ステップの斜めコミックカード
    int cardY = modalY + 58;
    int cardH = 196;
    int cardW = 258;
    int gap = 18;
    int cardStartX = modalX + 26;

    auto DrawStepCard = [&](int cx, int cy, int cw, int ch, const char* badgeStr, const char* titleStr, unsigned int accentCol)
    {
        DrawSlantedPlate(cx, cy, cw, ch, 8, 6, colCardBg, accentCol, 3);
        DrawSlantedPlate(cx + 8, cy - 8, 86, 26, 6, 2, accentCol, colBlack, 2);
        DrawStringToHandle(cx + 20, cy - 2, badgeStr, colBlack, font13);
        DrawStringToHandle(cx + 104, cy + 8, titleStr, accentCol, font16);
        DrawLine(cx + 14, cy + 34, cx + cw - 12, cy + 34, accentCol);
    };

    // STEP 1: 筋トレ
    int c1X = cardStartX;
    DrawStepCard(c1X, cardY, cardW, cardH, "STEP 01", "筋トレでパワーUP", colYellow);
    DrawStringToHandle(c1X + 18, cardY + 50, "SPACE長押し ➔ 黄色ゾーンで離す", colWhite, font13);
    DrawStringToHandle(c1X + 18, cardY + 80, "成功すると体が巨大化＆速度UP!", colYellow, font13);
    DrawStringToHandle(c1X + 18, cardY + 110, "失敗すると5秒間「筋肉痛」で停止", colNeonRed, font13);
    DrawStringToHandle(c1X + 18, cardY + 140, "15秒間筋トレしないと -3 Lv減衰", GetColor(180, 195, 220), font13);

    // STEP 2: 必殺技
    int c2X = cardStartX + cardW + gap;
    DrawStepCard(c2X, cardY, cardW, cardH, "STEP 02", "必殺スキル (Lv.4~)", colOrange);
    DrawStringToHandle(c2X + 18, cardY + 50, "左クリック / Shift : 飛びつき", colYellow, font13);
    DrawStringToHandle(c2X + 26, cardY + 72, "前方へ大きく跳躍してネズミを捕獲", colWhite, font13);
    DrawStringToHandle(c2X + 18, cardY + 108, "右クリック / E : スタン衝撃波", colOrange, font13);
    DrawStringToHandle(c2X + 26, cardY + 130, "周囲のネズミを気絶させて足止め", colWhite, font13);

    // STEP 3: クリア目標
    int c3X = cardStartX + (cardW + gap) * 2;
    DrawStepCard(c3X, cardY, cardW, cardH, "STEP 03", "ネズミを全員捕獲!", colNeonRed);
    DrawStringToHandle(c3X + 18, cardY + 50, "ステージ内のネズミをすべて捕獲!", colWhite, font13);
    DrawStringToHandle(c3X + 18, cardY + 80, "気絶したネズミは簡単に捕まる!", colYellow, font13);
    DrawStringToHandle(c3X + 18, cardY + 110, "残り数が減るとネズミがスピードUP", colOrange, font13);
    DrawStringToHandle(c3X + 18, cardY + 140, "残り時間が多いほど高ランク獲得!", colGold, font13);

    // 4. 下段：操作一覧パネル
    int keyTableY = modalY + 274;
    int keyTableW = 810;
    int keyTableH = 178;
    int keyTableX = modalX + 25;

    DrawSlantedPlate(keyTableX, keyTableY, keyTableW, keyTableH, 8, 6, colCardBg, colYellow, 3);
    DrawSlantedPlate(keyTableX + 12, keyTableY - 10, 140, 26, 6, 2, colYellow, colBlack, 2);
    DrawStringToHandle(keyTableX + 28, keyTableY - 4, "CONTROLS", colBlack, font13);

    int col1X = keyTableX + 28;
    int col2X = keyTableX + 430;
    int row1Y = keyTableY + 32;
    int rowStep = 44;

    DrawStringToHandle(col1X, row1Y + rowStep * 0, "[SPACE]  筋トレ (長押し)", colYellow, font16);
    DrawStringToHandle(col1X + 16, row1Y + rowStep * 0 + 21, "ゲージの黄色ゾーンでタイミングよく離す", GetColor(190, 205, 225), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 1, "[左クリック / Shift]  飛びつき捕獲", colOrange, font16);
    DrawStringToHandle(col1X + 16, row1Y + rowStep * 1 + 21, "Lv.4以上で解放・前方のネズミへ跳躍", GetColor(190, 205, 225), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 2, "[右クリック / E]  スタン衝撃波", colNeonRed, font16);
    DrawStringToHandle(col1X + 16, row1Y + rowStep * 2 + 21, "Lv.4以上で解放・周囲のネズミを気絶させる", GetColor(190, 205, 225), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 0, "[W / A / S / D]  移動", colWhite, font16);
    DrawStringToHandle(col2X + 16, row1Y + rowStep * 0 + 21, "ネコの前後左右移動", GetColor(190, 205, 225), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 1, "[マウス / 矢印キー]  カメラ操作", colGold, font16);
    DrawStringToHandle(col2X + 16, row1Y + rowStep * 1 + 21, "感度は [- / +] またはポーズ画面で変更可能", GetColor(190, 205, 225), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 2, "[F] 視点リセット   /   [ESC] ポーズ", colYellow, font16);
    DrawStringToHandle(col2X + 16, row1Y + rowStep * 2 + 21, "カメラをネコの背後に戻す / 一時停止メニュー", GetColor(190, 205, 225), font13);

    // 5. 閉じるボタン
    int closeBtnW = 140;
    int closeBtnH = 38;
    int closeBtnX = modalX + modalW - 164;
    int closeBtnY = modalY + modalH - 52;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                       my >= closeBtnY && my <= closeBtnY + closeBtnH);

    DrawSlantedPlate(closeBtnX, closeBtnY, closeBtnW, closeBtnH, 8, 4, hoverClose ? colYellow : colNeonRed, colWhite, 3);
    const char* closeStr = "とじる [ESC]";
    int cw = GetDrawStringWidthToHandle(closeStr, static_cast<int>(strlen(closeStr)), font16);
    DrawStringToHandle(closeBtnX + 4 + (closeBtnW - cw) / 2, closeBtnY + 11, closeStr, hoverClose ? colBlack : colWhite, font16);
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
    int minusBtnX  = btnX + 14;
    int sliderBarX = btnX + 68;
    int sliderBarW = 224;
    int sliderBarY = sensRowY + 44;
    int sliderBarH = 12;
    int plusBtnX   = btnX + 300;

    int resumeBtnW = 260;
    int resumeBtnH = 38;
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
// ポーズモーダルダイアログ描画（斜めカッティングのコミック・ポーズ画面）
// ============================================================================
void GameManager::DrawPauseModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    unsigned int colYellow  = GetColor(255, 230, 0);
    unsigned int colOrange  = GetColor(255, 135, 0);
    unsigned int colBlack   = GetColor(0, 0, 0);
    unsigned int colWhite   = GetColor(255, 255, 255);
    unsigned int colGold    = GetColor(255, 205, 0);
    unsigned int colNeonRed = GetColor(245, 25, 65);
    unsigned int colDarkBg  = GetColor(18, 21, 32);
    unsigned int colCardBg  = GetColor(25, 30, 45);

    auto DrawSlantedPlate = [&](int x, int y, int w, int h, int skew, int shadowOff, unsigned int fillCol, unsigned int borderCol, int borderThick)
    {
        auto FillQuad = [](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned int col)
        {
            DrawTriangle(x1, y1, x2, y2, x3, y3, col, TRUE);
            DrawTriangle(x1, y1, x3, y3, x4, y4, col, TRUE);
        };
        if (shadowOff > 0)
        {
            FillQuad(x + skew + shadowOff, y + shadowOff,
                     x + w + skew + shadowOff, y + shadowOff,
                     x + w + shadowOff, y + h + shadowOff,
                     x + shadowOff, y + h + shadowOff, colBlack);
        }
        if (borderThick > 0)
        {
            int b = borderThick;
            FillQuad(x + skew - b, y - b,
                     x + w + skew + b, y - b,
                     x + w + b, y + h + b,
                     x - b, y + h + b, colBlack);
        }
        if (borderCol != colBlack)
        {
            FillQuad(x + skew - 2, y - 2,
                     x + w + skew + 2, y - 2,
                     x + w + 2, y + h + 2,
                     x - 2, y + h + 2, borderCol);
        }
        FillQuad(x + skew, y,
                 x + w + skew, y,
                 x + w, y + h,
                 x, y + h, fillCol);
    };

    // 1. 全画面暗転レイヤー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(6, 8, 14), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ外枠・本体
    int modalW = 480;
    int modalH = 400;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    DrawSlantedPlate(modalX, modalY, modalW, modalH, 14, 10, colDarkBg, colYellow, 5);

    // 左上に重なる斜めヘッダータグ
    DrawSlantedPlate(modalX + 20, modalY - 14, 210, 44, 10, 5, colOrange, colBlack, 3);
    DrawStringToHandle(modalX + 42, modalY - 2, "PAUSE", colBlack, font24);
    DrawStringToHandle(modalX + 40, modalY - 4, "PAUSE", colWhite, font24);
    DrawStringToHandle(modalX + 138, modalY + 2, "一時停止", colBlack, font16);

    // ボタン配置
    int btnW = 360;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int retryBtnY = modalY + 68;
    int retryBtnH = 50;
    int titleBtnY = modalY + 126;
    int titleBtnH = 50;
    int sensRowY  = modalY + 184;
    int sensRowH  = 74;

    auto DrawPauseSlantedBtn = [&](int bx, int by, int bw, int bh, const char* label, const char* keyCap, bool isSelected, unsigned int activeColor)
    {
        int shiftX = isSelected ? 5 : 0;
        int dx = bx + shiftX;
        DrawSlantedPlate(dx, by, bw, bh, 10, 6, isSelected ? activeColor : colCardBg, isSelected ? colWhite : colYellow, 3);

        // キーキャップ
        int kw = 44;
        int kh = 26;
        int kx = dx + 14;
        int ky = by + (bh - kh) / 2;
        DrawSlantedPlate(kx, ky, kw, kh, 5, 2, isSelected ? colBlack : colNeonRed, colBlack, 2);
        int ktw = GetDrawStringWidthToHandle(keyCap, static_cast<int>(strlen(keyCap)), font16);
        DrawStringToHandle(kx + 2 + (kw - ktw) / 2, ky + 5, keyCap, colWhite, font16);

        // ラベル
        int lw = GetDrawStringWidthToHandle(label, static_cast<int>(strlen(label)), font24);
        int lx = dx + 24 + (bw - lw) / 2;
        int ly = by + (bh - 24) / 2;
        if (isSelected)
        {
            DrawStringToHandle(lx + 2, ly + 2, label, colWhite, font24);
            DrawStringToHandle(lx,     ly,     label, colBlack, font24);
        }
        else
        {
            DrawStringToHandle(lx + 2, ly + 2, label, colBlack, font24);
            DrawStringToHandle(lx,     ly,     label, colWhite, font24);
        }
    };

    // ボタン1: リトライ
    DrawPauseSlantedBtn(btnX, retryBtnY, btnW, retryBtnH, "リトライ", "R", (m_pauseMenuItem == 0), colYellow);

    // ボタン2: タイトルへ戻る
    DrawPauseSlantedBtn(btnX, titleBtnY, btnW, titleBtnH, "タイトルへ戻る", "T", (m_pauseMenuItem == 1), colGold);

    // コントロール3: マウス感度調整
    bool isSensSelected = (m_pauseMenuItem == 2);
    int sensBtnW   = 46;
    int sensBtnH   = 28;
    int sensBtnY   = sensRowY + 36;
    int minusBtnX  = btnX + 14;
    int sliderBarX = btnX + 68;
    int sliderBarW = 224;
    int sliderBarY = sensRowY + 44;
    int sliderBarH = 12;
    int plusBtnX   = btnX + 300;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverMinus = (mx >= minusBtnX && mx <= minusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);
    bool hoverPlus  = (mx >= plusBtnX && mx <= plusBtnX + sensBtnW && my >= sensBtnY && my <= sensBtnY + sensBtnH);

    DrawSlantedPlate(btnX, sensRowY, btnW, sensRowH, 10, 6, colCardBg, isSensSelected ? colOrange : colYellow, 3);

    float currentSens = m_camera.GetMouseSensitivity();
    char sensTxt[64];
    snprintf(sensTxt, sizeof(sensTxt), "マウス感度 : %.1fx", currentSens);
    DrawStringToHandle(btnX + 18, sensRowY + 9, sensTxt, isSensSelected ? colYellow : colWhite, font16);
    DrawStringToHandle(btnX + btnW - 114, sensRowY + 11, "[←/→] で調整", GetColor(175, 190, 215), font13);

    // [◀ -] ボタン
    DrawSlantedPlate(minusBtnX, sensBtnY, sensBtnW, sensBtnH, 5, 2, hoverMinus ? colYellow : colDarkBg, colYellow, 2);
    DrawStringToHandle(minusBtnX + 11, sensBtnY + 6, "◀ -", hoverMinus ? colBlack : colWhite, font16);

    // スライダーバー
    DrawBox(sliderBarX - 2, sliderBarY - 2, sliderBarX + sliderBarW + 2, sliderBarY + sliderBarH + 2, colBlack, TRUE);
    DrawBox(sliderBarX, sliderBarY, sliderBarX + sliderBarW, sliderBarY + sliderBarH, GetColor(10, 12, 20), TRUE);
    DrawBox(sliderBarX, sliderBarY, sliderBarX + sliderBarW, sliderBarY + sliderBarH, colYellow, FALSE);

    float standardRatio = (1.0f - 0.2f) / (3.0f - 0.2f);
    int standardX = sliderBarX + static_cast<int>(sliderBarW * standardRatio);
    DrawLine(standardX, sliderBarY - 4, standardX, sliderBarY + sliderBarH + 4, colWhite);

    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    sensRatio = MathHelper::Clamp(sensRatio, 0.0f, 1.0f);
    int fillW = static_cast<int>(sliderBarW * sensRatio);
    if (fillW > 0)
    {
        unsigned int progColor = (currentSens >= 2.0f) ? colNeonRed : ((currentSens >= 1.0f) ? colYellow : colOrange);
        DrawBox(sliderBarX + 1, sliderBarY + 1, sliderBarX + fillW, sliderBarY + sliderBarH - 1, progColor, TRUE);
    }

    int knobX = sliderBarX + fillW;
    int knobY = sliderBarY + sliderBarH / 2;
    DrawCircle(knobX, knobY, 9, colBlack, TRUE);
    DrawCircle(knobX, knobY, 7, colYellow, TRUE);
    DrawCircle(knobX, knobY, 3, colBlack, TRUE);

    // [+ ▶] ボタン
    DrawSlantedPlate(plusBtnX, sensBtnY, sensBtnW, sensBtnH, 5, 2, hoverPlus ? colYellow : colDarkBg, colYellow, 2);
    DrawStringToHandle(plusBtnX + 11, sensBtnY + 6, "+ ▶", hoverPlus ? colBlack : colWhite, font16);

    // 下部: ゲームに戻る (ESC)
    int resumeBtnW = 260;
    int resumeBtnH = 38;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 280;

    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW &&
                       my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    DrawSlantedPlate(resumeBtnX, resumeBtnY, resumeBtnW, resumeBtnH, 8, 5, hoverResume ? colYellow : colNeonRed, colWhite, 3);
    const char* resumeTxt = "ゲームに戻る [ESC]";
    int rW = GetDrawStringWidthToHandle(resumeTxt, static_cast<int>(strlen(resumeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - rW) / 2 + 4, resumeBtnY + 11, resumeTxt, hoverResume ? colBlack : colWhite, font16);

    // 操作ガイド
    const char* guideTxt = "[↑/↓] 選択   [ENTER / SPACE] 決定   [ESC] 閉じる";
    int gW = GetDrawStringWidthToHandle(guideTxt, static_cast<int>(strlen(guideTxt)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW) / 2, modalY + 346, guideTxt, GetColor(185, 200, 225), font13);
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

    // =========================================================================
    // 6. 見えない壁（AddWallQuad補助壁 & 外周ClampToBounds）および壁コリジョンの3Dフレーム表示
    // =========================================================================
    {
        unsigned int invWallBeamCol = GetColor(255, 30, 210); // 見えない壁(AddWallQuad): 鮮やかなマゼンタ
        unsigned int invWallDiagCol = GetColor(255, 230, 60); // 見えない壁の対角フレーム: イエロー
        unsigned int meshWallCol    = GetColor(80, 235, 150); // モデル壁コリジョン: ライトグリーン

        for (const auto& wp : stage.GetWallPolygons())
        {
            if (wp.isInvisibleWall)
            {
                // 見えない壁（AddWallQuad）は視認しやすいよう立体ビームで3Dフレーム表示
                drawBeam3D(wp.v0, wp.v1, 2.0f, invWallBeamCol);
                drawBeam3D(wp.v1, wp.v2, 2.0f, invWallBeamCol);
                drawBeam3D(wp.v2, wp.v0, 1.4f, invWallDiagCol);
            }
            else
            {
                // 通常壁コリジョンポリゴンも3Dワイヤーフレーム表示
                DrawLine3D(wp.v0, wp.v1, meshWallCol);
                DrawLine3D(wp.v1, wp.v2, meshWallCol);
                DrawLine3D(wp.v2, wp.v0, meshWallCol);
            }
        }

        // ステージ外周のセーフティネット透明境界（ClampToBounds）の3Dフレーム表示
        VECTOR minB = stage.GetMinBounds();
        VECTOR maxB = stage.GetMaxBounds();
        float bY0 = -10.0f;
        float bY1 = 80.0f;
        unsigned int boundsCol = GetColor(200, 80, 255);
        VECTOR c00 = VGet(minB.x, bY0, minB.z), c10 = VGet(maxB.x, bY0, minB.z);
        VECTOR c11 = VGet(maxB.x, bY0, maxB.z), c01 = VGet(minB.x, bY0, maxB.z);
        VECTOR u00 = VGet(minB.x, bY1, minB.z), u10 = VGet(maxB.x, bY1, minB.z);
        VECTOR u11 = VGet(maxB.x, bY1, maxB.z), u01 = VGet(minB.x, bY1, maxB.z);
        DrawLine3D(c00, c10, boundsCol); DrawLine3D(c10, c11, boundsCol);
        DrawLine3D(c11, c01, boundsCol); DrawLine3D(c01, c00, boundsCol);
        DrawLine3D(u00, u10, boundsCol); DrawLine3D(u10, u11, boundsCol);
        DrawLine3D(u11, u01, boundsCol); DrawLine3D(u01, u00, boundsCol);
        DrawLine3D(c00, u00, boundsCol); DrawLine3D(c10, u10, boundsCol);
        DrawLine3D(c11, u11, boundsCol); DrawLine3D(c01, u01, boundsCol);
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
        unsigned int col = wp.isInvisibleWall ? GetColor(255, 50, 220) : wallMapCol;
        DrawLine(x0, y0, x1, y1, col);
        int x2, y2;
        worldToMinimap(wp.v2.x, wp.v2.z, x2, y2);
        DrawLine(x1, y1, x2, y2, col);
        DrawLine(x2, y2, x0, y0, col);
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

