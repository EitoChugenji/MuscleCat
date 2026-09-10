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
    constexpr int NORMAL_MOUSE_COUNT = 5; // 通常ネズミの配置数
    constexpr int FAST_MOUSE_COUNT   = 3; // 高速逃走ネズミの配置数

    // 3Dステージ設定（コンパクトなジム空間）
    constexpr float STAGE_HALF_WIDTH  = 260.0f; // X方向の半幅 (-260 〜 +260)
    constexpr float STAGE_HALF_DEPTH  = 200.0f; // Z方向の半奥行 (-200 〜 +200)
    constexpr float WALL_HEIGHT       = 60.0f;  // 壁の高さ

    // カメラ設定（三人称視点 TPS）
    constexpr float CAMERA_DISTANCE = 110.0f; // プレイヤーからの追従距離
    constexpr float CAMERA_HEIGHT   = 55.0f;  // プレイヤーからのカメラ高さ
    constexpr float CAMERA_TARGET_OFFSET_Y = 14.0f; // 注視点のプレイヤー高さオフセット
}
