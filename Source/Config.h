#pragma once

// ゲーム設定・定数定義
namespace Config
{
    // 画面設定
    constexpr int SCREEN_WIDTH  = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int COLOR_DEPTH   = 32;
    constexpr const char* TITLE = "MuscleCat 3D";

    // ゲームバランス設定
    constexpr int NORMAL_MOUSE_COUNT = 3; // 通常ネズミの配置数（合計5匹）
    constexpr int FAST_MOUSE_COUNT   = 2; // 高速逃走ネズミの配置数

    // 制限時間モード設定（false にすると元の経過タイム計測モードに戻せます）
    constexpr bool  USE_TIME_LIMIT_MODE = true;
    constexpr float TIME_LIMIT_SECONDS  = 120.0f; // 制限時間 120秒

    // 3Dステージ設定（広々としたジム空間）
    constexpr float STAGE_HALF_WIDTH  = 500.0f; // X方向の半幅 (-500 〜 +500)
    constexpr float STAGE_HALF_DEPTH  = 300.0f; // Z方向の半奥行 (-380 〜 +380)
    constexpr float WALL_HEIGHT       = 32.0f;  // 壁の高さ（視界を遮らないフェンス高）

    // カメラ設定（斜め上見下ろしアングル - モデルと周囲が見渡しやすい視点）
    constexpr float CAMERA_DISTANCE = 220.0f; // 猫からの距離（程よく見下ろせる引き具合）
    constexpr float CAMERA_HEIGHT   = 140.0f; // カメラの初期高さ
    constexpr float CAMERA_TARGET_OFFSET_Y = 20.0f; // 注視点のプレイヤー高さオフセット（猫の中心付近）

    // CSV管理ファイルパス設定
    constexpr const char* STATUS_CSV_PATH          = "Resource/CSV/Statu.csv";
    constexpr const char* POSITION_CSV_PATH        = "Resource/CSV/Position.csv";
    constexpr const char* STATUS_CSV_SOURCE_PATH   = "CSV元ファイル/Statu.csv";
    constexpr const char* POSITION_CSV_SOURCE_PATH = "CSV元ファイル/Position.csv";

    // 距離・スケール目測基準（猫の基準身長 = 1.8m としたときのワールド座標換算）
    constexpr float CAT_BASE_HEIGHT_METERS = 1.8f;                                           // 猫の基準身長 (1.8m)
    constexpr float CAT_BASE_HEIGHT_UNITS  = 104.496f;                                       // 猫モデルの基準高さ (435.4 * 0.24)
    constexpr float WORLD_UNITS_PER_METER  = CAT_BASE_HEIGHT_UNITS / CAT_BASE_HEIGHT_METERS; // 1mあたりのワールド単位 (約58.05u)
    constexpr float CUBE_10M_SIZE_UNITS    = WORLD_UNITS_PER_METER * 10.0f;                  // 10mのワールド単位 (約580.53u)
}