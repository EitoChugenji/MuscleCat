#define NOMINMAX
#include "WorkoutCollectionManager.h"
#include "DxLib.h"
#include "SoundManager.h"
#include "Config.h"
#include <algorithm>
#include <sstream>
#include <random>
#include <fstream>
#include <windows.h>

// ============================================================================
// 1. A/E 二進数システム (4桁二進数: 1 -> A, 0 -> E)
// 例:
//  ID 0 (0000) -> EEEE (スクワット)
//  ID 1 (0001) -> EEEA (腕立て伏せ: 初期解放)
//  ID 2 (0010) -> EEAE (腹筋)
//  ID 3 (0011) -> EEAA (懸垂)
//  ID 4 (0100) -> EAEE (サイドレイズ)
//  ID 5 (0101) -> EAEA (ダンベルカール)
//  ID 6 (0110) -> EAAE (インクラインベンチ)
//  ID 7 (0111) -> EAAA (ベンチプレス)
//  ID 8 (1000) -> AEEE (インクラインOHP)
// ============================================================================
std::string WorkoutCollectionManager::IdToAeCode(int id)
{
    if (id < 0 || id > 15) return "EEEE";
    std::string code = "EEEE";
    code[0] = (id & 0x08) ? 'A' : 'E';
    code[1] = (id & 0x04) ? 'A' : 'E';
    code[2] = (id & 0x02) ? 'A' : 'E';
    code[3] = (id & 0x01) ? 'A' : 'E';
    return code;
}

int WorkoutCollectionManager::AeCodeToId(const std::string& code)
{
    if (code.length() != 4) return -1;
    int id = 0;
    if (code[0] == 'A') id |= 0x08;
    else if (code[0] != 'E') return -1;

    if (code[1] == 'A') id |= 0x04;
    else if (code[1] != 'E') return -1;

    if (code[2] == 'A') id |= 0x02;
    else if (code[2] != 'E') return -1;

    if (code[3] == 'A') id |= 0x01;
    else if (code[3] != 'E') return -1;

    return id;
}

WorkoutCollectionManager::WorkoutCollectionManager()
    : m_proteinPoints(0)
    , m_isConfirmDialogOpen(false)
    , m_statusMessageTimer(0)
{
    // コンストラクタ内では循環初期化デッドロックを防ぐためメンバ初期化のみ行う
    m_unlockedIds.insert(INITIAL_MOTION_ID);
}

void WorkoutCollectionManager::Initialize()
{
    // 初期状態へリセット
    ResetToInitialState();

    // ローカルセーブファイルがあれば読み込み試行
    LoadFromFile("SaveData.txt");
}

// ============================================================================
// 2. 初期状態（腕立て伏せ: ID 1 / EEEA のみ解放）
// ============================================================================
void WorkoutCollectionManager::ResetToInitialState()
{
    m_proteinPoints = 0;
    m_unlockedIds.clear();
    m_unlockedIds.insert(INITIAL_MOTION_ID); // 腕立て伏せ (EEEA)
    m_isConfirmDialogOpen = false;
    m_lastGachaName.clear();
    m_lastGachaCode.clear();

    SetStatusMessage("データを初期化しました（腕立て伏せ [EEEA] のみ解放）", 180);
    SyncToPlayer3D();
}

void WorkoutCollectionManager::AddProteinPoints(int points)
{
    if (points > 0)
    {
        m_proteinPoints += points;
    }
}

bool WorkoutCollectionManager::ConsumeProteinPoints(int points)
{
    if (m_proteinPoints >= points)
    {
        m_proteinPoints -= points;
        return true;
    }
    return false;
}

// 筋トレ1回プレイ完了時のポイント付与
void WorkoutCollectionManager::OnWorkoutCompleted()
{
    AddProteinPoints(POINTS_PER_WORKOUT);
    SetStatusMessage("筋トレ完了！ プロテインポイント +" + std::to_string(POINTS_PER_WORKOUT) + " pt 獲得！", 150);
}

bool WorkoutCollectionManager::IsMotionUnlocked(int id) const
{
    return m_unlockedIds.find(id) != m_unlockedIds.end();
}

bool WorkoutCollectionManager::IsMotionUnlocked(WorkoutMotionType type) const
{
    return IsMotionUnlocked(static_cast<int>(type));
}

void WorkoutCollectionManager::UnlockMotion(int id)
{
    if (id >= 0 && id < static_cast<int>(WorkoutMotionType::Count))
    {
        m_unlockedIds.insert(id);
        SyncToPlayer3D();
    }
}

// ============================================================================
// 3. ガチャシステム（未所持のアニメーションからランダム抽選）
// ============================================================================
WorkoutCollectionManager::GachaResult WorkoutCollectionManager::RollGacha()
{
    GachaResult res;
    res.success    = false;
    res.obtainedId = -1;

    // 未所持リストを抽出
    std::vector<int> lockedList;
    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        if (!IsMotionUnlocked(i))
        {
            lockedList.push_back(i);
        }
    }

    if (lockedList.empty())
    {
        res.message = "全筋トレアニメーション解放済み（コンプリート！）";
        SetStatusMessage(res.message, 200);
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
        return res;
    }

    if (m_proteinPoints < GACHA_COST)
    {
        res.message = "ポイントが足りません（必要: " + std::to_string(GACHA_COST) + " pt）";
        SetStatusMessage(res.message, 180);
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
        return res;
    }

    // ポイント消費
    ConsumeProteinPoints(GACHA_COST);

    // 未所持からランダム抽選
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, lockedList.size() - 1);
    int chosenId = lockedList[dist(gen)];

    // 解放登録
    UnlockMotion(chosenId);

    res.success    = true;
    res.obtainedId = chosenId;
    res.aeCode     = IdToAeCode(chosenId);
    res.motionName = Player3D::GetWorkoutMotionName(static_cast<WorkoutMotionType>(chosenId));
    res.message    = "★ NEW! [" + res.aeCode + "] " + res.motionName + " を獲得！";

    m_lastGachaName = res.motionName;
    m_lastGachaCode = res.aeCode;
    SetStatusMessage(res.message, 300);

    SoundManager::GetInstance().PlaySE(SeType::Decide);
    SaveToFile(); // 自動保存
    return res;
}

// ============================================================================
// 4. セーブ・ロード機能（セーブコード方式）
// ============================================================================
std::string WorkoutCollectionManager::GenerateSaveCode() const
{
    std::ostringstream oss;
    oss << "PT:" << m_proteinPoints << "#";

    // ソートして出力
    std::vector<int> sortedIds(m_unlockedIds.begin(), m_unlockedIds.end());
    std::sort(sortedIds.begin(), sortedIds.end());

    bool first = true;
    for (int id : sortedIds)
    {
        if (!first) oss << ",";
        oss << IdToAeCode(id);
        first = false;
    }
    return oss.str();
}

bool WorkoutCollectionManager::LoadSaveCode(const std::string& saveCode)
{
    if (saveCode.empty())
    {
        SetStatusMessage("セーブコードが空です", 180);
        return false;
    }

    int loadedPoints = 0;
    std::string motionPart = saveCode;

    // ポイント情報の分離 ("PT:数値#" 形式)
    size_t hashPos = saveCode.find('#');
    if (hashPos != std::string::npos)
    {
        std::string ptPrefix = saveCode.substr(0, hashPos);
        if (ptPrefix.rfind("PT:", 0) == 0)
        {
            try {
                loadedPoints = std::stoi(ptPrefix.substr(3));
            } catch (...) {
                loadedPoints = 0;
            }
        }
        motionPart = saveCode.substr(hashPos + 1);
    }

    std::unordered_set<int> newUnlocked;

    // カンマ区切り、または連続した4文字(A/E)コードをパース
    std::string cleanStr;
    for (char c : motionPart)
    {
        char uc = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (uc == 'A' || uc == 'E' || uc == ',')
        {
            cleanStr += uc;
        }
    }

    std::istringstream tokenStream(cleanStr);
    std::string token;
    while (std::getline(tokenStream, token, ','))
    {
        // 4文字ごとのチャンクをパース
        for (size_t i = 0; i + 4 <= token.length(); i += 4)
        {
            std::string sub = token.substr(i, 4);
            int id = AeCodeToId(sub);
            if (id >= 0 && id < static_cast<int>(WorkoutMotionType::Count))
            {
                newUnlocked.insert(id);
            }
        }
    }

    // 仕様: 初期解放「腕立て伏せ (ID: 1 / EEEA)」は必ず解放維持
    newUnlocked.insert(INITIAL_MOTION_ID);

    m_proteinPoints = (std::max)(0, loadedPoints);
    m_unlockedIds   = newUnlocked;
    SyncToPlayer3D();

    SetStatusMessage("セーブコード読込成功！（所持数: " + std::to_string(m_unlockedIds.size()) + "種 / " + std::to_string(m_proteinPoints) + " pt）", 200);
    SoundManager::GetInstance().PlaySE(SeType::Decide);
    SaveToFile();
    return true;
}

// クリップボードへのセーブコード書き込み
void WorkoutCollectionManager::CopySaveCodeToClipboard() const
{
    std::string code = GenerateSaveCode();

    // 1. DxLib API
    SetClipboardText(code.c_str());

    // 2. Windows API フォールバック
    if (OpenClipboard(nullptr))
    {
        EmptyClipboard();
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, code.size() + 1);
        if (hGlob)
        {
            char* pData = static_cast<char*>(GlobalLock(hGlob));
            if (pData)
            {
                memcpy(pData, code.c_str(), code.size() + 1);
                GlobalUnlock(hGlob);
                SetClipboardData(CF_TEXT, hGlob);
            }
        }
        CloseClipboard();
    }

    const_cast<WorkoutCollectionManager*>(this)->SetStatusMessage("セーブコードをクリップボードにコピーしました！", 180);
    SoundManager::GetInstance().PlaySE(SeType::Decide);
}

// クリップボードからのセーブコード読み込み
bool WorkoutCollectionManager::PasteSaveCodeFromClipboard()
{
    std::string text;

    // 1. DxLib API
    char buffer[2048] = { 0 };
    if (GetClipboardText(buffer, sizeof(buffer)) >= 0 && buffer[0] != '\0')
    {
        text = buffer;
    }
    // 2. Windows API フォールバック
    else if (OpenClipboard(nullptr))
    {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData)
        {
            char* pData = static_cast<char*>(GlobalLock(hData));
            if (pData)
            {
                text = pData;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }

    if (!text.empty())
    {
        return LoadSaveCode(text);
    }

    SetStatusMessage("クリップボードにテキストがありません", 180);
    SoundManager::GetInstance().PlaySE(SeType::Cancel);
    return false;
}

void WorkoutCollectionManager::SaveToFile(const char* filePath) const
{
    std::ofstream ofs(filePath);
    if (ofs.is_open())
    {
        ofs << GenerateSaveCode() << std::endl;
    }
}

bool WorkoutCollectionManager::LoadFromFile(const char* filePath)
{
    std::ifstream ifs(filePath);
    if (ifs.is_open())
    {
        std::string line;
        if (std::getline(ifs, line))
        {
            return LoadSaveCode(line);
        }
    }
    return false;
}

// ============================================================================
// 5. デバッグ機能（管理者・開発用）全削除実行
// ============================================================================
void WorkoutCollectionManager::ExecuteConfirmedReset()
{
    ResetToInitialState();
    m_isConfirmDialogOpen = false;
    SoundManager::GetInstance().PlaySE(SeType::Decide);
    SaveToFile();
}

void WorkoutCollectionManager::SetStatusMessage(const std::string& msg, int durationFrames)
{
    m_statusMessage = msg;
    m_statusMessageTimer = durationFrames;
}

void WorkoutCollectionManager::Update()
{
    if (m_statusMessageTimer > 0)
    {
        --m_statusMessageTimer;
    }
}

// Player3Dのランダム筋トレ選択対象フラグと同期
void WorkoutCollectionManager::SyncToPlayer3D()
{
    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        auto type = static_cast<WorkoutMotionType>(i);
        bool isUnlocked = IsMotionUnlocked(i);
        // 未解放のものは強制OFF、解放済みのものは現在状態を維持
        if (!isUnlocked)
        {
            Player3D::SetWorkoutMotionEnabled(type, false);
        }
    }
    // 解放済みが1つ以上ONになっていることを保証
    if (Player3D::GetEnabledWorkoutMotionCount() == 0)
    {
        Player3D::SetWorkoutMotionEnabled(WorkoutMotionType::PushUp, true);
    }
}

// ============================================================================
// 6. 確認ダイアログのモーダル描画
// ============================================================================
void WorkoutCollectionManager::DrawConfirmDialogModal(int fontTitle, int fontBody)
{
    if (!m_isConfirmDialogOpen) return;

    // 画面全体を暗転（Alpha半透明オーバーレイ）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 185);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(5, 8, 16), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ダイアログカード中央配置
    int dlgW = 540;
    int dlgH = 250;
    int dlgX = (Config::SCREEN_WIDTH - dlgW) / 2;
    int dlgY = (Config::SCREEN_HEIGHT - dlgH) / 2;

    // カード背景
    DrawRoundRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH, 16, 16, GetColor(28, 22, 34), TRUE);
    DrawRoundRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH, 16, 16, GetColor(255, 75, 75), FALSE);
    DrawRoundRect(dlgX + 2, dlgY + 2, dlgX + dlgW - 2, dlgY + dlgH - 2, 14, 14, GetColor(180, 45, 45), FALSE);

    // タイトルバナー
    const char* titleTxt = "★ 【Debug】データ全削除の確認 ★";
    int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), fontTitle);
    DrawStringToHandle(dlgX + (dlgW - tW) / 2, dlgY + 22, titleTxt, GetColor(255, 95, 95), fontTitle);

    // 警告メッセージ
    const char* msg1 = "本当に全データを削除して初期化しますか？";
    int mW1 = GetDrawStringWidthToHandle(msg1, static_cast<int>(strlen(msg1)), fontBody);
    DrawStringToHandle(dlgX + (dlgW - mW1) / 2, dlgY + 75, msg1, GetColor(255, 255, 255), fontBody);

    const char* msg2 = "※所持モーション・プロテインpt・セーブコードがすべてリセットされ、";
    int mW2 = GetDrawStringWidthToHandle(msg2, static_cast<int>(strlen(msg2)), fontBody);
    DrawStringToHandle(dlgX + (dlgW - mW2) / 2, dlgY + 105, msg2, GetColor(255, 200, 150), fontBody);

    const char* msg3 = "「腕立て伏せ (ID: EEEA)」のみの初期状態に戻ります。";
    int mW3 = GetDrawStringWidthToHandle(msg3, static_cast<int>(strlen(msg3)), fontBody);
    DrawStringToHandle(dlgX + (dlgW - mW3) / 2, dlgY + 128, msg3, GetColor(255, 220, 160), fontBody);

    // ボタン配置
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);

    // [はい（削除）] ボタン
    int yesX = dlgX + 60, yesY = dlgY + 175, yesW = 180, yesH = 46;
    bool hoverYes = (mx >= yesX && mx <= yesX + yesW && my >= yesY && my <= yesY + yesH);
    unsigned int yesBg = hoverYes ? GetColor(235, 45, 45) : GetColor(185, 30, 30);
    DrawRoundRect(yesX, yesY, yesX + yesW, yesY + yesH, 10, 10, yesBg, TRUE);
    DrawRoundRect(yesX, yesY, yesX + yesW, yesY + yesH, 10, 10, hoverYes ? GetColor(255, 220, 220) : GetColor(255, 100, 100), FALSE);

    const char* yesTxt = "はい（削除実行）";
    int yW = GetDrawStringWidthToHandle(yesTxt, static_cast<int>(strlen(yesTxt)), fontBody);
    DrawStringToHandle(yesX + (yesW - yW) / 2, yesY + 13, yesTxt, GetColor(255, 255, 255), fontBody);

    // [いいえ（キャンセル）] ボタン
    int noX = dlgX + dlgW - 240, noY = dlgY + 175, noW = 180, noH = 46;
    bool hoverNo = (mx >= noX && mx <= noX + noW && my >= noY && my <= noY + noH);
    unsigned int noBg = hoverNo ? GetColor(70, 85, 115) : GetColor(45, 55, 80);
    DrawRoundRect(noX, noY, noX + noW, noY + noH, 10, 10, noBg, TRUE);
    DrawRoundRect(noX, noY, noX + noW, noY + noH, 10, 10, hoverNo ? GetColor(200, 225, 255) : GetColor(110, 135, 175), FALSE);

    const char* noTxt = "いいえ（戻る）";
    int nW = GetDrawStringWidthToHandle(noTxt, static_cast<int>(strlen(noTxt)), fontBody);
    DrawStringToHandle(noX + (noW - nW) / 2, noY + 13, noTxt, GetColor(230, 240, 255), fontBody);
}
