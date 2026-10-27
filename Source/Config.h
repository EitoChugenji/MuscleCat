// ============================================================================
// @file   Config.h
// @brief  ゲーム全体の設定定数（画面解像度、ステージ寸法、ネズミ数、制限時間等）
// ============================================================================
#pragma once

// ============================================================================
// ゲーム基本設定定数名前空間
// ============================================================================
namespace Config
{
    // ------------------------------------------------------------------------
    // ウィンドウ・画面解像度設定
    // ------------------------------------------------------------------------
    constexpr int SCREEN_WIDTH  = 1280;            // 画面横幅（ピクセル）
    constexpr int SCREEN_HEIGHT = 720;             // 画面縦幅（ピクセル）
    constexpr int COLOR_BIT     = 32;              // カラービット数
    constexpr const char* TITLE = "マッスルねこ";  // ウィンドウタイトル名

    // ------------------------------------------------------------------------
    // 3Dステージ基準寸法（フォールバック描画および初期境界の基準値）
    // ------------------------------------------------------------------------
    constexpr float STAGE_HALF_WIDTH = 450.0f;     // ステージX軸方向の半径（中心から壁まで）
    constexpr float STAGE_HALF_DEPTH = 450.0f;     // ステージZ軸方向の半径（中心から壁まで）
    constexpr float WALL_HEIGHT      = 90.0f;      // 外壁の基準高さ

    // ------------------------------------------------------------------------
    // ワールド空間の距離換算定数（F1デバッグ目測・速度メートル換算用）
    // プレイヤー猫の基本身長（cat.mv1 全高 435.4 × スケール 0.24 = 104.496 units）を
    // 1.8m と定義し、1m あたりのワールド単位数を算出している
    // ------------------------------------------------------------------------
    constexpr float CAT_BASE_HEIGHT_UNITS  = 104.496f;                                      // 猫の基本身長（ワールド単位）
    constexpr float CAT_BASE_HEIGHT_METERS = 1.8f;                                          // 猫の基本身長（メートル換算）
    constexpr float WORLD_UNITS_PER_METER  = CAT_BASE_HEIGHT_UNITS / CAT_BASE_HEIGHT_METERS; // 1m = 約58.0533 units
    constexpr float CUBE_10M_SIZE_UNITS    = WORLD_UNITS_PER_METER * 10.0f;                  // 10m目測キューブの1辺長（約580.5333 units）

    // ------------------------------------------------------------------------
    // 初期スポーンさせるネズミの数
    // ------------------------------------------------------------------------
    constexpr int NORMAL_MOUSE_COUNT = 5;          // 通常ネズミの初期出現数
    constexpr int FAST_MOUSE_COUNT   = 3;          // 高速ネズミの初期出現数

    // ------------------------------------------------------------------------
    // 外部CSV設定ファイルのパス定義
    // 実行フォルダ側（Resource/CSV）とソースツリー側（Source/CSV）の両方を探索し、
    // 更新日時が新しい方を優先して読み込む（CsvManager::ResolveCsvFilePath）
    // ------------------------------------------------------------------------
    constexpr const char* STATUS_CSV_PATH          = "Resource/CSV/Statu.csv";
    constexpr const char* STATUS_CSV_SOURCE_PATH   = "Source/CSV/Statu.csv";
    constexpr const char* POSITION_CSV_PATH        = "Resource/CSV/Position.csv";
    constexpr const char* POSITION_CSV_SOURCE_PATH = "Source/CSV/Position.csv";

    // ------------------------------------------------------------------------
    // ゲームルール：制限時間モード設定
    // USE_TIME_LIMIT_MODE を false に変更すると、従来の経過タイム計測モードに戻る
    // ------------------------------------------------------------------------
    constexpr bool  USE_TIME_LIMIT_MODE = true;    // true: 制限時間制 / false: 従来の経過タイム制
    constexpr float TIME_LIMIT_SECONDS  = 120.0f;  // 制限時間（120秒 = 2分）
}
