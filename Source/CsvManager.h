// ============================================================================
// @file   CsvManager.h
// @brief  外部CSVファイル（Statu.csv / Position.csv）の読み込み・管理クラス
// ============================================================================
#pragma once
#include "DxLib.h"
#include "ModelConfig.h"
#include <string>
#include <vector>

// ============================================================================
// キャラクター種別ごとの移動速度ステータス設定データ構造体 (Statu.csv 対応)
// ============================================================================
struct RoleStatusData
{
    std::string role;                  // ロール名 ("Cat", "Mouse", "FastMouse")
    float baseSpeed          = 4.0f;   // 初期移動速度 (base_speed: 単位/フレーム)
    float speedGrowthPerUnit = 0.1f;   // 1単位（1Rep または 被捕食1匹）あたりの速度成長量
    float maxSpeed           = 5.5f;   // 最大移動速度の上限値 (max_speed)
};

// ============================================================================
// CsvManager クラス（シングルトン）
// Statu.csv（猫・ネズミの速度パラメータ）および Position.csv（スポーン・巡回座標）を
// パースして一元管理し、ファイル未配置時にも安全なデフォルト値で動作を保証する
// ============================================================================
class CsvManager
{
private:
    // キャラクター種別ごとの速度パラメータ
    RoleStatusData m_catStatus;                // プレイヤー（猫）の速度設定
    RoleStatusData m_mouseStatus;              // 通常ネズミの速度設定
    RoleStatusData m_fastMouseStatus;          // 高速ネズミの速度設定
    bool           m_hasCustomFastMouse = false; // CSV内にFastMouse専用の行が定義されていたか

    // マップ種別ごとのスポーン・巡回座標リスト
    std::vector<VECTOR> m_catPositions;        // NewMap（または汎用）での猫初期位置候補
    std::vector<VECTOR> m_mousePositions;      // NewMap（または汎用）でのネズミ出現・巡回座標候補
    std::vector<VECTOR> m_catPositionsHouse;   // Houseマップ用の猫初期位置フォールバック
    std::vector<VECTOR> m_mousePositionsHouse; // Houseマップ用のネズミ出現・巡回座標フォールバック

    CsvManager();
    ~CsvManager() = default;

    // CSV未検出時・再読み込み前に安全な初期値をセットする
    void SetDefaultValues();

    // 実行フォルダ側とソース側を比較し、存在する（または更新日時が新しい）CSVパスを返す
    static std::string ResolveCsvFilePath(const std::string& primaryPath, const std::string& fallbackPath);

    // CSVの1行をカンマ区切りでセル配列に分割する（ダブルクォート対応）
    static std::vector<std::string> SplitCsvLine(const std::string& line);

    // 文字列前後の空白・改行・ダブルクォートを除去する
    static std::string Trim(const std::string& s);

    // ASCII英字を小文字に変換した文字列を返す（キー名の大文字小文字無視比較用）
    static std::string ToLower(const std::string& s);

    // "X:Y:Z" や "X/Y/Z" 形式のセル文字列から3D座標 VECTOR を抽出する
    static bool ParseVectorCell(const std::string& cell, VECTOR& outVec);

public:
    // シングルトンインスタンスの取得
    static CsvManager& GetInstance()
    {
        static CsvManager instance;
        return instance;
    }

    CsvManager(const CsvManager&) = delete;
    CsvManager& operator=(const CsvManager&) = delete;

    // 初期化および全CSVの再読み込み
    void Init();
    void ReloadAll();

    // 個別CSVファイルの読み込み
    bool LoadStatusCsv(const std::string& primaryPath, const std::string& fallbackPath = "");
    bool LoadPositionCsv(const std::string& primaryPath, const std::string& fallbackPath = "");

    // ------------------------------------------------------------------------
    // ステータス取得ゲッター
    // ------------------------------------------------------------------------
    const RoleStatusData& GetCatStatus() const       { return m_catStatus; }
    const RoleStatusData& GetMouseStatus() const     { return m_mouseStatus; }
    const RoleStatusData& GetFastMouseStatus() const { return m_fastMouseStatus; }

    // ------------------------------------------------------------------------
    // 座標リスト取得ゲッター
    // ------------------------------------------------------------------------
    const std::vector<VECTOR>& GetCatPositions() const   { return m_catPositions; }
    const std::vector<VECTOR>& GetMousePositions() const { return m_mousePositions; }

    // 選択されたマップ種別（House / NewMap）の境界内に適合する座標リストを返す
    std::vector<VECTOR> GetCatPositionsForMap(ModelConfig::MapType mapType) const;
    std::vector<VECTOR> GetMousePositionsForMap(ModelConfig::MapType mapType) const;
};
