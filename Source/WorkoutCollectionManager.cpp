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
    res.message    = "★ NEW! " + res.motionName + " を獲得！";

    m_lastGachaName = res.motionName;
    m_lastGachaCode = res.aeCode;
    SetStatusMessage(res.message, 300);

    // ★ 筋トレ感大あり熱血ガチャ演出を開始！
    StartGachaAnimation(chosenId, res.motionName, res.aeCode);

    SaveToFile();
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


// ============================================================================
// ★ 筋トレ感大あり！熱血ガチャ演出の実装
// ============================================================================
void WorkoutCollectionManager::StartGachaAnimation(int obtainedId, const std::string& name, const std::string& aeCode)
{
    m_gachaAnimPhase   = GachaAnimPhase::ShakeCharge;
    m_gachaAnimTimer   = 0;
    m_gachaObtainedId  = obtainedId;
    m_gachaMotionName  = name;
    m_gachaAeCode      = aeCode;
    m_gachaShakeX      = 0.0f;
    m_gachaShakeY      = 0.0f;
    m_gachaWhiteFlash  = 0.0f;

    // 各モーションに対応する熱血マッスル情報
    switch (obtainedId)
    {
    case 0: // スクワット
        m_gachaTargetMuscle = "鍛えられる部位: 大腿四頭筋・大殿筋 (下半身の破壊力MAX!)";
        m_gachaMuscleCall   = "「脚が丸太！樹齢一千年！ナイススクワット！！」";
        break;
    case 1: // 腕立て伏せ
        m_gachaTargetMuscle = "鍛えられる部位: 大胸筋・上腕三頭筋 (厚みのある強靭な胸板!)";
        m_gachaMuscleCall   = "「床を押すな！地球を押せ！胸筋巨大メロンパン！！」";
        break;
    case 2: // 腹筋 (シットアップ)
        m_gachaTargetMuscle = "鍛えられる部位: 腹直筋・体幹 (ダイヤモンド級シックスパック!)";
        m_gachaMuscleCall   = "「腹筋が6LDK！板チョコを超えた神の彫刻！！」";
        break;
    case 3: // 懸垂 (プルアップ)
        m_gachaTargetMuscle = "鍛えられる部位: 広背筋・大円筋 (空飛ぶムササビ背筋!)";
        m_gachaMuscleCall   = "「背中に鬼の顔降臨！広背筋に翼が生えてるぞ！！」";
        break;
    case 4: // サイドレイズ
        m_gachaTargetMuscle = "鍛えられる部位: 三角筋中部 (圧倒的メロン肩シルエット!)";
        m_gachaMuscleCall   = "「肩メロン！肩にちっちゃい重機乗せてんのかい！！」";
        break;
    case 5: // ダンベルカール
        m_gachaTargetMuscle = "鍛えられる部位: 上腕二頭筋 (力こぶチョモランマ!)";
        m_gachaMuscleCall   = "「前足が油圧ジャッキ！仕上がりすぎてる筋肉美！！」";
        break;
    case 6: // インクラインベンチ
        m_gachaTargetMuscle = "鍛えられる部位: 大胸筋上部 (鎖骨下を埋め尽くす装甲板!)";
        m_gachaMuscleCall   = "「大胸筋上部が分厚い盾！鎖骨を突き破れ！！」";
        break;
    case 7: // ベンチプレス
        m_gachaTargetMuscle = "鍛えられる部位: 大胸筋全体・前鋸筋 (男のロマン100kgプレス!)";
        m_gachaMuscleCall   = "「大胸筋が咆哮してる！バーベルを吹き飛ばせ！！」";
        break;
    case 8: // インクラインOHP
        m_gachaTargetMuscle = "鍛えられる部位: 三角筋前部・僧帽筋 (天を突く筋肉の巨塔!)";
        m_gachaMuscleCall   = "「巨塔！スカイツリー！僧帽筋が富士山大噴火！！」";
        break;
    default:
        m_gachaTargetMuscle = "鍛えられる部位: 全身の筋肉 (超マッスルパンプアップ!)";
        m_gachaMuscleCall   = "「ナイスバルク！筋肉の神が微笑んでいる！！」";
        break;
    }

    SoundManager::GetInstance().PlaySE(SeType::Countdown);
}

void WorkoutCollectionManager::SkipGachaAnimation()
{
    if (m_gachaAnimPhase != GachaAnimPhase::None && m_gachaAnimPhase != GachaAnimPhase::ResultCard)
    {
        m_gachaAnimPhase  = GachaAnimPhase::ResultCard;
        m_gachaAnimTimer  = 0;
        m_gachaShakeX     = 0.0f;
        m_gachaShakeY     = 0.0f;
        m_gachaWhiteFlash = 0.0f;
        SoundManager::GetInstance().PlaySE(SeType::SquatSuccess);
    }
}

void WorkoutCollectionManager::CloseGachaAnimation()
{
    m_gachaAnimPhase  = GachaAnimPhase::None;
    m_gachaAnimTimer  = 0;
    m_gachaShakeX     = 0.0f;
    m_gachaShakeY     = 0.0f;
    m_gachaWhiteFlash = 0.0f;
}

void WorkoutCollectionManager::UpdateGachaAnimation(bool triggerEnter, bool triggerSpace, bool triggerClick, bool triggerEsc)
{
    if (m_gachaAnimPhase == GachaAnimPhase::None) return;

    ++m_gachaAnimTimer;

    // スキップ判定 (Enter / Space / Click)
    if (m_gachaAnimPhase != GachaAnimPhase::ResultCard)
    {
        if (triggerEnter || triggerSpace || triggerClick || triggerEsc)
        {
            SkipGachaAnimation();
            return;
        }
    }
    else
    {
        // 結果カード表示中：決定で閉じる
        if (triggerEnter || triggerSpace || triggerClick || triggerEsc)
        {
            CloseGachaAnimation();
            SoundManager::GetInstance().PlaySE(SeType::Decide);
            return;
        }
    }

    // 各フェーズの進行
    if (m_gachaAnimPhase == GachaAnimPhase::ShakeCharge)
    {
        m_gachaShakeX = std::sin(m_gachaAnimTimer * 0.95f) * (6.0f + m_gachaAnimTimer * 0.15f);
        m_gachaShakeY = std::cos(m_gachaAnimTimer * 1.3f)  * 4.0f;

        if (m_gachaAnimTimer % 12 == 0)
        {
            SoundManager::GetInstance().PlaySE(SeType::Countdown);
        }

        if (m_gachaAnimTimer >= 46)
        {
            m_gachaAnimPhase = GachaAnimPhase::MusclePump;
            m_gachaAnimTimer = 0;
            SoundManager::GetInstance().PlaySE(SeType::Cursor);
        }
    }
    else if (m_gachaAnimPhase == GachaAnimPhase::MusclePump)
    {
        float pulse = std::abs(std::sin(m_gachaAnimTimer * 0.18f));
        m_gachaShakeX = (pulse > 0.65f) ? (std::sin(m_gachaAnimTimer * 1.5f) * 9.0f) : 0.0f;
        m_gachaShakeY = (pulse > 0.65f) ? (std::cos(m_gachaAnimTimer * 1.5f) * 9.0f) : 0.0f;

        if (m_gachaAnimTimer % 18 == 0)
        {
            SoundManager::GetInstance().PlaySE(SeType::Countdown);
        }

        if (m_gachaAnimTimer >= 62)
        {
            m_gachaAnimPhase  = GachaAnimPhase::BulkExplosion;
            m_gachaAnimTimer  = 0;
            m_gachaShakeX     = 0.0f;
            m_gachaShakeY     = 0.0f;
            m_gachaWhiteFlash = 1.0f;
            SoundManager::GetInstance().PlaySE(SeType::SquatSuccess);
            SoundManager::GetInstance().PlaySE(SeType::Tackle);
        }
    }
    else if (m_gachaAnimPhase == GachaAnimPhase::BulkExplosion)
    {
        m_gachaWhiteFlash = 1.0f - (static_cast<float>(m_gachaAnimTimer) / 22.0f);
        if (m_gachaWhiteFlash < 0.0f) m_gachaWhiteFlash = 0.0f;

        if (m_gachaAnimTimer >= 22)
        {
            m_gachaAnimPhase  = GachaAnimPhase::ResultCard;
            m_gachaAnimTimer  = 0;
            m_gachaWhiteFlash = 0.0f;
        }
    }
}

void WorkoutCollectionManager::DrawGachaAnimationModal(int fontTitle, int fontSubtitle, int fontBody, int fontSmall)
{
    if (m_gachaAnimPhase == GachaAnimPhase::None) return;

    int sx = static_cast<int>(m_gachaShakeX);
    int sy = static_cast<int>(m_gachaShakeY);

    int cX = Config::SCREEN_WIDTH / 2 + sx;
    int cY = Config::SCREEN_HEIGHT / 2 + sy;

    // ------------------------------------------------------------------------
    // 背景暗幕レイヤー（フェーズに応じて背景の3D筋トレ猫の透過度を最適化）
    // ------------------------------------------------------------------------
    if (m_gachaAnimPhase == GachaAnimPhase::ResultCard)
    {
        // ResultCard時は左半分（猫の筋トレ実演エリア）を透明度高め(alpha: 80)にして3D猫を鮮明に見せる！
        // 右半分（カード配置エリア）はalpha: 190でカードテキストを際立たせる
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        DrawBox(0, 0, 620, Config::SCREEN_HEIGHT, GetColor(8, 12, 22), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(620, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 14, 26), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    else
    {
        // チャージ・爆発中は全体を程よく暗転 (alpha: 220)
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 14, 26), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ------------------------------------------------------------------------
    // Phase 1: ShakeCharge（プロテイン急速注入＆シェイク！）
    // ------------------------------------------------------------------------
    if (m_gachaAnimPhase == GachaAnimPhase::ShakeCharge)
    {
        // A. タイトルバナー
        const char* title = "★ PROTEIN SHAKE CHARGE !! ★";
        int tW = GetDrawStringWidthToHandle(title, static_cast<int>(strlen(title)), fontTitle);
        DrawStringToHandle(cX - tW / 2 + 2, 132 + sy, title, GetColor(0, 0, 0), fontTitle);
        DrawStringToHandle(cX - tW / 2,     130 + sy, title, GetColor(255, 170, 30), fontTitle);

        const char* sub = "プロテイン100ptを筋肉へ急速注入中…！ SHAKE! SHAKE! SHAKE!";
        int sW = GetDrawStringWidthToHandle(sub, static_cast<int>(strlen(sub)), fontBody);
        DrawStringToHandle(cX - sW / 2, 175 + sy, sub, GetColor(255, 230, 160), fontBody);

        // B. プロテインシェイカーのイラスト風オブジェクト
        int bottleW = 140;
        int bottleH = 220;
        int bX = cX - bottleW / 2;
        int bY = cY - 20;

        // シェイカー本体
        DrawRoundRect(bX, bY, bX + bottleW, bY + bottleH, 16, 16, GetColor(24, 48, 80), TRUE);
        DrawRoundRect(bX, bY, bX + bottleW, bY + bottleH, 16, 16, GetColor(100, 200, 255), FALSE);

        // プロテイン液面
        float fillRatio = MathHelper::Clamp(static_cast<float>(m_gachaAnimTimer) / 46.0f, 0.2f, 0.95f);
        int liquidH = static_cast<int>(bottleH * fillRatio);
        int waveY = bY + bottleH - liquidH;
        DrawRoundRect(bX + 4, waveY, bX + bottleW - 4, bY + bottleH - 4, 12, 12, GetColor(255, 140, 40), TRUE);

        // 泡
        SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
        DrawCircle(bX + 40, waveY + 20, 14, GetColor(255, 240, 180), TRUE);
        DrawCircle(bX + 90, waveY + 35, 10, GetColor(255, 240, 180), TRUE);
        DrawCircle(bX + 65, waveY + 60, 18, GetColor(255, 240, 180), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 目盛
        for (int m = 1; m <= 4; ++m)
        {
            int my = bY + bottleH - m * 38;
            DrawLine(bX + 10, my, bX + 35, my, GetColor(255, 255, 255));
        }

        // キャップ
        int capH = 34;
        DrawRoundRect(bX - 8, bY - capH, bX + bottleW + 8, bY + 4, 10, 10, GetColor(255, 100, 20), TRUE);
        DrawRoundRect(bX - 8, bY - capH, bX + bottleW + 8, bY + 4, 10, 10, GetColor(255, 220, 100), FALSE);
        DrawRoundRect(bX + bottleW / 2 - 16, bY - capH - 18, bX + bottleW / 2 + 16, bY - capH + 2, 6, 6, GetColor(220, 80, 10), TRUE);

        // ボトル前面のロゴ
        const char* pTxt = "PROTEIN";
        int pW = GetDrawStringWidthToHandle(pTxt, static_cast<int>(strlen(pTxt)), fontSubtitle);
        DrawStringToHandle(cX - pW / 2, bY + bottleH / 2 - 10, pTxt, GetColor(255, 255, 255), fontSubtitle);

        // C. 左右に飛び散るポップテキスト "SHAKE!"
        if ((m_gachaAnimTimer / 6) % 2 == 0)
        {
            DrawStringToHandle(bX - 90, bY + 30, "SHAKE!!", GetColor(255, 230, 60), fontBody);
            DrawStringToHandle(bX + bottleW + 20, bY + 90, "PUMP!!", GetColor(255, 120, 40), fontBody);
        }
        else
        {
            DrawStringToHandle(bX - 80, bY + 110, "POWER!!", GetColor(255, 100, 80), fontBody);
            DrawStringToHandle(bX + bottleW + 15, bY + 40, "BULK!!", GetColor(255, 220, 90), fontBody);
        }

        // スキップ案内
        const char* skipTxt = "※ [ENTER] / [SPACE] / クリック でスキップ";
        int skW = GetDrawStringWidthToHandle(skipTxt, static_cast<int>(strlen(skipTxt)), fontSmall);
        DrawStringToHandle(cX - skW / 2, Config::SCREEN_HEIGHT - 65, skipTxt, GetColor(150, 175, 205), fontSmall);
    }

    // ------------------------------------------------------------------------
    // Phase 2: MusclePump（集中線・パンプアップ！）
    // ------------------------------------------------------------------------
    else if (m_gachaAnimPhase == GachaAnimPhase::MusclePump)
    {
        // A. 回転コミック集中線
        int rayCount = 32;
        float rotAngle = m_gachaAnimTimer * 0.08f;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
        for (int i = 0; i < rayCount; ++i)
        {
            float a1 = rotAngle + (i * DX_PI_F * 2.0f / rayCount);
            float a2 = a1 + 0.05f;
            int px1 = cX + static_cast<int>(std::cos(a1) * 850.0f);
            int py1 = cY + static_cast<int>(std::sin(a1) * 850.0f);
            int px2 = cX + static_cast<int>(std::cos(a2) * 850.0f);
            int py2 = cY + static_cast<int>(std::sin(a2) * 850.0f);

            unsigned int rayCol = (i % 2 == 0) ? GetColor(255, 180, 40) : GetColor(255, 70, 30);
            DrawTriangle(cX, cY, px1, py1, px2, py2, rayCol, TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // B. パンプパルスリング
        float ringRadius = (m_gachaAnimTimer % 18) * 28.0f;
        int ringAlpha = static_cast<int>((1.0f - (m_gachaAnimTimer % 18) / 18.0f) * 220.0f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, ringAlpha);
        DrawCircle(cX, cY, static_cast<int>(ringRadius), GetColor(255, 230, 90), FALSE);
        DrawCircle(cX, cY, static_cast<int>(ringRadius) + 2, GetColor(255, 120, 50), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // C. 特大パンプメッセージ
        const char* pTitle = "★ PUMP UP MAXIMUM !! ★";
        int ptW = GetDrawStringWidthToHandle(pTitle, static_cast<int>(strlen(pTitle)), fontTitle);
        DrawStringToHandle(cX - ptW / 2 + 2, 112 + sy, pTitle, GetColor(0, 0, 0), fontTitle);
        DrawStringToHandle(cX - ptW / 2,     110 + sy, pTitle, GetColor(255, 230, 60), fontTitle);

        // スキップ案内
        const char* skipTxt = "※ [ENTER] / [SPACE] / クリック でスキップ";
        int skW = GetDrawStringWidthToHandle(skipTxt, static_cast<int>(strlen(skipTxt)), fontSmall);
        DrawStringToHandle(cX - skW / 2, Config::SCREEN_HEIGHT - 65, skipTxt, GetColor(150, 175, 205), fontSmall);
    }

    // ------------------------------------------------------------------------
    // Phase 3: BulkExplosion（白フラッシュ＆筋肉大爆発！）
    // ------------------------------------------------------------------------
    else if (m_gachaAnimPhase == GachaAnimPhase::BulkExplosion)
    {
        // 放射状の金スパーク
        for (int p = 0; p < 24; ++p)
        {
            float ang = p * (DX_PI_F * 2.0f / 24.0f);
            float dist = m_gachaAnimTimer * 22.0f;
            int spX = cX + static_cast<int>(std::cos(ang) * dist);
            int spY = cY + static_cast<int>(std::sin(ang) * dist);
            DrawCircle(spX, spY, 8, GetColor(255, 220, 60), TRUE);
            DrawCircle(spX, spY, 4, GetColor(255, 255, 255), TRUE);
        }

        // フラッシュレイヤー
        int alpha = static_cast<int>(m_gachaWhiteFlash * 255.0f);
        if (alpha > 0)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // ------------------------------------------------------------------------
    // Phase 4: ResultCard（獲得筋トレ大発表 ＆ 左側で3D猫が筋トレを実演中！）
    // ------------------------------------------------------------------------
    else if (m_gachaAnimPhase == GachaAnimPhase::ResultCard)
    {
        // --------------------------------------------------------------------
        // [左側] 猫の筋トレ実演ショーケース装飾
        // --------------------------------------------------------------------
        // 左上バッジ: ★ NEW WORKOUT SHOWCASE ★
        DrawRoundRect(45, 60, 45 + 480, 60 + 42, 12, 12, GetColor(28, 24, 46), TRUE);
        DrawRoundRect(45, 60, 45 + 480, 60 + 42, 12, 12, GetColor(255, 215, 60), FALSE);
        const char* scTitle = "★ NEW WORKOUT MOTION SHOWCASE ★";
        int scW = GetDrawStringWidthToHandle(scTitle, static_cast<int>(strlen(scTitle)), fontSubtitle);
        DrawStringToHandle(45 + (480 - scW) / 2, 69, scTitle, GetColor(255, 230, 80), fontSubtitle);

        // 左下バッジ: 【実演中】 〇〇 （リアルタイムで新筋トレを実践！）
        DrawRoundRect(45, Config::SCREEN_HEIGHT - 120, 45 + 520, Config::SCREEN_HEIGHT - 120 + 58, 14, 14, GetColor(18, 26, 42), TRUE);
        DrawRoundRect(45, Config::SCREEN_HEIGHT - 120, 45 + 520, Config::SCREEN_HEIGHT - 120 + 58, 14, 14, GetColor(80, 225, 140), FALSE);
        char showcaseBuf[64];
        snprintf(showcaseBuf, sizeof(showcaseBuf), "【実演中】 %s", m_gachaMotionName.c_str());
        DrawStringToHandle(65, Config::SCREEN_HEIGHT - 114, showcaseBuf, GetColor(255, 255, 255), fontTitle);
        DrawStringToHandle(65, Config::SCREEN_HEIGHT - 85, "※獲得したばかりの新筋トレモーションを全力で実践中！", GetColor(180, 230, 255), fontSmall);

        // --------------------------------------------------------------------
        // [右側] 獲得カード本体（右半分にスマート＆豪華に配置）
        // --------------------------------------------------------------------
        float popProgress = MathHelper::Clamp(static_cast<float>(m_gachaAnimTimer) / 10.0f, 0.0f, 1.0f);
        float popScale = 1.0f + std::sin(popProgress * DX_PI_F) * 0.06f;

        int baseCardW = 590;
        int baseCardH = 460;
        int cardW = static_cast<int>(baseCardW * popScale);
        int cardH = static_cast<int>(baseCardH * popScale);

        int cardX = 640 + (baseCardW - cardW) / 2;
        int cardY = (Config::SCREEN_HEIGHT - cardH) / 2;

        // 背景のゴージャスなゴールド放射光（カード中心）
        int cardCenterX = cardX + cardW / 2;
        int cardCenterY = cardY + cardH / 2;
        SetDrawBlendMode(DX_BLENDMODE_ADD, 50);
        for (int i = 0; i < 16; ++i)
        {
            float a1 = (m_gachaAnimTimer * 0.015f) + (i * DX_PI_F * 2.0f / 16.0f);
            float a2 = a1 + 0.16f;
            int px1 = cardCenterX + static_cast<int>(std::cos(a1) * 700.0f);
            int py1 = cardCenterY + static_cast<int>(std::sin(a1) * 700.0f);
            int px2 = cardCenterX + static_cast<int>(std::cos(a2) * 700.0f);
            int py2 = cardCenterY + static_cast<int>(std::sin(a2) * 700.0f);
            DrawTriangle(cardCenterX, cardCenterY, px1, py1, px2, py2, GetColor(255, 200, 60), TRUE);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 金色＆マゼンタネオンの豪華カード本体
        DrawRoundRect(cardX, cardY, cardX + cardW, cardY + cardH, 22, 22, GetColor(18, 22, 36), TRUE);
        DrawRoundRect(cardX, cardY, cardX + cardW, cardY + cardH, 22, 22, GetColor(255, 215, 50), FALSE);
        DrawRoundRect(cardX + 2, cardY + 2, cardX + cardW - 2, cardY + cardH - 2, 20, 20, GetColor(220, 90, 240), FALSE);

        // ヘッダーバー背景
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
        DrawRoundRect(cardX + 4, cardY + 4, cardX + cardW - 4, cardY + 54, 18, 18, GetColor(60, 45, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawLine(cardX + 16, cardY + 54, cardX + cardW - 16, cardY + 54, GetColor(255, 215, 50));

        // ヘッダータイトル
        const char* resTitle = "★ NEW MUSCLE UNLOCKED !! ★";
        int rtW = GetDrawStringWidthToHandle(resTitle, static_cast<int>(strlen(resTitle)), fontTitle);
        DrawStringToHandle(cardX + (cardW - rtW) / 2, cardY + 14, resTitle, GetColor(255, 225, 80), fontTitle);

        // 内側メイン紹介枠
        int innerX = cardX + 20;
        int innerY = cardY + 68;
        int innerW = cardW - 40;
        int innerH = cardH - 146;
        DrawRoundRect(innerX, innerY, innerX + innerW, innerY + innerH, 14, 14, GetColor(26, 32, 50), TRUE);
        DrawRoundRect(innerX, innerY, innerX + innerW, innerY + innerH, 14, 14, GetColor(80, 220, 130), FALSE);

        // モーション名（超特大アピール）
        std::string nameDisplay = "【 " + m_gachaMotionName + " 】";
        int nW = GetDrawStringWidthToHandle(nameDisplay.c_str(), static_cast<int>(nameDisplay.length()), fontTitle);
        DrawStringToHandle(cardX + (cardW - nW) / 2 + 2, innerY + 24, nameDisplay.c_str(), GetColor(0, 0, 0), fontTitle);
        DrawStringToHandle(cardX + (cardW - nW) / 2,     innerY + 22, nameDisplay.c_str(), GetColor(255, 245, 130), fontTitle);

        // 習得告知
        const char* addMsg = "★ 筋トレメニューに新種目追加！ ★";
        int amW = GetDrawStringWidthToHandle(addMsg, static_cast<int>(strlen(addMsg)), fontSubtitle);
        DrawStringToHandle(cardX + (cardW - amW) / 2, innerY + 76, addMsg, GetColor(110, 230, 255), fontSubtitle);

        // 鍛えられる部位
        int tgW = GetDrawStringWidthToHandle(m_gachaTargetMuscle.c_str(), static_cast<int>(m_gachaTargetMuscle.length()), fontBody);
        DrawStringToHandle(cardX + (cardW - tgW) / 2, innerY + 124, m_gachaTargetMuscle.c_str(), GetColor(230, 245, 255), fontBody);

        // 筋肉コール賞賛
        int mcW = GetDrawStringWidthToHandle(m_gachaMuscleCall.c_str(), static_cast<int>(m_gachaMuscleCall.length()), fontBody);
        DrawStringToHandle(cardX + (cardW - mcW) / 2, innerY + 170, m_gachaMuscleCall.c_str(), GetColor(255, 175, 60), fontBody);

        // 最下部決定ボタン（点滅・決定案内）
        int btnW = cardW - 60;
        int btnH = 46;
        int btnX = cardX + 30;
        int btnY = cardY + cardH - 62;

        float btnPulse = (std::sin(GetNowCount() * 0.012f) + 1.0f) * 0.5f;
        unsigned int btnFill = GetColor(255, 140 + static_cast<int>(50 * btnPulse), 40);
        DrawRoundRect(btnX, btnY, btnX + btnW, btnY + btnH, 20, 20, btnFill, TRUE);
        DrawRoundRect(btnX, btnY, btnX + btnW, btnY + btnH, 20, 20, GetColor(255, 255, 180), FALSE);

        const char* enterTxt = "★ 決定！筋トレを習得する (ENTER / クリック) ★";
        int eW = GetDrawStringWidthToHandle(enterTxt, static_cast<int>(strlen(enterTxt)), fontBody);
        DrawStringToHandle(btnX + (btnW - eW) / 2, btnY + 13, enterTxt, GetColor(20, 20, 30), fontBody);
    }
}
#ifdef _DEBUG
// ============================================================================
// ★ 筋トレコレクション デバッグモード機能の実装（Debugビルド時のみ）
// ============================================================================
void WorkoutCollectionManager::ToggleDebugMode()
{
    m_isDebugMode = !m_isDebugMode;
    if (m_isDebugMode)
    {
        SetStatusMessage("【Debugモード ON】[1:全解放] [2:+1000pt] [3:全ON] [4:全OFF] [5:初期化]", 240);
        SoundManager::GetInstance().PlaySE(SeType::SquatSuccess);
    }
    else
    {
        SetStatusMessage("【Debugモード OFF】通常モードに戻りました", 150);
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
    }
}

void WorkoutCollectionManager::DebugUnlockAll()
{
    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        m_unlockedIds.insert(i);
    }
    SyncToPlayer3D();
    SaveToFile();
    SetStatusMessage("【Debug】全9種の筋トレモーションを全解放しました！", 240);
    SoundManager::GetInstance().PlaySE(SeType::SquatSuccess);
}

void WorkoutCollectionManager::DebugAddProtein(int amount)
{
    AddProteinPoints(amount);
    SaveToFile();
    SetStatusMessage("【Debug】プロテイン +" + std::to_string(amount) + " pt 追加！ (所持: " + std::to_string(m_proteinPoints) + " pt)", 200);
    SoundManager::GetInstance().PlaySE(SeType::Decide);
}

void WorkoutCollectionManager::DebugEnableAllMotions()
{
    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        if (IsMotionUnlocked(i))
        {
            Player3D::SetWorkoutMotionEnabled(static_cast<WorkoutMotionType>(i), true);
        }
    }
    SetStatusMessage("【Debug】所持している全筋トレモーションをONに設定しました！", 200);
    SoundManager::GetInstance().PlaySE(SeType::Decide);
}

void WorkoutCollectionManager::DebugDisableAllMotions()
{
    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        Player3D::SetWorkoutMotionEnabled(static_cast<WorkoutMotionType>(i), (i == INITIAL_MOTION_ID));
    }
    SetStatusMessage("【Debug】腕立て伏せ以外をすべてOFFに設定しました！", 200);
    SoundManager::GetInstance().PlaySE(SeType::Cursor);
}
#endif
