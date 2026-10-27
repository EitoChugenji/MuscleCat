// ============================================================================
// @file   ObjectManager3D.h
// @brief  3D空間内の全オブジェクト（プレイヤー・ネズミ・障害物・ステージ）の統合管理
// ============================================================================
#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Stage3D.h"

class GameObject3D;
class Player3D;
class Obstacle3D;
class Camera3D;

// ============================================================================
// ObjectManager3D クラス
// プレイヤー・ネズミ群・障害物・3Dステージの一括更新、描画、壁/床衝突解決、
// ネズミ捕獲判定、コンボ集計、ヒットストップ、捕獲ポップアップ演出を管理する
// ============================================================================
class ObjectManager3D
{
private:
    std::vector<std::shared_ptr<GameObject3D>> m_objects;               // 動的オブジェクト（ネズミ等）のリスト
    std::vector<std::shared_ptr<Obstacle3D>>   m_obstacles;             // 静的障害物（家具等）のリスト
    std::shared_ptr<Player3D>                  m_player;                // プレイヤー（マッスルねこ）
    Stage3D                                    m_stage;                 // 3Dステージ（床・壁コリジョン）
    int                                        m_caughtCount       = 0; // 捕獲したネズミの合計数
    int                                        m_caughtNormalCount = 0; // 捕獲した通常ネズミの数
    int                                        m_caughtFastCount   = 0; // 捕獲した高速ネズミの数
    bool                                       m_isTimeFrozen      = false; // デバッグ用：ネズミの行動停止フラグ
    bool                                       m_showDebugHitboxes = false; // デバッグ用：当たり判定可視化フラグ
    int                                        m_hitStopFrames     = 0; // 捕獲瞬間のヒットストップ残りフレーム数

    // ------------------------------------------------------------------------
    // ネズミ捕獲時に3D座標から浮かび上がるコミック風ポップアップ構造体
    // ------------------------------------------------------------------------
    struct CatchPopup
    {
        VECTOR       worldPos   = VGet(0.0f, 0.0f, 0.0f); // 捕獲発生地点のワールド座標
        int          timer      = 55;                     // 残り表示フレーム数
        int          maxTimer   = 55;                     // 最大表示フレーム数（進行度算出用）
        std::string  titleText;                           // メイン見出し ("★ CATCH!! ★", "🔥 2 COMBO!! 🔥" 等)
        std::string  subText;                             // サブテキスト ("+1 POINT!", "NICE CATCH! +1" 等)
        unsigned int titleColor = 0;                      // メイン文字カラー
        unsigned int badgeColor = 0;                      // カード外枠カラー
        int          combo      = 1;                      // 捕獲時のコンボ数
        bool         isFast     = false;                  // 高速ネズミ捕獲かどうか
    };

    std::vector<CatchPopup>                    m_catchPopups;           // 現在表示中の捕獲ポップアップ配列
    int                                        m_comboCount      = 0;   // 連続捕獲コンボ数（約3.5秒以内の連続捕獲で加算）
    int                                        m_comboTimer      = 0;   // コンボ持続残りフレーム（210F = 3.5秒）
    int                                        m_catchFlashTimer = 0;   // 捕獲瞬間の画面外周ゴールドフラッシュ残りフレーム

public:
    ObjectManager3D() = default;
    ~ObjectManager3D() = default;

    // 全オブジェクト・スコア・演出状態のクリア
    void Clear();

    // ステージの初期化およびマップ切り替えロード
    void InitStage(ModelConfig::MapType mapType = ModelConfig::MapType::SlopeHills);
    void LoadStage(ModelConfig::MapType mapType);

    // プレイヤー・オブジェクト・障害物の登録＆取得
    void SetPlayer(std::shared_ptr<Player3D> player);
    std::shared_ptr<Player3D> GetPlayer() const { return m_player; }

    Stage3D&       GetStage()       { return m_stage; }
    const Stage3D& GetStage() const { return m_stage; }

    void AddObject(std::shared_ptr<GameObject3D> obj);
    void AddObstacle(std::shared_ptr<Obstacle3D> obs);

    const std::vector<std::shared_ptr<GameObject3D>>& GetObjects() const   { return m_objects; }
    const std::vector<std::shared_ptr<Obstacle3D>>&   GetObstacles() const { return m_obstacles; }

    // デバッグチート用：ネズミ時間停止制御
    void SetTimeFrozen(bool frozen) { m_isTimeFrozen = frozen; }
    bool IsTimeFrozen() const       { return m_isTimeFrozen; }
    void ToggleTimeFrozen()         { m_isTimeFrozen = !m_isTimeFrozen; }

    // 捕獲時のヒットストップ発動・判定
    void TriggerHitStop(int frames)
    {
        if (frames > m_hitStopFrames)
        {
            m_hitStopFrames = frames;
        }
    }
    bool IsHitStopping() const { return m_hitStopFrames > 0; }

    // デバッグ当たり判定表示切替
    bool IsDebugHitboxesEnabled() const     { return m_showDebugHitboxes; }
    void ToggleDebugHitboxes()              { m_showDebugHitboxes = !m_showDebugHitboxes; }
    void SetDebugHitboxes(bool enabled)     { m_showDebugHitboxes = enabled; }

    // デバッグ用：ネズミの動的追加スポーン
    void SpawnNormalMouse();
    void SpawnFastMouse();

    // 毎フレームの一括更新・3D描画・2Dオーバーレイ描画
    void Update(Camera3D& camera);
    void Draw3D();
    void Draw2D();

    // プレイヤーとネズミの接触（捕獲 または タックル気絶）判定
    void CheckCollisions(Camera3D& camera);

    // プレイヤー・ネズミと障害物・ステージ壁・床面の衝突解決および崖落ち防止
    void CheckObstacleAndStageCollisions(Camera3D& camera);

    // 残りネズミ数・捕獲数・コンボ数の取得
    int   GetRemainingMouseCount() const;
    int   GetCaughtCount() const       { return m_caughtCount; }
    int   GetCaughtNormalCount() const { return m_caughtNormalCount; }
    int   GetCaughtFastCount() const   { return m_caughtFastCount; }
    int   GetComboCount() const        { return m_comboCount; }
    int   GetComboTimer() const        { return m_comboTimer; }

    // 現在の捕獲数に応じたネズミのスピード算出・反映
    float GetCurrentMouseSpeed() const;
    void  ApplyMouseSpeedBonus();
};
