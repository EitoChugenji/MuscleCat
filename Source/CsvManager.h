#pragma once
#include "DxLib.h"
#include "ModelConfig.h"
#include <string>
#include <vector>

// キャラクター種別ごとのステータス設定データ (Statu.csv 対応)
struct RoleStatusData
{
    std::string role;
    float baseSpeed          = 4.0f;  // 初期速度 (base_speed)
    float speedGrowthPerUnit = 0.1f;  // 1単位あたりの速度成長量 (speed_growth_per_unit)
    float maxSpeed           = 5.5f;  // 最大速度上限 (max_speed)
};

// CsvManager クラス (シングルトン)
// Statu.csv（速度ステータス管理）および Position.csv（スポーン・巡回座標管理）の読み込み・一元管理
class CsvManager
{
private:
    RoleStatusData m_catStatus;
    RoleStatusData m_mouseStatus;
    RoleStatusData m_fastMouseStatus;
    bool           m_hasCustomFastMouse = false;

    std::vector<VECTOR> m_catPositions;
    std::vector<VECTOR> m_mousePositions;
    std::vector<VECTOR> m_catPositionsHouse;
    std::vector<VECTOR> m_mousePositionsHouse;

    CsvManager();
    ~CsvManager() = default;

    void SetDefaultValues();
    static std::string ResolveCsvFilePath(const std::string& primaryPath, const std::string& fallbackPath);
    static std::vector<std::string> SplitCsvLine(const std::string& line);
    static std::string Trim(const std::string& s);
    static std::string ToLower(const std::string& s);
    static bool ParseVectorCell(const std::string& cell, VECTOR& outVec);

public:
    static CsvManager& GetInstance()
    {
        static CsvManager instance;
        return instance;
    }

    CsvManager(const CsvManager&) = delete;
    CsvManager& operator=(const CsvManager&) = delete;

    // 初期化および全CSV読み込み
    void Init();
    void ReloadAll();

    // 個別CSV読み込み
    bool LoadStatusCsv(const std::string& primaryPath, const std::string& fallbackPath = "");
    bool LoadPositionCsv(const std::string& primaryPath, const std::string& fallbackPath = "");

    // ステータス取得
    const RoleStatusData& GetCatStatus() const
    {
        return m_catStatus;
    }

    const RoleStatusData& GetMouseStatus() const
    {
        return m_mouseStatus;
    }

    const RoleStatusData& GetFastMouseStatus() const
    {
        return m_fastMouseStatus;
    }

    // 座標リスト取得（CSV生データ）
    const std::vector<VECTOR>& GetCatPositions() const
    {
        return m_catPositions;
    }

    const std::vector<VECTOR>& GetMousePositions() const
    {
        return m_mousePositions;
    }

    // マップ種別に応じた有効座標リスト取得
    std::vector<VECTOR> GetCatPositionsForMap(ModelConfig::MapType mapType) const;
    std::vector<VECTOR> GetMousePositionsForMap(ModelConfig::MapType mapType) const;
};
