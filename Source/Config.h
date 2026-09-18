#pragma once

// ゲーム設定・定数定義
namespace Config
{
    // 画面設定
    constexpr int SCREEN_WIDTH  = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int COLOR_DEPTH   = 32;
    constexpr const char* TITLE = "マッスルねこ 3D (Muscle Cat 3D)";

    // ゲームバランス設定
    constexpr int NORMAL_MOUSE_COUNT = 3; // 通常ネズミの配置数（合計5匹）
    constexpr int FAST_MOUSE_COUNT   = 2; // 高速逃走ネズミの配置数

    // 3Dステージ設定（広々としたジム空間）
    constexpr float STAGE_HALF_WIDTH  = 500.0f; // X方向の半幅 (-500 〜 +500)
    constexpr float STAGE_HALF_DEPTH  = 300.0f; // Z方向の半奥行 (-380 〜 +380)
    constexpr float WALL_HEIGHT       = 32.0f;  // 壁の高さ（視界を遮らないフェンス高）

    // カメラ設定（ステージ全体俯瞰・固定見下ろし視点 - 程よい近さ）
    constexpr float CAMERA_DISTANCE = 500.0f; // ステージ中心からの手前オフセット距離
    constexpr float CAMERA_HEIGHT   = 480.0f; // 上空からの高さ
    constexpr float CAMERA_TARGET_OFFSET_Y = 0.0f;
}
