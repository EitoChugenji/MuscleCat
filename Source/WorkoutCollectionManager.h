#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include "Player3D.h"

// ============================================================================
// 筋トレコレクション管理クラス (WorkoutCollectionManager)
// 
// 仕様:
// 1. A/E 二進数システム (4桁二進数: 1 -> A, 0 -> E)
// 2. 初期状態: 腕立て伏せ (ID: 1 / EEEA) のみ解放
// 3. 筋トレ完了時にプロテインポイント獲得
// 4. ポイント消費による未所持ランダムガチャ
// 5. セーブコード生成・読み込み（クリップボード連携対応）
// 6. デバッグ用全データ初期化（確認ダイアログ付き）
// ============================================================================
class WorkoutCollectionManager
{
public:
    static constexpr int GACHA_COST          = 100; // ガチャ1回の消費ポイント
    static constexpr int POINTS_PER_WORKOUT  = 100; // 筋トレ1回完了時の獲得ポイント
    static constexpr int INITIAL_MOTION_ID   = 1;   // 初期解放: 腕立て伏せ (PushUp / EEEA)

    static WorkoutCollectionManager& GetInstance()
    {
        static WorkoutCollectionManager s_instance;
        return s_instance;
    }

    // ------------------------------------------------------------------------
    // A/E 二進数システム (4桁: 1 -> A, 0 -> E)
    // ------------------------------------------------------------------------
    static std::string IdToAeCode(int id);
    static int         AeCodeToId(const std::string& code);

    // ------------------------------------------------------------------------
    // 初期化・ライフサイクル
    // ------------------------------------------------------------------------
    void Initialize();
    void ResetToInitialState(); // デバッグ用初期化（腕立て伏せのみ・0ptにリセット）

    // ------------------------------------------------------------------------
    // プロテインポイント
    // ------------------------------------------------------------------------
    int  GetProteinPoints() const { return m_proteinPoints; }
    void AddProteinPoints(int points);
    bool ConsumeProteinPoints(int points);
    void OnWorkoutCompleted(); // 筋トレ完了通知 (+POINTS_PER_WORKOUT)

    // ------------------------------------------------------------------------
    // 所持・コレクション管理
    // ------------------------------------------------------------------------
    bool IsMotionUnlocked(int id) const;
    bool IsMotionUnlocked(WorkoutMotionType type) const;
    void UnlockMotion(int id);
    int  GetUnlockedCount() const { return static_cast<int>(m_unlockedIds.size()); }
    int  GetTotalMotionCount() const { return static_cast<int>(WorkoutMotionType::Count); }
    bool IsAllUnlocked() const { return GetUnlockedCount() >= GetTotalMotionCount(); }
    const std::unordered_set<int>& GetUnlockedIds() const { return m_unlockedIds; }

    // ------------------------------------------------------------------------
    // ガチャシステム
    // ------------------------------------------------------------------------
    struct GachaResult
    {
        bool        success;     // ガチャ成功フラグ
        int         obtainedId;  // 獲得したモーションID (-1: 失敗またはコンプリート)
        std::string aeCode;      // A/Eコード (例: "EAEE")
        std::string motionName;  // モーション名 (例: "サイドレイズ")
        std::string message;     // UI用メッセージ
    };
    GachaResult RollGacha();

    // ------------------------------------------------------------------------
    // セーブ・ロード機能（セーブコード方式）
    // ------------------------------------------------------------------------
    std::string GenerateSaveCode() const;
    bool        LoadSaveCode(const std::string& saveCode);
    void        CopySaveCodeToClipboard() const;
    bool        PasteSaveCodeFromClipboard();

    // ローカルファイルへの自動バックアップ（QoL機能）
    void SaveToFile(const char* filePath = "SaveData.txt") const;
    bool LoadFromFile(const char* filePath = "SaveData.txt");

    // ------------------------------------------------------------------------
    // デバッグ・確認ダイアログ
    // ------------------------------------------------------------------------
    bool IsConfirmDialogOpen() const { return m_isConfirmDialogOpen; }
    void OpenConfirmDialog()         { m_isConfirmDialogOpen = true; }
    void CloseConfirmDialog()        { m_isConfirmDialogOpen = false; }
    void ExecuteConfirmedReset();

    // ------------------------------------------------------------------------
    // UI・状態管理
    // ------------------------------------------------------------------------
    const std::string& GetStatusMessage() const { return m_statusMessage; }
    int                GetStatusMessageTimer() const { return m_statusMessageTimer; }
    void               SetStatusMessage(const std::string& msg, int durationFrames = 180);

    // UIボタン判定・操作処理
    bool HandleUIMouseClick(int mx, int my, int cardX, int cardY, int cardW, int cardH);
    void Update();

    // 確認ダイアログの描画
    void DrawConfirmDialogModal(int fontTitle, int fontBody);

private:
    WorkoutCollectionManager();
    ~WorkoutCollectionManager() = default;
    WorkoutCollectionManager(const WorkoutCollectionManager&) = delete;
    WorkoutCollectionManager& operator=(const WorkoutCollectionManager&) = delete;

    // プレイヤーの利用可能フラグを同期
    void SyncToPlayer3D();

    int                     m_proteinPoints;
    std::unordered_set<int> m_unlockedIds;
    bool                    m_isConfirmDialogOpen;

    // UI演出用
    std::string             m_statusMessage;
    int                     m_statusMessageTimer;
    std::string             m_lastGachaName;
    std::string             m_lastGachaCode;
};
