#include "DxLib.h"
#include "GameManager.h"
#include "Player3D.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "EffectManager.h"
#include "CustomEffectManager.h"
#include "CsvManager.h"
#include "Config.h"
#include <cstdlib>
#include <ctime>
#include <fstream>

// メイン関数 (WinMain)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetCurrentDirectoryA("d:\\MuscleCat");
    srand(static_cast<unsigned int>(time(nullptr)));

    // 文字コード形式をUTF-8に設定
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

    // ウィンドウモード設定
    ChangeWindowMode(TRUE);

    // 画面解像度とカラービット数設定
    SetGraphMode(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, Config::COLOR_DEPTH);

    // ウィンドウタイトル設定
    SetMainWindowText(Config::TITLE);

    // ログ出力を有効化
    SetOutApplicationLogValidFlag(TRUE);

    // DxLibの初期化
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    // 描画先を裏画面に設定
    SetDrawScreen(DX_SCREEN_BACK);

    // ウィンドウが非アクティブでも安定動作を維持
    SetAlwaysRunFlag(TRUE);

    // 3D初期設定（Zバッファ、カリング）
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);
    SetUseBackCulling(TRUE);

    // CSV・フォント・モデル・エフェクトマネージャー初期化
    CsvManager::GetInstance().Init();
    FontManager::GetInstance().Init();
    ModelManager::GetInstance().Init();
    EffectManager::GetInstance().Init();
    CustomEffectManager::GetInstance().Init();

    // ゲームマネージャー初期化
    GameManager game;
    game.Init();

    // 60FPSフレーム調停用
    LONGLONG prevFrameTime = GetNowHiPerformanceCount();
    const LONGLONG targetFrameMicroseconds = 1000000 / 60; // 16666μs

    // メインループ
    int autoTestFrame = 0;
    const char* fullCmd = GetCommandLineA();
    bool isAutoTest = (std::strstr(fullCmd, "--test") != nullptr);
    std::ofstream testLog;
    if (isAutoTest)
    {
        testLog.open("test_result.txt");
        testLog << "=== AUTOTEST SUITE START ===" << std::endl;
    }

    auto LogTest = [&](const std::string& msg) {
        AppLogAdd("%s", msg.c_str());
        if (testLog.is_open())
        {
            testLog << msg << std::flush;
        }
    };

    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        if (game.ShouldQuit())
        {
            break;
        }

        LONGLONG processStartTime = GetNowHiPerformanceCount();

        // 自動テストモード更新
        if (isAutoTest)
        {
            autoTestFrame++;
            if (autoTestFrame == 8)
            {
                game.SetShowHowToPlay(true);
            }
            else if (autoTestFrame == 14)
            {
                game.SetShowHowToPlay(false);
            }
            else if (autoTestFrame == 25)
            {
                // マップ1（House）に切り替え
                game.SwitchSelectedMap(ModelConfig::MapType::House);
                LogTest("AutoTest: Switched to MAP 1: House\n");
            }
            else if (autoTestFrame == 40)
            {
                // マップ1でゲーム開始
                game.StartGame();
                LogTest("AutoTest: Started game on MAP 1: House\n");
                AppLogAdd("AutoTest: Started game on MAP 1: House\n");
            }
            else if (autoTestFrame > 40 && autoTestFrame <= 65)
            {
                // 部屋の境目（画像で光っていた重なり領域方向）へ歩行移動
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    VECTOR pos = p->GetPos();
                    pos.x += 12.0f;
                    pos.z += 10.0f;
                    VECTOR pushNorm;
                    bool hit = game.GetObjManager().GetStage().ResolveWallCollision(pos, p->GetRadius(), &pushNorm);
                    float gy = 0.0f;
                    if (game.GetObjManager().GetStage().GetGroundHeight(pos, gy))
                    {
                        pos.y = gy;
                        p->SetGroundY(gy);
                    }
                    p->SetPos(pos);
                    if (autoTestFrame % 5 == 0)
                    {
                        AppLogAdd("AutoTest MAP 1 Move: Pos=(%.1f, %.1f, %.1f), HitWall=%s (push=%.2f, %.2f)\n",
                            pos.x, pos.y, pos.z, (hit ? "YES" : "NO"), pushNorm.x, pushNorm.z);
                    }
                }
            }
            else if (autoTestFrame == 70)
            {
                // タイトルに戻る
                game.SetupTitle();
                AppLogAdd("AutoTest: Returned to Title\n");
            }
            else if (autoTestFrame == 80)
            {
                // マップ2（NEW MAP）に切り替え
                game.SwitchSelectedMap(ModelConfig::MapType::NewMap);
                AppLogAdd("AutoTest: Switched to MAP 2: NEW MAP\n");
            }
            else if (autoTestFrame == 90)
            {
                // マップ2でゲーム開始
                game.StartGame();
                AppLogAdd("AutoTest: Started game on MAP 2: NEW MAP\n");
            }
            else if (autoTestFrame == 95)
            {
                // テスト1: 東側突出プラットフォーム (X=2000, Z=650) から東外側へ歩行
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    p->SetPos(VGet(2000.0f, 20.0f, 650.0f));
                }
            }
            else if (autoTestFrame > 95 && autoTestFrame <= 110)
            {
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    VECTOR pos = p->GetPos();
                    pos.x += 6.0f; // 東側外周へ向かって前進
                    VECTOR prevPos = p->GetPos();
                    VECTOR pushNorm;
                    bool wallHit = game.GetObjManager().GetStage().ResolveWallCollision(pos, p->GetRadius(), &pushNorm);
                    float gy = 0.0f;
                    bool groundHit = game.GetObjManager().GetStage().GetGroundHeight(pos, gy);
                    if (!groundHit)
                    {
                        pos = prevPos; // 足場外への踏み出し防止
                    }
                    else
                    {
                        pos.y = gy;
                    }
                    p->SetPos(pos);
                    if (autoTestFrame == 110)
                    {
                        char buf[256];
                        snprintf(buf, sizeof(buf), "AutoTest PROTRUSION EAST: FinalPos=(%.1f, %.1f, %.1f), WallHit=%s, HasGround=%s, BlockedInside=%s\n",
                            pos.x, pos.y, pos.z, (wallHit ? "YES" : "NO"), (groundHit ? "YES" : "NO"),
                            (pos.x <= 2060.0f ? "PASS" : "FAIL"));
                        LogTest(buf);
                    }
                }
            }
            else if (autoTestFrame == 115)
            {
                // テスト2: 西側突出スタブ (X=-1220, Z=-1200) から外側へ歩行
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    p->SetPos(VGet(-1220.0f, 20.0f, -1200.0f));
                }
            }
            else if (autoTestFrame > 115 && autoTestFrame <= 130)
            {
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    VECTOR pos = p->GetPos();
                    pos.x += 6.0f; // 行き止まり端から外へ向かって前進
                    VECTOR prevPos = p->GetPos();
                    VECTOR pushNorm;
                    bool wallHit = game.GetObjManager().GetStage().ResolveWallCollision(pos, p->GetRadius(), &pushNorm);
                    float gy = 0.0f;
                    bool groundHit = game.GetObjManager().GetStage().GetGroundHeight(pos, gy);
                    if (!groundHit)
                    {
                        pos = prevPos;
                    }
                    else
                    {
                        pos.y = gy;
                    }
                    p->SetPos(pos);
                    if (autoTestFrame == 130)
                    {
                        char buf[256];
                        snprintf(buf, sizeof(buf), "AutoTest PROTRUSION WEST: FinalPos=(%.1f, %.1f, %.1f), WallHit=%s, HasGround=%s, BlockedInside=%s\n",
                            pos.x, pos.y, pos.z, (wallHit ? "YES" : "NO"), (groundHit ? "YES" : "NO"),
                            (pos.x <= -1200.0f ? "PASS" : "FAIL"));
                        LogTest(buf);
                    }
                }
            }
            else if (autoTestFrame == 135)
            {
                // テスト3: 南側大通路終端 (X=400, Z=-1800) から南外側へ歩行
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    p->SetPos(VGet(400.0f, 20.0f, -1800.0f));
                }
            }
            else if (autoTestFrame > 135 && autoTestFrame <= 150)
            {
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    VECTOR pos = p->GetPos();
                    pos.z -= 6.0f; // 南端から外へ向かって前進
                    VECTOR prevPos = p->GetPos();
                    VECTOR pushNorm;
                    bool wallHit = game.GetObjManager().GetStage().ResolveWallCollision(pos, p->GetRadius(), &pushNorm);
                    float gy = 0.0f;
                    bool groundHit = game.GetObjManager().GetStage().GetGroundHeight(pos, gy);
                    if (!groundHit)
                    {
                        pos = prevPos;
                    }
                    else
                    {
                        pos.y = gy;
                    }
                    p->SetPos(pos);
                    if (autoTestFrame == 150)
                    {
                        char buf[256];
                        snprintf(buf, sizeof(buf), "AutoTest PROTRUSION SOUTH: FinalPos=(%.1f, %.1f, %.1f), WallHit=%s, HasGround=%s, BlockedInside=%s\n",
                            pos.x, pos.y, pos.z, (wallHit ? "YES" : "NO"), (groundHit ? "YES" : "NO"),
                            (pos.z >= -1830.0f ? "PASS" : "FAIL"));
                        LogTest(buf);
                    }
                }
            }
            else if (autoTestFrame == 155)
            {
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    // Lv.0（初期状態）では攻撃アクションがロックされていることを検証
                    bool canTackleLv0 = p->CanTackle();
                    bool canPounceLv0 = p->CanPounce();
                    bool passLv0 = (!canTackleLv0 && !canPounceLv0);

                    // Lv.4（4 Rep到達時）で攻撃アクションが解放されることを検証
                    p->AddRep(4);
                    bool canTackleLv4 = p->CanTackle();
                    bool canPounceLv4 = p->CanPounce();
                    bool passLv4 = (canTackleLv4 && canPounceLv4);

                    char buf[256];
                    snprintf(buf, sizeof(buf), "AutoTest ATTACKS: Lv0_Locked=%s, Lv4_Unlocked=%s (Overall=%s)\n",
                        (passLv0 ? "PASS" : "FAIL"), (passLv4 ? "PASS" : "FAIL"),
                        (passLv0 && passLv4 ? "PASS" : "FAIL"));
                    LogTest(buf);
                }
            }
            else if (autoTestFrame == 158)
            {
                // テスト: 家具撤去確認（Obstacle 数が 0 であること）
                int obstacleCount = static_cast<int>(game.GetObjManager().GetObstacles().size());
                bool noObstacles = (obstacleCount == 0);

                // テスト: ネズミの壁当たり判定確認（壁位置 X=2060 に向けてネズミを置いた時、壁を検出し内側へ押し戻すこと）
                const auto& stage = game.GetObjManager().GetStage();
                VECTOR testMousePos = VGet(2058.0f, 20.0f, 650.0f);
                float mRadius = 12.0f;
                VECTOR pushNormal = VGet(0.0f, 0.0f, 0.0f);
                bool wallHit = stage.ResolveWallCollision(testMousePos, mRadius, &pushNormal);
                bool pushBackOk = (testMousePos.x <= 2049.0f);

                char buf[256];
                snprintf(buf, sizeof(buf), "AutoTest FURNITURE & MOUSE WALL: NoObstacles=%s (count=%d), MouseWallCollision=%s, PushedInside=%s\n",
                    (noObstacles ? "PASS" : "FAIL"), obstacleCount,
                    (wallHit ? "PASS" : "FAIL"),
                    (pushBackOk ? "PASS" : "FAIL"));
                LogTest(buf);
            }
            else if (autoTestFrame == 162)
            {
                // テスト: マウス視点感度調整の検証
                auto& cam = game.GetCamera();
                float initialSens = cam.GetMouseSensitivity();
                bool defaultPass = (std::abs(initialSens - 1.0f) < 0.01f);

                cam.AdjustMouseSensitivity(+0.1f);
                bool adjustPass = (std::abs(cam.GetMouseSensitivity() - 1.1f) < 0.01f);

                cam.SetMouseSensitivity(10.0f);
                bool maxClampPass = (std::abs(cam.GetMouseSensitivity() - 3.0f) < 0.01f);

                cam.SetMouseSensitivity(-5.0f);
                bool minClampPass = (std::abs(cam.GetMouseSensitivity() - 0.2f) < 0.01f);

                cam.SetMouseSensitivity(1.5f);
                game.StartGame(); // リトライ
                bool persistPass = (std::abs(cam.GetMouseSensitivity() - 1.5f) < 0.01f);

                bool allSensPass = defaultPass && adjustPass && maxClampPass && minClampPass && persistPass;

                char buf[256];
                snprintf(buf, sizeof(buf), "AutoTest MOUSE SENSITIVITY: Default=%.1f(%s), Adjust=%s, ClampMax=%s, ClampMin=%s, PersistAfterRetry=%s (Overall=%s)\n",
                    initialSens, (defaultPass ? "PASS" : "FAIL"),
                    (adjustPass ? "PASS" : "FAIL"),
                    (maxClampPass ? "PASS" : "FAIL"),
                    (minClampPass ? "PASS" : "FAIL"),
                    (persistPass ? "PASS" : "FAIL"),
                    (allSensPass ? "PASS" : "FAIL"));
                LogTest(buf);
            }
            else if (autoTestFrame == 163)
            {
                // テスト: CSV読み込み（Statu.csv / Position.csv）の検証
                const auto& csv = CsvManager::GetInstance();
                const auto& catStat = csv.GetCatStatus();
                const auto& mouseStat = csv.GetMouseStatus();
                const auto& catPos = csv.GetCatPositions();
                const auto& mousePos = csv.GetMousePositions();

                bool catStatOk = (std::abs(catStat.baseSpeed - 4.0f) < 0.01f &&
                                  std::abs(catStat.speedGrowthPerUnit - 0.1f) < 0.01f &&
                                  std::abs(catStat.maxSpeed - 5.5f) < 0.01f);
                bool mouseStatOk = (std::abs(mouseStat.baseSpeed - 4.2f) < 0.01f &&
                                    std::abs(mouseStat.speedGrowthPerUnit - 0.05f) < 0.01f &&
                                    std::abs(mouseStat.maxSpeed - 4.7f) < 0.01f);
                bool posOk = (!catPos.empty() && mousePos.size() == 8);

                auto p = game.GetObjManager().GetPlayer();
                bool playerSpdOk = false;
                if (p)
                {
                    float spd0 = p->CalculateSpeedForRep(0);
                    float spd15 = p->CalculateSpeedForRep(15);
                    playerSpdOk = (std::abs(spd0 - 4.0f) < 0.01f && std::abs(spd15 - 5.5f) < 0.01f);
                }

                char buf[256];
                snprintf(buf, sizeof(buf), "AutoTest CSV MANAGER: CatStat=%s(%.1f,%.2f,%.1f), MouseStat=%s(%.1f,%.2f,%.1f), PosCount=%s(Cat:%d,Mouse:%d), PlayerSpdCalc=%s\n",
                    (catStatOk ? "PASS" : "FAIL"), catStat.baseSpeed, catStat.speedGrowthPerUnit, catStat.maxSpeed,
                    (mouseStatOk ? "PASS" : "FAIL"), mouseStat.baseSpeed, mouseStat.speedGrowthPerUnit, mouseStat.maxSpeed,
                    (posOk ? "PASS" : "FAIL"), static_cast<int>(catPos.size()), static_cast<int>(mousePos.size()),
                    (playerSpdOk ? "PASS" : "FAIL"));
                LogTest(buf);
            }
            else if (autoTestFrame == 164)
            {
                // F1デバッグモード（10m×10m×10m目測キューブ・速度数値・2Dミニマップ）の検証
                game.SetPaused(false);
                game.SetCheatEnabled(true);
                game.SetDebugWideCamera(true);
                auto p = game.GetObjManager().GetPlayer();
                bool f1ScaleOk = (std::abs(Config::CAT_BASE_HEIGHT_METERS - 1.8f) < 0.001f &&
                                  std::abs(Config::CUBE_10M_SIZE_UNITS - 580.5333f) < 0.1f);
                bool f1SpeedOk = (p != nullptr && p->GetCurrentSpeed() >= 4.0f);
                char buf[256];
                snprintf(buf, sizeof(buf), "AutoTest F1 DEBUG OVERLAY: Enabled=%s, Scale10m=%.1fu(%s), PlayerSpeed=%.2f(%s)\n",
                    (game.IsCheatEnabled() ? "PASS" : "FAIL"),
                    Config::CUBE_10M_SIZE_UNITS, (f1ScaleOk ? "PASS" : "FAIL"),
                    (p ? p->GetCurrentSpeed() : 0.0f), (f1SpeedOk ? "PASS" : "FAIL"));
                LogTest(buf);
            }
            else if (autoTestFrame == 167)
            {
                game.SetDebugWideCamera(false);
                game.SetCheatEnabled(false);
                game.SetPaused(true, 2); // ポーズ画面（感度調整行を選択した状態）
            }
            else if (autoTestFrame > 172)
            {
                break;
            }
        }

        // 自動テスト用カメラオーバーライド
        if (isAutoTest)
        {
            if (autoTestFrame == 145)
            {
                SetCameraNearFar(10.0f, 30000.0f);
                SetCameraPositionAndTargetAndUpVec(VGet(150.0f, 4200.0f, -190.0f), VGet(150.0f, 0.0f, -190.0f), VGet(0.0f, 0.0f, 1.0f));
            }
            else if (autoTestFrame == 148)
            {
                SetCameraNearFar(10.0f, 30000.0f);
                SetCameraPositionAndTarget_UpVecY(VGet(150.0f, 2600.0f, -2800.0f), VGet(150.0f, 0.0f, -190.0f));
            }
        }

        // ゲームロジック更新
        game.Update();

        // 描画
        game.Draw();

        // 自動テストスクリーンショット保存（描画後に行う）
        if (isAutoTest)
        {
            if (autoTestFrame == 11)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_how_to_play.png");
                AppLogAdd("AutoTest: Saved screenshot_how_to_play.png\n");
            }
            else if (autoTestFrame == 18)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_title_map2.png");
                AppLogAdd("AutoTest: Saved screenshot_title_map2.png\n");
            }
            else if (autoTestFrame == 35)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_title_map1.png");
                AppLogAdd("AutoTest: Saved screenshot_title_map1.png\n");
            }
            else if (autoTestFrame == 60)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map1.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map1.png\n");
            }
            else if (autoTestFrame == 105)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map2.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map2.png\n");
            }
            else if (autoTestFrame == 140)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map2_slope.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map2_slope.png\n");
            }
            else if (autoTestFrame == 145)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_map2_topdown.png");
                AppLogAdd("AutoTest: Saved screenshot_map2_topdown.png\n");
            }
            else if (autoTestFrame == 148)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_map2_overview.png");
                AppLogAdd("AutoTest: Saved screenshot_map2_overview.png\n");
            }
            else if (autoTestFrame == 165)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_f1_debug.png");
                AppLogAdd("AutoTest: Saved screenshot_f1_debug.png\n");
            }
            else if (autoTestFrame == 168)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_pause_sensitivity.png");
                AppLogAdd("AutoTest: Saved screenshot_pause_sensitivity.png\n");
            }
        }

        // Pキーでスクリーンショット保存
        if (CheckHitKey(KEY_INPUT_P))
        {
            SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot.png");
        }

        LONGLONG processEndTime = GetNowHiPerformanceCount();
        game.SetFrameProcessTime(static_cast<float>(processEndTime - processStartTime) / 1000.0f);

        // 画面フリップ（垂直同期）
        ScreenFlip();

        // 60FPSフレーム調停
        while (GetNowHiPerformanceCount() - prevFrameTime < targetFrameMicroseconds)
        {
            LONGLONG remain = targetFrameMicroseconds - (GetNowHiPerformanceCount() - prevFrameTime);

            if (remain > 2000)
            {
                Sleep(1);
            }
            
            else
            {
                Sleep(0);
            }
        }

        prevFrameTime = GetNowHiPerformanceCount();
    }

    // リソース解放
    CustomEffectManager::GetInstance().Release();
    EffectManager::GetInstance().Release();
    FontManager::GetInstance().Release();
    ModelManager::GetInstance().Release();

    // DxLib終了処理
    DxLib_End();

    return 0;
}