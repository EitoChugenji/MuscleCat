// ============================================================================
// @file   CsvManager.cpp
// @brief  CsvManager クラスの実装（Statu.csv / Position.csv の解析・座標管理）
// ============================================================================
#include "CsvManager.h"
#include "Config.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

CsvManager::CsvManager()
{
    SetDefaultValues();
}

void CsvManager::SetDefaultValues()
{
    // Statu.csv デフォルト値 (Cat: 4, 0.1, 5.5 / Mouse: 4.2, 0.05, 4.7)
    m_catStatus = { "Cat", 4.0f, 0.1f, 5.5f };
    m_mouseStatus = { "Mouse", 4.2f, 0.05f, 4.7f };
    // 高速ネズミはデフォルトで通常ネズミの約1.1倍（猫の最大速度5.5fより遅く捕獲可能なバランス）
    m_fastMouseStatus = { "FastMouse", m_mouseStatus.baseSpeed * 1.1f, m_mouseStatus.speedGrowthPerUnit, m_mouseStatus.maxSpeed * 1.1f };
    m_hasCustomFastMouse = false;

    // Position.csv デフォルト値 (NewMap)
    m_catPositions = {
        VGet(0.0f, 0.0f, 450.0f)
    };

    m_mousePositions = {
        VGet(   0.0f, 20.0f,  -400.0f),
        VGet( 600.0f, 20.0f,   800.0f),
        VGet(-600.0f, 20.0f,   800.0f),
        VGet(1200.0f, 20.0f,     0.0f),
        VGet(1600.0f, 20.0f,   400.0f),
        VGet(1800.0f, 20.0f,  -400.0f),
        VGet( 400.0f, 20.0f, -1400.0f),
        VGet(-1200.0f, 20.0f, -1200.0f)
    };

    // House マップ用フォールバック座標
    m_catPositionsHouse = {
        VGet(0.0f, 5.0f, 0.0f)
    };

    m_mousePositionsHouse = {
        VGet(   0.0f, 5.0f,    0.0f),
        VGet( 180.0f, 5.0f,  180.0f),
        VGet(-180.0f, 5.0f,  180.0f),
        VGet(-180.0f, 5.0f, -180.0f),
        VGet( 550.0f, 5.0f,  450.0f),
        VGet( 600.0f, 5.0f, -100.0f),
        VGet( 650.0f, 5.0f, -700.0f),
        VGet(-100.0f, 5.0f, -480.0f)
    };
}

void CsvManager::Init()
{
    ReloadAll();
}

void CsvManager::ReloadAll()
{
    SetDefaultValues();
    LoadStatusCsv(Config::STATUS_CSV_PATH, Config::STATUS_CSV_SOURCE_PATH);
    LoadPositionCsv(Config::POSITION_CSV_PATH, Config::POSITION_CSV_SOURCE_PATH);
}

std::string CsvManager::ResolveCsvFilePath(const std::string& primaryPath, const std::string& fallbackPath)
{
    namespace fs = std::filesystem;
    std::error_code ec1, ec2;
    bool pExists = !primaryPath.empty() && fs::exists(fs::u8path(primaryPath), ec1);
    bool fExists = !fallbackPath.empty() && fs::exists(fs::u8path(fallbackPath), ec2);

    if (pExists && fExists)
    {
        auto pTime = fs::last_write_time(fs::u8path(primaryPath), ec1);
        auto fTime = fs::last_write_time(fs::u8path(fallbackPath), ec2);
        if (!ec1 && !ec2 && fTime > pTime)
        {
            return fallbackPath;
        }
        return primaryPath;
    }

    if (pExists)
    {
        return primaryPath;
    }

    if (fExists)
    {
        return fallbackPath;
    }

    return primaryPath;
}

std::string CsvManager::Trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && (static_cast<unsigned char>(s[start]) <= 0x20 || s[start] == '"'))
    {
        start++;
    }

    size_t end = s.size();
    while (end > start && (static_cast<unsigned char>(s[end - 1]) <= 0x20 || s[end - 1] == '"'))
    {
        end--;
    }

    return s.substr(start, end - start);
}

std::string CsvManager::ToLower(const std::string& s)
{
    std::string res = s;
    for (char& c : res)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return res;
}

std::vector<std::string> CsvManager::SplitCsvLine(const std::string& line)
{
    std::vector<std::string> cells;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes)
        {
            cells.push_back(Trim(current));
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }
    cells.push_back(Trim(current));
    return cells;
}

bool CsvManager::ParseVectorCell(const std::string& cell, VECTOR& outVec)
{
    std::string cleaned = Trim(cell);
    if (cleaned.empty())
    {
        return false;
    }

    // 区切り文字 (':', '/', ';', '|', ',', '(', ')') を空白に置換して数値を抽出
    for (char& c : cleaned)
    {
        if (c == ':' || c == '/' || c == ';' || c == '|' || c == ',' || c == '(' || c == ')')
        {
            c = ' ';
        }
    }

    std::stringstream ss(cleaned);
    std::vector<float> vals;
    float v = 0.0f;
    while (ss >> v)
    {
        vals.push_back(v);
    }

    if (vals.size() >= 3)
    {
        outVec = VGet(vals[0], vals[1], vals[2]);
        return true;
    }
    else if (vals.size() == 2)
    {
        // 2要素 (X, Z) の場合は Y = 0.0f とする
        outVec = VGet(vals[0], 0.0f, vals[1]);
        return true;
    }

    return false;
}

bool CsvManager::LoadStatusCsv(const std::string& primaryPath, const std::string& fallbackPath)
{
    std::string targetPath = ResolveCsvFilePath(primaryPath, fallbackPath);
    std::ifstream ifs( std::filesystem::u8path(targetPath) );
    if (!ifs.is_open())
    {
        AppLogAdd("[CsvManager] Warning: Could not open Status CSV '%s'. Using defaults.\n", targetPath.c_str());
        return false;
    }

    std::string line;
    bool isFirstLine = true;
    int loadedRowCount = 0;

    while (std::getline(ifs, line))
    {
        if (isFirstLine)
        {
            isFirstLine = false;
            // UTF-8 BOM の除去
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF)
            {
                line = line.substr(3);
            }
        }

        std::string trimmedLine = Trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#')
        {
            continue;
        }

        std::vector<std::string> cells = SplitCsvLine(line);

        // 先頭の空セルをスキップ（Excelで余白列がある状態のままCSV出力した場合にも対応）
        size_t roleIdx = 0;
        while (roleIdx < cells.size() && cells[roleIdx].empty())
        {
            roleIdx++;
        }

        if (roleIdx + 3 >= cells.size())
        {
            continue;
        }

        std::string roleKey = ToLower(cells[roleIdx]);
        if (roleKey == "role" || roleKey.empty())
        {
            continue; // ヘッダー行スキップ
        }

        try
        {
            float baseSpd   = std::stof(cells[roleIdx + 1]);
            float growthSpd = std::stof(cells[roleIdx + 2]);
            float maxSpd    = std::stof(cells[roleIdx + 3]);

            if (roleKey == "cat" || roleKey == "player")
            {
                m_catStatus = { cells[roleIdx], baseSpd, growthSpd, maxSpd };
                loadedRowCount++;
            }
            else if (roleKey == "mouse" || roleKey == "normalmouse")
            {
                m_mouseStatus = { cells[roleIdx], baseSpd, growthSpd, maxSpd };
                if (!m_hasCustomFastMouse)
                {
                    m_fastMouseStatus = {
                        "FastMouse",
                        m_mouseStatus.baseSpeed * 1.1f,
                        m_mouseStatus.speedGrowthPerUnit,
                        m_mouseStatus.maxSpeed * 1.1f
                    };
                }
                loadedRowCount++;
            }
            else if (roleKey == "fastmouse")
            {
                m_fastMouseStatus = { cells[roleIdx], baseSpd, growthSpd, maxSpd };
                m_hasCustomFastMouse = true;
                loadedRowCount++;
            }
        }
        catch (...)
        {
            continue;
        }
    }

    AppLogAdd("[CsvManager] Loaded Status CSV '%s' (rows=%d): Cat(base=%.2f, growth=%.2f, max=%.2f), Mouse(base=%.2f, growth=%.2f, max=%.2f)\n",
        targetPath.c_str(), loadedRowCount,
        m_catStatus.baseSpeed, m_catStatus.speedGrowthPerUnit, m_catStatus.maxSpeed,
        m_mouseStatus.baseSpeed, m_mouseStatus.speedGrowthPerUnit, m_mouseStatus.maxSpeed);

    return (loadedRowCount > 0);
}

bool CsvManager::LoadPositionCsv(const std::string& primaryPath, const std::string& fallbackPath)
{
    std::string targetPath = ResolveCsvFilePath(primaryPath, fallbackPath);
    std::ifstream ifs( std::filesystem::u8path(targetPath) );
    if (!ifs.is_open())
    {
        AppLogAdd("[CsvManager] Warning: Could not open Position CSV '%s'. Using defaults.\n", targetPath.c_str());
        return false;
    }

    std::vector<VECTOR> loadedCatPos;
    std::vector<VECTOR> loadedMousePos;
    std::vector<VECTOR> loadedCatHousePos;
    std::vector<VECTOR> loadedMouseHousePos;

    std::string line;
    bool isFirstLine = true;

    while (std::getline(ifs, line))
    {
        if (isFirstLine)
        {
            isFirstLine = false;
            // UTF-8 BOM の除去
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF)
            {
                line = line.substr(3);
            }
        }

        std::string trimmedLine = Trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#')
        {
            continue;
        }

        std::vector<std::string> cells = SplitCsvLine(line);

        size_t roleIdx = 0;
        while (roleIdx < cells.size() && cells[roleIdx].empty())
        {
            roleIdx++;
        }

        if (roleIdx >= cells.size())
        {
            continue;
        }

        std::string roleKey = ToLower(cells[roleIdx]);
        if (roleKey == "role" || roleKey.rfind("pos", 0) == 0)
        {
            continue; // ヘッダー行スキップ
        }

        std::vector<VECTOR> rowVectors;
        for (size_t i = roleIdx + 1; i < cells.size(); ++i)
        {
            VECTOR v;
            if (ParseVectorCell(cells[i], v))
            {
                rowVectors.push_back(v);
            }
        }

        if (rowVectors.empty())
        {
            continue;
        }

        if (roleKey == "cat" || roleKey == "player")
        {
            loadedCatPos.insert(loadedCatPos.end(), rowVectors.begin(), rowVectors.end());
        }
        else if (roleKey == "mouse" || roleKey == "mice")
        {
            loadedMousePos.insert(loadedMousePos.end(), rowVectors.begin(), rowVectors.end());
        }
        else if (roleKey == "cat_house" || roleKey == "cathouse")
        {
            loadedCatHousePos.insert(loadedCatHousePos.end(), rowVectors.begin(), rowVectors.end());
        }
        else if (roleKey == "mouse_house" || roleKey == "mousehouse")
        {
            loadedMouseHousePos.insert(loadedMouseHousePos.end(), rowVectors.begin(), rowVectors.end());
        }
    }

    if (!loadedCatPos.empty())
    {
        m_catPositions = loadedCatPos;
    }

    if (!loadedMousePos.empty())
    {
        m_mousePositions = loadedMousePos;
    }

    if (!loadedCatHousePos.empty())
    {
        m_catPositionsHouse = loadedCatHousePos;
    }

    if (!loadedMouseHousePos.empty())
    {
        m_mousePositionsHouse = loadedMouseHousePos;
    }

    AppLogAdd("[CsvManager] Loaded Position CSV '%s': Cat positions=%d, Mouse positions=%d\n",
        targetPath.c_str(),
        static_cast<int>(m_catPositions.size()),
        static_cast<int>(m_mousePositions.size()));

    return (!loadedCatPos.empty() || !loadedMousePos.empty());
}

std::vector<VECTOR> CsvManager::GetCatPositionsForMap(ModelConfig::MapType mapType) const
{
    if (mapType == ModelConfig::MapType::House)
    {
        // Position.csv 内の Cat 座標が House マップの範囲内であれば優先採用
        std::vector<VECTOR> validForHouse;
        for (const auto& p : m_catPositions)
        {
            if (p.x >= -300.0f && p.x <= 820.0f && p.z >= -800.0f && p.z <= 650.0f &&
                !(std::abs(p.x) < 1.0f && std::abs(p.z - 450.0f) < 1.0f))
            {
                validForHouse.push_back(p);
            }
        }
        if (!validForHouse.empty())
        {
            return validForHouse;
        }
        return m_catPositionsHouse;
    }

    return m_catPositions;
}

std::vector<VECTOR> CsvManager::GetMousePositionsForMap(ModelConfig::MapType mapType) const
{
    if (mapType == ModelConfig::MapType::House)
    {
        std::vector<VECTOR> validForHouse;
        for (const auto& p : m_mousePositions)
        {
            if (p.x >= -300.0f && p.x <= 820.0f && p.z >= -800.0f && p.z <= 650.0f)
            {
                validForHouse.push_back(p);
            }
        }
        if (validForHouse.size() >= 3)
        {
            return validForHouse;
        }
        return m_mousePositionsHouse;
    }

    return m_mousePositions;
}
