// ============================================================================
// @file   Main.cpp
// @brief  アプリケーションのエントリーポイント（WinMain）およびメインループ制御
// ============================================================================
#include "DxLib.h"
#include "Config.h"
#include "FontManager.h"
#include "ModelManager.h"
#include "EffectManager.h"
#include "CustomEffectManager.h"
#include "CsvManager.h"
#include "GameManager.h"
#include <ctime>
#include <cstdlib>

// ============================================================================
// WinMain 関数：Windowsアプリケーションのエントリーポイント
// DxLib・各種マネージャの初期化、60FPSメインループ、終了時のリソース解放を行う
// ============================================================================
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // 乱数シードの初期化
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // ------------------------------------------------------------------------
    // 1. DxLib 初期設定（DxLib_Init 呼び出し前に設定が必要な項目）
    // ------------------------------------------------------------------------
    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);                       // ソースコード内のUTF-8文字列（日本語・絵文字等）を正しく描画
    ChangeWindowMode(TRUE);                                             // ウィンドウモードで起動
    SetGraphMode(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, Config::COLOR_BIT); // 解像度 1280x720, 32bitカラー
    SetMainWindowText(Config::TITLE);                                   // ウィンドウタイトルを「マッスルねこ」に設定
    SetUseDirect3DVersion(DX_DIRECT3D_11);                              // Effekseer連携のためDirect3D 11を使用
    SetAlwaysRunFlag(TRUE);                                             // 非アクティブ時も処理を継続

    // DxLib本体の初期化
    if (DxLib_Init() == -1)
    {
        return -1;
    }

    // 裏画面描画（ダブルバッファリング）を有効化
    SetDrawScreen(DX_SCREEN_BACK);

    // ------------------------------------------------------------------------
    // 2. 各種シングルトンマネージャおよびゲーム進行管理クラスの初期化
    // ------------------------------------------------------------------------
    CsvManager::GetInstance().Init();           // ステータス(Statu.csv)・配置座標(Position.csv)の読み込み
    FontManager::GetInstance().Init();          // 各サイズ（13〜96pt）のフォント生成
    ModelManager::GetInstance().Init();         // 3Dモデル管理テーブルの初期化
    EffectManager::GetInstance().Init();        // Effekseerエフェクトの初期化・リソース読み込み
    CustomEffectManager::GetInstance().Init();  // 3Dプロシージャル炎・煙・雷撃・地面えぐりエフェクトの初期化

    GameManager game;
    game.Init();                                // ステージ構築・タイトル画面の初期セットアップ

    // ------------------------------------------------------------------------
    // 3. メインゲームループ（毎フレームの更新・描画・時間計測）
    // ------------------------------------------------------------------------
    while (ProcessMessage() == 0 && !game.ShouldQuit())
    {
        // フレーム開始時刻を記録（処理時間計測用）
        LONGLONG frameStartUs = GetNowHiPerformanceCount();

        // 画面とZバッファをクリア
        ClearDrawScreen();

        // ゲーム状態の更新と描画
        game.Update();
        game.Draw();

        // 1フレームあたりのCPU処理時間（ミリ秒）を算出して記録
        LONGLONG frameEndUs = GetNowHiPerformanceCount();
        game.SetFrameProcessTime(static_cast<float>(frameEndUs - frameStartUs) / 1000.0f);

        // 裏画面の内容を表画面へ反映（垂直同期）
        ScreenFlip();
    }

    // ------------------------------------------------------------------------
    // 4. 終了時のリソース一括解放（初期化の逆順で安全に破棄）
    // ------------------------------------------------------------------------
    CustomEffectManager::GetInstance().Release();
    EffectManager::GetInstance().Release();
    ModelManager::GetInstance().Release();
    FontManager::GetInstance().Release();
    DxLib_End();

    return 0;
}
