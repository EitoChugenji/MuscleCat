#include "DxLib.h"
#include "GameManager.h"
#include "Player3D.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "EffectManager.h"
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

    // フォント・モデル・エフェクトマネージャー初期化
    FontManager::GetInstance().Init();
    ModelManager::GetInstance().Init();
    EffectManager::GetInstance().Init();

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
            if (autoTestFrame == 25)
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
                    bool canTackle = p->CanTackle();
                    bool canPounce = p->CanPounce();
                    char buf[256];
                    snprintf(buf, sizeof(buf), "AutoTest ATTACKS LV0: CanTackle=%s, CanPounce=%s\n",
                        (canTackle ? "PASS" : "FAIL"), (canPounce ? "PASS" : "FAIL"));
                    LogTest(buf);
                }
            }
            else if (autoTestFrame > 160)
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
            if (autoTestFrame == 18)
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
    EffectManager::GetInstance().Release();
    FontManager::GetInstance().Release();
    ModelManager::GetInstance().Release();

    // DxLib終了処理
    DxLib_End();

    return 0;
}