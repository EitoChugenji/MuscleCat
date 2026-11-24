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
    constexpr const char* TITLE = "マッスルにゃんこ";  // ウィンドウタイトル名

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

    // ------------------------------------------------------------------------
    // ★ コンテスト向け演出強化スイッチ（false にすると各演出を個別に無効化可能）
    // ------------------------------------------------------------------------
    constexpr bool ENABLE_QTE_PERFECT_ZONE    = true; // ① 筋トレQTEの「PERFECT（金ゾーン）」判定（+2Lv・稲妻・ptボーナス）
    constexpr bool ENABLE_CATCH_FINISH_SLOWMO = true; // ② 捕獲時のヒットストップ強化 ＆ ラスト1匹のK.O.スロー＆カメラズーム演出
    constexpr bool ENABLE_STAGE_START_INTRO   = true; // ③ ステージ開始時の「3・2・1・MUSCLE GO!!」開幕カメラ＆カウントダウン演出

    // ------------------------------------------------------------------------
    // ゲームプレイ画面 HUD のカスタム PNG 画像・座標設定
    // Resource/UI/ に同名PNGファイルを置くだけで自動的に差し替え描画される
    // （W または H に 0 を指定すると、PNG画像の元サイズそのまま DrawGraph で描画）
    // ------------------------------------------------------------------------
    namespace HudUI
    {
        // 左上：ステータスHUD背景（レベル・残りネズミ・残り時間・速度）
        constexpr const char* STATUS_PNG_PATH = "Resource/UI/hud_status.png";
        constexpr int         STATUS_X        = 14;
        constexpr int         STATUS_Y        = 14;
        constexpr int         STATUS_W        = 380; // 0で画像元サイズ
        constexpr int         STATUS_H        = 84;  // 0で画像元サイズ

        // 右下：操作ガイド背景（攻撃スキル・筋トレ・移動操作）
        constexpr const char* GUIDE_PNG_PATH  = "Resource/UI/hud_guide.png";
        constexpr int         GUIDE_X         = 620; // SCREEN_WIDTH(1280) - 640 - 20
        constexpr int         GUIDE_Y         = 628; // SCREEN_HEIGHT(720) - 72 - 20
        constexpr int         GUIDE_W         = 640; // 0で画像元サイズ
        constexpr int         GUIDE_H         = 72;  // 0で画像元サイズ

        // 任意：画面追加オーバーレイ用PNG（キャラ立ち絵・枠装飾などを自由座標に配置可能）
        constexpr const char* OVERLAY_PNG_PATH = "Resource/UI/hud_overlay.png";
        constexpr int         OVERLAY_X        = 0;
        constexpr int         OVERLAY_Y        = 0;
        constexpr int         OVERLAY_W        = 0;  // 0で画像元サイズ
        constexpr int         OVERLAY_H        = 0;  // 0で画像元サイズ

        // HUDの上下浮遊アニメーション（イラスト固定表示にしたい場合は false）
        constexpr bool        ENABLE_FLOAT_ANIM = true;
    }
}
