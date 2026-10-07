#pragma once

// 3Dモデル設定（モデル差し替え用）
// 指定したパスに3Dモデルファイルを配置すると自動的にモデル描画に切り替わります
// ファイルが存在しない場合はプロシージャルな3Dプリミティブで描画されます
namespace ModelConfig
{
    // プレイヤー（MuscleCat）
    constexpr const char* CAT_MODEL_PATH = "Resource/Models/CatModel/cat.mv1";
    constexpr float       CAT_MODEL_SCALE = 0.24f;          // スケール2倍に拡大（0.12 → 0.24）
    constexpr float       CAT_MODEL_ROT_Y = 3.14159265f;    // 180度反転（反対向き補正）

    // アクセサリ（バーベル）
    constexpr const char* BARBELL_MODEL_PATH  = "Resource/Models/AccessoryModels/Barbell.mv1";
    constexpr float       BARBELL_BASE_SCALE  = 18.0f;       // 基準スケール（両手幅に合わせて自動連動）

    // 通常ネズミ
    constexpr const char* MOUSE_NORMAL_MODEL_PATH = "Resource/Models/MouseModel/ネズミ.mv1";
    constexpr float       MOUSE_NORMAL_MODEL_SCALE = 7.0f;
    constexpr float       MOUSE_NORMAL_MODEL_ROT_Y = 3.14159265f; // 180度反転（ラジアン）

    // 高速逃走ネズミ
    constexpr const char* MOUSE_FAST_MODEL_PATH = "Resource/Models/MouseModel/ネズミ.mv1";
    constexpr float       MOUSE_FAST_MODEL_SCALE = 7.0f;
    constexpr float       MOUSE_FAST_MODEL_ROT_Y = 3.14159265f; // 180度反転（ラジアン）

    // ステージ（家/部屋）
    constexpr const char* STAGE_MODEL_PATH = "Resource/Models/Maps/firstmap.mv1";
    constexpr float       STAGE_MODEL_SCALE = 100.0f;

    // 家具モデル（AccessoryModels）
    constexpr const char* KITCHEN_MODEL_PATH       = "Resource/Models/AccessoryModels/Kitchen.mv1";
    constexpr float       KITCHEN_MODEL_SCALE      = 75.0f;
    constexpr float       KITCHEN_MODEL_OFFSET_Y   = 37.5f;

    constexpr const char* REFRIGERATOR_MODEL_PATH  = "Resource/Models/AccessoryModels/Refrigerator.mv1";
    constexpr float       REFRIGERATOR_MODEL_SCALE = 80.0f;
    constexpr float       REFRIGERATOR_MODEL_OFFSET_Y = 80.0f;

    constexpr const char* TABLE_MODEL_PATH         = "Resource/Models/AccessoryModels/table.mv1";
    constexpr float       TABLE_MODEL_SCALE        = 75.0f;
    constexpr float       TABLE_MODEL_OFFSET_Y     = 37.5f;

    constexpr const char* TV_MODEL_PATH            = "Resource/Models/AccessoryModels/TV.mv1";
    constexpr float       TV_MODEL_SCALE           = 60.0f;
    constexpr float       TV_MODEL_OFFSET_Y        = 3.0f;

    // 障害物（トレーニング器具・互換用）
    constexpr const char* BENCH_PRESS_MODEL_PATH  = "Resource/Models/bench_press.mv1";
    constexpr const char* DUMBBELL_RACK_MODEL_PATH = "Resource/Models/dumbbell_rack.mv1";
    constexpr const char* PROTEIN_BAR_MODEL_PATH   = "Resource/Models/protein_bar.mv1";
    constexpr const char* POWER_RACK_MODEL_PATH    = "Resource/Models/power_rack.mv1";
    constexpr const char* SMITH_MACHINE_MODEL_PATH = "Resource/Models/smith_machine.mv1";
    constexpr const char* TREADMILL_MODEL_PATH     = "Resource/Models/treadmill.mv1";
}