#include "DxLib.h"
#include "GameManager.h"
#include "Player3D.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "EffectManager.h"
#include "Config.h"
#include <cstdlib>
#include <ctime>

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

    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE))
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
                AppLogAdd("AutoTest: Switched to MAP 1: House\n");
            }
            else if (autoTestFrame == 40)
            {
                // マップ1でゲーム開始
                game.StartGame();
                AppLogAdd("AutoTest: Started game on MAP 1: House\n");
            }
            else if (autoTestFrame == 65)
            {
                // タイトルに戻る
                game.SetupTitle();
                AppLogAdd("AutoTest: Returned to Title\n");
            }
            else if (autoTestFrame == 75)
            {
                // マップ2（SlopeHills 15x）に切り替え
                game.SwitchSelectedMap(ModelConfig::MapType::SlopeHills);
                AppLogAdd("AutoTest: Switched to MAP 2: Slope Hills 15x\n");
            }
            else if (autoTestFrame == 85)
            {
                // マップ2でゲーム開始
                game.StartGame();
                AppLogAdd("AutoTest: Started game on MAP 2: Slope Hills 15x\n");
            }
            else if (autoTestFrame > 85 && autoTestFrame <= 140)
            {
                // 北東スロープに向かって登攀移動
                auto p = game.GetObjManager().GetPlayer();
                if (p)
                {
                    VECTOR pos = p->GetPos();
                    pos.x += 45.0f;
                    pos.z += 45.0f;
                    float gy = 0.0f;
                    if (game.GetObjManager().GetStage().GetGroundHeight(pos, gy))
                    {
                        pos.y = gy;
                        p->SetGroundY(gy);
                    }
                    p->SetPos(pos);
                    if (autoTestFrame % 10 == 0)
                    {
                        AppLogAdd("AutoTest MAP 2 Climb: Pos=(%.1f, %.1f, %.1f), GroundY=%.1f\n",
                            pos.x, pos.y, pos.z, gy);
                    }
                }
            }
            else if (autoTestFrame > 145)
            {
                break;
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
            else if (autoTestFrame == 58)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map1.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map1.png\n");
            }
            else if (autoTestFrame == 100)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map2.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map2.png\n");
            }
            else if (autoTestFrame == 140)
            {
                SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "screenshot_game_map2_slope.png");
                AppLogAdd("AutoTest: Saved screenshot_game_map2_slope.png\n");
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