#pragma once

// ============================================================================
// 3Dモデル設定（モデル差し替え用）
// ============================================================================
// ★ ここに指定したファイル名/パスに3Dモデルファイル（.mv1, .pmd, .mqo, .obj等）を
//    配置すれば、自動的にモデル描画に切り替わります。
//    ファイルが存在しない場合は、自動的にプロシージャルな3Dプリミティブで描画されます。
// ============================================================================
namespace ModelConfig {
    // ------------------------------------------------------------------------
    // プレイヤー（マッスルねこ）
    // ------------------------------------------------------------------------
    constexpr const char* CAT_MODEL_PATH = "Resource/Models/cat.mv1";
    constexpr float       CAT_MODEL_SCALE = 1.6f;
    constexpr float       CAT_MODEL_ROT_Y = 0.0f; // モデル初期向き補正（ラジアン）

    // ------------------------------------------------------------------------
    // 通常ネズミ
    // ------------------------------------------------------------------------
    constexpr const char* MOUSE_NORMAL_MODEL_PATH = "Resource/ネズミ.mv1";
    constexpr float       MOUSE_NORMAL_MODEL_SCALE = 7.0f;
    constexpr float       MOUSE_NORMAL_MODEL_ROT_Y = 3.14159265f; // 180度反転（ラジアン）

    // ------------------------------------------------------------------------
    // 高速逃走ネズミ
    // ------------------------------------------------------------------------
    constexpr const char* MOUSE_FAST_MODEL_PATH = "Resource/ネズミ.mv1";
    constexpr float       MOUSE_FAST_MODEL_SCALE = 7.0f;
    constexpr float       MOUSE_FAST_MODEL_ROT_Y = 3.14159265f; // 180度反転（ラジアン）

    // ------------------------------------------------------------------------
    // 3Dステージ / ジムルーム
    // ------------------------------------------------------------------------
    constexpr const char* STAGE_MODEL_PATH = "Resource/Models/stage.mv1";
    constexpr float       STAGE_MODEL_SCALE = 1.0f;

    // ------------------------------------------------------------------------
    // 障害物（トレーニング器具）
    // ------------------------------------------------------------------------
    constexpr const char* BENCH_PRESS_MODEL_PATH  = "Resource/Models/bench_press.mv1";
    constexpr const char* DUMBBELL_RACK_MODEL_PATH = "Resource/Models/dumbbell_rack.mv1";
    constexpr const char* PROTEIN_BAR_MODEL_PATH   = "Resource/Models/protein_bar.mv1";
    constexpr const char* POWER_RACK_MODEL_PATH    = "Resource/Models/power_rack.mv1";
    constexpr const char* SMITH_MACHINE_MODEL_PATH = "Resource/Models/smith_machine.mv1";
    constexpr const char* TREADMILL_MODEL_PATH     = "Resource/Models/treadmill.mv1";
}