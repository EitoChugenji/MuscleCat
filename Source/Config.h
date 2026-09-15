#pragma once

// ============================================================================
// ゲーム設定・定数定義
// ============================================================================
namespace Config {
    // 画面設定
    constexpr int SCREEN_WIDTH  = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int COLOR_DEPTH   = 32;
    constexpr const char* TITLE = "マッスルねこ 3D (Muscle Cat 3D)";

    // ゲームバランス設定
    constexpr int NORMAL_MOUSE_COUNT = 7; // 通常ネズミの配置数
    constexpr int FAST_MOUSE_COUNT   = 4; // 高速逃走ネズミの配置数

    // 3Dステージ設定（広々としたジム空間）
    constexpr float STAGE_HALF_WIDTH  = 500.0f; // X方向の半幅 (-500 〜 +500)
    constexpr float STAGE_HALF_DEPTH  = 380.0f; // Z方向の半奥行 (-380 〜 +380)
    constexpr float WALL_HEIGHT       = 32.0f;  // 壁の高さ（視界を遮らないフェンス高）

    // カメラ設定（三人称視点 TPS - 引き気味の広角設定）
    constexpr float CAMERA_DISTANCE = 175.0f; // プレイヤーからの追従距離（引き気味）
    constexpr float CAMERA_HEIGHT   = 80.0f;  // プレイヤーからのカメラ高さ
    constexpr float CAMERA_TARGET_OFFSET_Y = 16.0f; // 注視点のプレイヤー高さオフセット
}
