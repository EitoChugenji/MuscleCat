#include "GameManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
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
    // 1. 左上: 2人掛けソファー
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(-220.0f, 0.0f, 160.0f), 80.0f, 22.0f, 48.0f,
        "SOFA", ModelConfig::BENCH_PRESS_MODEL_PATH,
        GetColor(75, 115, 135), GetColor(110, 155, 175)
        ));

    // 2. 右上: キャットタワー
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(220.0f, 0.0f, 160.0f), 50.0f, 40.0f, 50.0f,
        "CAT TOWER", ModelConfig::DUMBBELL_RACK_MODEL_PATH,
        GetColor(215, 195, 160), GetColor(160, 135, 95)
        ));

    // 3. 左下: 本棚（ブックシェルフ）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(-220.0f, 0.0f, -160.0f), 70.0f, 42.0f, 35.0f,
        "BOOKSHELF", ModelConfig::PROTEIN_BAR_MODEL_PATH,
        GetColor(130, 85, 55), GetColor(180, 125, 85)
        ));

    // 4. 右下: 収納チェスト・キャビネット
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(220.0f, 0.0f, -160.0f), 65.0f, 30.0f, 42.0f,
        "CHEST", ModelConfig::POWER_RACK_MODEL_PATH,
        GetColor(145, 95, 60), GetColor(200, 140, 95)
        ));

    // 5. 中央奥: テレビボード＆薄型TV
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(0.0f, 0.0f, 210.0f), 85.0f, 24.0f, 38.0f,
        "TV BOARD", ModelConfig::SMITH_MACHINE_MODEL_PATH,
        GetColor(50, 45, 45), GetColor(140, 140, 150)
        ));

    // 6. 中央手前: ローテーブル（ラグ上）
    m_objManager.AddObstacle(std::make_shared<Obstacle3D>
        (
        VGet(0.0f, 0.0f, -60.0f), 70.0f, 18.0f, 50.0f,
        "TABLE", ModelConfig::TREADMILL_MODEL_PATH,
        GetColor(165, 115, 75), GetColor(215, 165, 115)
        ));
}

void GameManager::SetupTitle()
{
    m_state = GameState::Title;
    m_objManager.Clear();

    // タイトル画面：猫だけを生成（ネズミは配置しない）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    m_objManager.SetPlayer(player);

    m_camera.Init(player->GetPos());
    SetupRoomObstacles();
}

void GameManager::StartGame()
{
    m_objManager.Clear();

    // プレイヤー生成（原点）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    m_objManager.SetPlayer(player);

    // カメラ初期化
    m_camera.Init(player->GetPos());

    // 部屋の家具配置
    SetupRoomObstacles();

    // ネズミの生成（ゲームスタート時に初めて出現）
    for (int i = 0; i < Config::NORMAL_MOUSE_COUNT; ++i)
    {
        float x = static_cast<float>(-380 + rand() % 760);
        float z = static_cast<float>(100 + rand() % 220);
        if (rand() % 2 == 0) z = -z;
        m_objManager.AddObject(std::make_shared<NormalMouse3D>(VGet(x, 0.0f, z), player));
    }

    for (int i = 0; i < Config::FAST_MOUSE_COUNT; ++i)
    {
        float x = static_cast<float>(-380 + rand() % 760);
        float z = static_cast<float>(120 + rand() % 200);
        if (rand() % 2 == 0) z = -z;
        m_objManager.AddObject(std::make_shared<FastMouse3D>(VGet(x, 0.0f, z), player));
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
        // タイトル画面：カメラが3D空間をゆったり旋回
        m_titleCameraAngle += 0.008f;
        VECTOR center = VGet(0.0f, 15.0f, 0.0f);
        float camDist = 180.0f;
        VECTOR camPos = VGet
        (
            std::sin(m_titleCameraAngle) * camDist,
            80.0f,
            std::cos(m_titleCameraAngle) * camDist
        );

        SetCameraPositionAndTarget_UpVecY(camPos, center);

        // ゲームスタート判定：Enterキー または マウス左クリックのみ
        bool isEnter = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
        bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
        m_prevMouseLeft = isLeftClick;

        if (isEnter || isMouseTrigger)
        {
            StartGame();
        }

    }
    
    else if (m_state == GameState::Playing)
    {
        SetMouseDispFlag(TRUE); // ゲーム中もカーソル表示（マウス移動操作用）

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            m_camera.Update(player->GetPos(), player->GetRotY(), true);
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
            m_camera.Update(player->GetPos(), player->GetRotY(), false);
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
}

void GameManager::Draw() {
    // 1. 3Dシーンの描画 (Zバッファ有効)
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    m_objManager.Draw3D();

    // 2. 2D HUD / UI描画 (Zバッファ無効)
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    m_objManager.Draw2D();

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
        // タイトル画面（簡易）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 140);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 15, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        int titleW = GetDrawStringWidthToHandle("マッスルねこ 3D", static_cast<int>(std::string("マッスルねこ 3D").length()), font48);
        DrawStringToHandle((Config::SCREEN_WIDTH - titleW) / 2, 160, "マッスルねこ 3D", yellow, font48);

        int subW = GetDrawStringWidthToHandle("- MUSCLE CAT 3D CHASE -", static_cast<int>(std::string("- MUSCLE CAT 3D CHASE -").length()), font18);
        DrawStringToHandle((Config::SCREEN_WIDTH - subW) / 2, 225, "- MUSCLE CAT 3D CHASE -", white, font18);

        // スタート促進点滅表示
        if ((GetNowCount() / 400) % 2 == 0)
        {
            int startW = GetDrawStringWidthToHandle("[ PRESS ENTER or CLICK TO START ]", static_cast<int>(std::string("[ PRESS ENTER or CLICK TO START ]").length()), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - startW) / 2, 360, "[ PRESS ENTER or CLICK TO START ]", cyan, font24);
        }

        // 操作説明パネル
        int panelX = (Config::SCREEN_WIDTH - 640) / 2;
        int panelY = 430;
        DrawBox(panelX, panelY, panelX + 640, panelY + 220, GetColor(30, 35, 48), TRUE);
        DrawBox(panelX, panelY, panelX + 640, panelY + 220, GetColor(100, 120, 160), FALSE);

        DrawStringToHandle(panelX + 20, panelY + 15, "【操作方法】", yellow, font18);
        DrawStringToHandle(panelX + 30, panelY + 45, "・移動: WASD キー または [マウス左クリック長押し]", white, font16);
        DrawStringToHandle(panelX + 30, panelY + 73, "・筋トレ: [SPACE] キー （タイミングよく押してRep獲得＆加速！）", GetColor(255, 210, 80), font16);
        DrawStringToHandle(panelX + 30, panelY + 101, "・タックル: [E] キー （ネズミ気絶！/壁激突で猫スタン・筋トレで強化）", GetColor(255, 160, 50), font16);
        DrawStringToHandle(panelX + 30, panelY + 129, "・飛びつき: [SHIFT] または [X] キー （4 Rep以上で跳躍突進！）", GetColor(255, 110, 50), font16);
        DrawStringToHandle(panelX + 30, panelY + 157, "・視点: ステージ全体俯瞰固定ビュー", cyan, font16);
        DrawStringToHandle(panelX + 30, panelY + 185, "※ 筋トレ失敗で5秒間筋肉痛（停止） / 15秒放置で筋肉減衰", GetColor(255, 120, 120), font13);

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
                DrawFormatStringToHandle(24, 62, GetColor(255, 150, 20), font16, "猫速度: %.1f [Lv.%d (飛びつき可! 残り%.1fs)]", speed, reps, player->GetPumpDecayRemainingSeconds());
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

        if (player && player->GetRepCount() >= 4 && player->GetMuscleState() != MuscleState::Soreness) {
            DrawFormatStringToHandle(20, Config::SCREEN_HEIGHT - 72, GetColor(255, 230, 80), font16, "★ 飛びつき: [SHIFT] または [X] (Lv.%d 跳躍突進！) | タックル: [E] (ネズミ気絶！壁激突注意)", player->GetRepCount());
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 50, "移動: WASD キー / [マウス左クリック長押し]", white, font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 28, "筋トレ: [SPACE] でさらにRep追加！ (15秒放置で0Rep / 失敗で5秒移動不可)", GetColor(255, 220, 100), font13);
        }
        
        else
        {
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 72, "タックル: [E] キー (ネズミを気絶スタン！壁や家具激突で猫スタン)", GetColor(255, 200, 80), font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 50, "移動: WASD キー / [マウス左クリック長押し]", white, font16);
            DrawStringToHandle(20, Config::SCREEN_HEIGHT - 28, "筋トレ: [SPACE] でRep追加！ (15秒放置で0Rep / 失敗で5秒移動不可)", GetColor(255, 220, 100), font13);
        }

        // デバッグチートパネル表示
        auto cheatPlayer = m_objManager.GetPlayer();
        // F1を押したらON（どの状態でも表示を出す）
        if (m_cheatEnabled)
        {
            // 半透明背景
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
            DrawBox(Config::SCREEN_WIDTH - 380, 40, Config::SCREEN_WIDTH - 5, 220, GetColor(15, 15, 30), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(Config::SCREEN_WIDTH - 380, 40, Config::SCREEN_WIDTH - 5, 220, GetColor(255, 80, 80), FALSE);

            DrawStringToHandle(Config::SCREEN_WIDTH - 370, 48, "★ DEBUG CHEAT MODE ON ★", GetColor(255, 80, 80), font16);

            bool tf  = m_objManager.IsTimeFrozen();
            bool ncd = cheatPlayer ? cheatPlayer->IsNoCooldown() : false;
            int  rep = cheatPlayer ? cheatPlayer->GetRepCount() : 0;

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 76,  GetColor(180, 230, 255), font16,
                "[1] 通常ネズミ追加  [2] 高速ネズミ追加");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 100, GetColor(180, 230, 255), font16,
                "[3] Rep+1  現在Rep: %d %s", rep, (rep >= 15) ? "(能力MAX)" : "");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 124, tf ? GetColor(255, 240, 60) : GetColor(180, 230, 255), font16,
                "[4] 時間停止: %s", tf ? "ON" : "OFF");

            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 148, ncd ? GetColor(255, 240, 60) : GetColor(180, 230, 255), font16,
                "[5] クールタイム無効: %s", ncd ? "ON (スタンも無効)" : "OFF");

            DrawStringToHandle(Config::SCREEN_WIDTH - 370, 172, "[F1] チートOFF", GetColor(255, 120, 120), font16);
            
            DrawFormatStringToHandle(Config::SCREEN_WIDTH - 370, 196, GetColor(140, 200, 140), font13,
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
}