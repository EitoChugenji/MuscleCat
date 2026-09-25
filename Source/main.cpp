#include "DxLib.h"
#include "GameManager.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "EffectManager.h"
#include "Config.h"
#include <cstdlib>
#include <ctime>

// メイン関数 (WinMain)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    srand(static_cast<unsigned int>(time(nullptr)));

    // 文字コード形式をUTF-8に設定
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

    // ウィンドウモード設定
    ChangeWindowMode(TRUE);

    // 画面解像度とカラービット数設定
    SetGraphMode(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, Config::COLOR_DEPTH);

    // ウィンドウタイトル設定
    SetMainWindowText(Config::TITLE);

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
    while (ProcessMessage() == 0 && ClearDrawScreen() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE))
        {
            break;
        }

        LONGLONG processStartTime = GetNowHiPerformanceCount();

        // ゲームロジック更新
        game.Update();

        // 描画
        game.Draw();

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
