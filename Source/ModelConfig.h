// ============================================================================
// @file   ModelConfig.h
// @brief  3Dモデルアセットのファイルパス・スケール・回転補正・マップ種別の定義
// ============================================================================
#pragma once

// ============================================================================
// 3Dモデル設定名前空間
// 指定したパスに3Dモデルファイル(.mv1)が存在する場合は自動的にモデル描画を行い、
// 存在しない場合は ModelManager 内のプロシージャル3Dプリミティブで代替描画する
// ============================================================================
namespace ModelConfig
{
    // ------------------------------------------------------------------------
    // プレイヤーモデル（マッスルねこ）設定
    // ------------------------------------------------------------------------
    constexpr const char* CAT_MODEL_PATH  = "Resource/Models/CatModel/cat.mv1";
    constexpr float       CAT_MODEL_SCALE = 0.24f;       // 基本スケール（筋トレレベルに応じてさらに拡大）
    constexpr float       CAT_MODEL_ROT_Y = 3.14159265f; // モデル正面方向の補正角（180度反転）

    // ------------------------------------------------------------------------
    // アクセサリモデル（スクワット中に両手で保持するバーベル）設定
    // ------------------------------------------------------------------------
    constexpr const char* BARBELL_MODEL_PATH = "Resource/Models/AccessoryModels/Barbell.mv1";
    constexpr float       BARBELL_BASE_SCALE = 18.0f;    // 基準スケール（猫の両手ボーン間隔に合わせて自動伸縮）

    // ------------------------------------------------------------------------
    // 通常ネズミモデル設定
    // ------------------------------------------------------------------------
    constexpr const char* MOUSE_NORMAL_MODEL_PATH  = "Resource/Models/MouseModel/ネズミ.mv1";
    constexpr float       MOUSE_NORMAL_MODEL_SCALE = 11.0f;       // 表示スケール
    constexpr float       MOUSE_NORMAL_MODEL_ROT_Y = 3.14159265f; // モデル正面方向の補正角（180度反転）

    // ------------------------------------------------------------------------
    // 高速逃走ネズミモデル設定
    // ------------------------------------------------------------------------
    constexpr const char* MOUSE_FAST_MODEL_PATH  = "Resource/Models/MouseModel/ネズミ.mv1";
    constexpr float       MOUSE_FAST_MODEL_SCALE = 11.0f;         // 表示スケール
    constexpr float       MOUSE_FAST_MODEL_ROT_Y = 3.14159265f;   // モデル正面方向の補正角（180度反転）

    // ------------------------------------------------------------------------
    // ステージ（マップ）種別定義
    // ------------------------------------------------------------------------
    enum class MapType
    {
        House      = 0, // マップ1: 通常ハウス (firstmap.mv1)
        NewMap     = 1, // マップ2: 大型ステージ (newmap.mv1)
        SlopeHills = 1  // マップ2の互換エイリアス
    };

    // ------------------------------------------------------------------------
    // ステージ1: 通常ハウス (MAP 1)
    // ------------------------------------------------------------------------
    constexpr const char* MAP1_HOUSE_PATH  = "Resource/Models/Maps/firstmap.mv1";
    constexpr float       MAP1_HOUSE_SCALE = 100.0f;
    constexpr const char* MAP1_HOUSE_NAME  = "通常ハウス (HOUSE)";

    // ------------------------------------------------------------------------
    // ステージ2: NEW MAP (MAP 2)
    // ------------------------------------------------------------------------
    constexpr const char* MAP2_NEWMAP_PATH  = "Resource/Models/Maps/newmap.mv1";
    constexpr float       MAP2_NEWMAP_SCALE = 100.0f;
    constexpr const char* MAP2_NEWMAP_NAME  = "NEW MAP";

    // 後方互換用定数エイリアス
    constexpr const char* MAP2_SLOPE_PATH   = MAP2_NEWMAP_PATH;
    constexpr float       MAP2_SLOPE_SCALE  = MAP2_NEWMAP_SCALE;
    constexpr const char* MAP2_SLOPE_NAME   = MAP2_NEWMAP_NAME;
    constexpr const char* STAGE_MODEL_PATH  = MAP2_NEWMAP_PATH;
    constexpr float       STAGE_MODEL_SCALE = MAP2_NEWMAP_SCALE;

    // ------------------------------------------------------------------------
    // 室内家具モデル（AccessoryModels）設定
    // ------------------------------------------------------------------------
    constexpr const char* KITCHEN_MODEL_PATH          = "Resource/Models/AccessoryModels/Kitchen.mv1";
    constexpr float       KITCHEN_MODEL_SCALE         = 75.0f;
    constexpr float       KITCHEN_MODEL_OFFSET_Y      = 37.5f;

    constexpr const char* REFRIGERATOR_MODEL_PATH     = "Resource/Models/AccessoryModels/Refrigerator.mv1";
    constexpr float       REFRIGERATOR_MODEL_SCALE    = 80.0f;
    constexpr float       REFRIGERATOR_MODEL_OFFSET_Y = 80.0f;

    constexpr const char* TABLE_MODEL_PATH            = "Resource/Models/AccessoryModels/table.mv1";
    constexpr float       TABLE_MODEL_SCALE           = 75.0f;
    constexpr float       TABLE_MODEL_OFFSET_Y        = 37.5f;

    constexpr const char* TV_MODEL_PATH               = "Resource/Models/AccessoryModels/TV.mv1";
    constexpr float       TV_MODEL_SCALE              = 60.0f;
    constexpr float       TV_MODEL_OFFSET_Y           = 3.0f;

    // ------------------------------------------------------------------------
    // トレーニング器具モデルパス（拡張・フォールバック互換用）
    // ------------------------------------------------------------------------
    constexpr const char* BENCH_PRESS_MODEL_PATH   = "Resource/Models/bench_press.mv1";
    constexpr const char* DUMBBELL_RACK_MODEL_PATH = "Resource/Models/dumbbell_rack.mv1";
    constexpr const char* PROTEIN_BAR_MODEL_PATH   = "Resource/Models/protein_bar.mv1";
    constexpr const char* POWER_RACK_MODEL_PATH    = "Resource/Models/power_rack.mv1";
    constexpr const char* SMITH_MACHINE_MODEL_PATH = "Resource/Models/smith_machine.mv1";
    constexpr const char* TREADMILL_MODEL_PATH     = "Resource/Models/treadmill.mv1";
}
