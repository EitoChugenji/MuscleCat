// ============================================================================
// @file   GameManager.cpp
// @brief  GameManager クラスの実装（タイトル・ゲーム本編・リザルト・ポーズ・デバッグ）
// ============================================================================
#include "GameManager.h"
#include "SoundManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "CustomEffectManager.h"
#include "CsvManager.h"
#include "ModelConfig.h"
#include "Config.h"
#include "Common.h"
#include "DxLib.h"
#include <cstdlib>

// ============================================================================
// ふわっとしたUI共通描画ヘルパー（角丸・多層ソフトシャドウ・マシュマロフロスト・浮遊感）
// ============================================================================
namespace SoftUI
{
    // ふわっとしたソフト角丸カード描画
    inline void DrawSoftCard(int x, int y, int w, int h, int radius,
                             unsigned int bgColor, int bgAlpha = 220,
                             unsigned int borderColor = 0, int borderThickness = 1,
                             bool hasGlow = false, unsigned int glowColor = 0)
    {
        // 1. 3層の拡散ソフトドロップシャドウ（ふんわりした落ち影）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 24);
        DrawRoundRect(x - 3, y + 5, x + w + 3, y + h + 8, radius + 4, radius + 4, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 50);
        DrawRoundRect(x - 1, y + 3, x + w + 1, y + h + 6, radius + 2, radius + 2, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
        DrawRoundRect(x, y + 2, x + w, y + h + 3, radius, radius, GetColor(0, 0, 0), TRUE);

        // 2. ほんのり外側ソフトグロー（フォーカス・選択時）
        if (hasGlow)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
            DrawRoundRect(x - 4, y - 4, x + w + 4, y + h + 4, radius + 4, radius + 4, glowColor, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            DrawRoundRect(x - 2, y - 2, x + w + 2, y + h + 2, radius + 2, radius + 2, glowColor, TRUE);
        }

        // 3. メインボディ（角丸＋半透明ブレンド）
        if (bgAlpha < 255)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, bgAlpha);
        }
        else
        {
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
        DrawRoundRect(x, y, x + w, y + h, radius, radius, bgColor, TRUE);

        // 4. 上部ソフトハイライト（マシュマロ・ガラス風のやわらかい光沢）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 38);
        int hiH = h / 3;
        if (hiH > 2)
        {
            int hiR = (radius > 4) ? (radius - 2) : 2;
            DrawRoundRect(x + 2, y + 2, x + w - 2, y + hiH, hiR, hiR, GetColor(255, 255, 255), TRUE);
        }

        // 5. 境界アウトライン
        if (borderColor != 0)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            DrawRoundRect(x, y, x + w, y + h, radius, radius, borderColor, FALSE);
            if (borderThickness > 1)
            {
                DrawRoundRect(x - 1, y - 1, x + w + 1, y + h + 1, radius + 1, radius + 1, borderColor, FALSE);
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ふわふわの雲・マシュマロ吹き出し描画
    inline void DrawFluffySpeechBubble(int x, int y, int w, int h, int radius,
                                       unsigned int bgColor, unsigned int textColor,
                                       bool isRight, const char* text, int fontHandle)
    {
        // 1. 拡散ソフトシャドウ
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 25);
        DrawRoundRect(x - 2, y + 4, x + w + 2, y + h + 7, radius + 3, radius + 3, GetColor(0, 0, 0), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 55);
        DrawRoundRect(x, y + 2, x + w, y + h + 4, radius, radius, GetColor(0, 0, 0), TRUE);

        // 2. 吹き出し本体（角丸マシュマロ）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 245);
        DrawRoundRect(x, y, x + w, y + h, radius, radius, bgColor, TRUE);

        // 3. ふんわり上部ハイライト
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 50);
        DrawRoundRect(x + 3, y + 2, x + w - 3, y + (h / 2), (radius - 2 > 1) ? (radius - 2) : 1, (radius - 2 > 1) ? (radius - 2) : 1, GetColor(255, 255, 255), TRUE);

        // 4. まるっこい接続バブル（しっぽの代わりにふわっと浮かぶ小さな丸）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 240);
        if (isRight)
        {
            DrawCircle(x + 16, y + h + 4, 5, bgColor, TRUE);
            DrawCircle(x + 7,  y + h + 10, 3, bgColor, TRUE);
        }
        else
        {
            DrawCircle(x + w - 16, y + h + 4, 5, bgColor, TRUE);
            DrawCircle(x + w - 7,  y + h + 10, 3, bgColor, TRUE);
        }

        // 5. 優しい白光リム
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
        DrawRoundRect(x, y, x + w, y + h, radius, radius, GetColor(255, 255, 255), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 6. テキスト描画（柔らかい影＋本体）
        DrawStringToHandle(x + 15, y + 11, text, GetColor(20, 25, 35), fontHandle);
        DrawStringToHandle(x + 14, y + 10, text, textColor, fontHandle);
    }
}

namespace
{
    // 設定スライダーカード共通描画ヘルパー（マウス感度・BGM音量・SE音量）
    void DrawSettingSliderCard(
        int x, int y, int w, int h,
        bool isSelected,
        const char* label,
        const char* valueStr,
        float currentRatio,
        unsigned int barColor,
        bool hoverMinus,
        bool hoverPlus,
        int font16, int font13
    )
    {
        // 1. 背景カード（選択時はゴールド枠・グロー）
        SoftUI::DrawSoftCard(x, y, w, h, 16,
                             isSelected ? GetColor(28, 38, 62) : GetColor(24, 30, 48), 235,
                             isSelected ? GetColor(255, 180, 50) : GetColor(50, 65, 95),
                             isSelected ? 2 : 1, isSelected, GetColor(255, 180, 50));

        // 2. ラベルと現在値
        DrawStringToHandle(x + 14, y + 8, label, isSelected ? GetColor(255, 230, 90) : GetColor(220, 235, 255), font16);

        int valW = GetDrawStringWidthToHandle(valueStr, static_cast<int>(strlen(valueStr)), font16);
        DrawStringToHandle(x + w - valW - 14, y + 8, valueStr, isSelected ? GetColor(255, 215, 60) : GetColor(160, 220, 255), font16);

        // 3. [-] ボタン
        int btnW = 46;
        int btnH = 26;
        int btnY = y + 33;
        int minusX = x + 10;
        int plusX = x + w - 56;
        int barX = x + 64;
        int barW = w - 128;
        int barY = y + 40;
        int barH = 12;

        SoftUI::DrawSoftCard(minusX, btnY, btnW, btnH, 8,
                             hoverMinus ? GetColor(60, 90, 140) : GetColor(38, 48, 75), 235,
                             hoverMinus ? GetColor(100, 220, 255) : GetColor(70, 85, 120), 1,
                             hoverMinus, GetColor(100, 220, 255));
        DrawStringToHandle(minusX + 11, btnY + 4, "◀ -", hoverMinus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font13);

        // 4. スライダー溝背景
        DrawRoundRect(barX, barY, barX + barW, barY + barH, 6, 6, GetColor(12, 16, 26), TRUE);
        DrawRoundRect(barX, barY, barX + barW, barY + barH, 6, 6, GetColor(60, 75, 105), FALSE);

        // 5. 塗りつぶしプログレス
        float r = MathHelper::Clamp(currentRatio, 0.0f, 1.0f);
        int fillW = static_cast<int>(barW * r);
        if (fillW > 0)
        {
            int rVal = (fillW > 6) ? 5 : 2;
            DrawRoundRect(barX + 1, barY + 1, barX + fillW, barY + barH - 1, rVal, rVal, barColor, TRUE);
        }

        // 6. スライダーつまみ（ノブ）
        int knobX = barX + fillW;
        int knobY = barY + barH / 2;
        DrawCircle(knobX, knobY, 9, GetColor(0, 0, 0), TRUE);
        DrawCircle(knobX, knobY, 8, isSelected ? GetColor(255, 230, 80) : GetColor(220, 240, 255), TRUE);
        DrawCircle(knobX, knobY, 4, GetColor(30, 40, 60), TRUE);

        // 7. [+] ボタン
        SoftUI::DrawSoftCard(plusX, btnY, btnW, btnH, 8,
                             hoverPlus ? GetColor(60, 90, 140) : GetColor(38, 48, 75), 235,
                             hoverPlus ? GetColor(100, 220, 255) : GetColor(70, 85, 120), 1,
                             hoverPlus, GetColor(100, 220, 255));
        DrawStringToHandle(plusX + 11, btnY + 4, "+ ▶", hoverPlus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font13);
    }
}

void GameManager::Init()
{
    m_state = GameState::Title;
    m_titleCameraAngle = 0.0f;
    m_frameCount = 0;
    m_fpsTimer = GetNowCount();
    m_prevMouseLeft = true; // 起動直後のクリック暴発防止

    m_objManager.InitStage(m_selectedMap);
    SetupTitle();
}

void GameManager::SetupRoomObstacles()
{
    // ★ ユーザーリクエスト: 家具を一度どかす（家具配置を無効化）
    return;

    // AccessoryModels（キッチン、冷蔵庫、テーブル、テレビ）を部屋・ステージに配置
    // 各家具の接地高さ（床・高台・スロープ）を自動取得して配置
    const auto& stage = m_objManager.GetStage();
    auto PlaceObstacle = [&](const VECTOR& basePos, float width, float height, float depth,
                             const std::string& name, const std::string& modelPath,
                             unsigned int mainColor, unsigned int frameColor, float scale,
                             float rotY, const VECTOR& modelOffset) {
        VECTOR pos = basePos;
        float groundY = 0.0f;
        if (stage.GetGroundHeight(pos, groundY))
        {
            pos.y = groundY;
        }
        m_objManager.AddObstacle(std::make_shared<Obstacle3D>(
            pos, width, height, depth, name, modelPath, mainColor, frameColor, scale, rotY, modelOffset
        ));
    };

    if (m_selectedMap == ModelConfig::MapType::House)
    {
        // 1. 北東大部屋（キッチン＆ダイニングエリア）
        // システムキッチン（北壁沿い）
        PlaceObstacle(
            VGet(520.0f, 0.0f, 600.0f),
            300.0f, 75.0f, 70.0f,
            "", ModelConfig::KITCHEN_MODEL_PATH,
            GetColor(200, 200, 210), GetColor(150, 150, 160),
            ModelConfig::KITCHEN_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y, 0.0f)
        );

        // 冷蔵庫（キッチンの隣・北壁沿い）
        PlaceObstacle(
            VGet(740.0f, 0.0f, 600.0f),
            80.0f, 160.0f, 64.0f,
            "", ModelConfig::REFRIGERATOR_MODEL_PATH,
            GetColor(220, 220, 230), GetColor(160, 160, 170),
            ModelConfig::REFRIGERATOR_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y, 0.0f)
        );

        // ダイニングテーブル（北東部屋中央）
        PlaceObstacle(
            VGet(600.0f, 0.0f, 360.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y, 0.0f)
        );

        // 2. メインリビング（中央エリア）
        // リビングのテレビ（西壁沿い・東向き）
        PlaceObstacle(
            VGet(-270.0f, 0.0f, 50.0f),
            30.0f, 60.0f, 120.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE,
            DX_PI_F * 0.5f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
        );

        // リビングのテーブル
        PlaceObstacle(
            VGet(-100.0f, 0.0f, 120.0f),
            140.0f, 50.0f, 140.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            70.0f,
            0.0f,
            VGet(0.0f, 35.0f, 0.0f)
        );

        // 3. 南東奥部屋（寝室/個室エリア）
        // 奥部屋のテレビ（南壁沿い・北向き）
        PlaceObstacle(
            VGet(600.0f, 0.0f, -820.0f),
            120.0f, 60.0f, 30.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE,
            0.0f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y, 0.0f)
        );

        // 奥部屋のテーブル
        PlaceObstacle(
            VGet(480.0f, 0.0f, -700.0f),
            120.0f, 45.0f, 120.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            60.0f,
            0.0f,
            VGet(0.0f, 30.0f, 0.0f)
        );

        // 4. 東部屋のテーブル
        PlaceObstacle(
            VGet(700.0f, 0.0f, -100.0f),
            140.0f, 50.0f, 140.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            70.0f,
            0.0f,
            VGet(0.0f, 35.0f, 0.0f)
        );
    }
    else
    {
        // NEW MAP (newmap.mv1) 用の家具配置（各部屋・通路エリア）
        // 1. 北東エリアのシステムキッチン
        PlaceObstacle(
            VGet(600.0f, 0.0f, 800.0f),
            300.0f, 75.0f, 70.0f,
            "", ModelConfig::KITCHEN_MODEL_PATH,
            GetColor(200, 200, 210), GetColor(150, 150, 160),
            ModelConfig::KITCHEN_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::KITCHEN_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 2. 北東エリアの冷蔵庫
        PlaceObstacle(
            VGet(850.0f, 0.0f, 800.0f),
            80.0f, 160.0f, 64.0f,
            "", ModelConfig::REFRIGERATOR_MODEL_PATH,
            GetColor(220, 220, 230), GetColor(160, 160, 170),
            ModelConfig::REFRIGERATOR_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::REFRIGERATOR_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 3. 中央西エリアのテーブル
        PlaceObstacle(
            VGet(-500.0f, 0.0f, 600.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 4. 中央西エリアのテレビ
        PlaceObstacle(
            VGet(-500.0f, 0.0f, 150.0f),
            30.0f, 60.0f, 120.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.2f,
            DX_PI_F * 0.5f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 5. 東エリアのテーブル
        PlaceObstacle(
            VGet(1600.0f, 0.0f, -400.0f),
            150.0f, 50.0f, 150.0f,
            "", ModelConfig::TABLE_MODEL_PATH,
            GetColor(180, 140, 100), GetColor(140, 100, 70),
            ModelConfig::TABLE_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TABLE_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );

        // 6. 南東エリアのテレビ
        PlaceObstacle(
            VGet(800.0f, 0.0f, -1400.0f),
            120.0f, 60.0f, 30.0f,
            "", ModelConfig::TV_MODEL_PATH,
            GetColor(40, 40, 45), GetColor(20, 20, 25),
            ModelConfig::TV_MODEL_SCALE * 1.2f,
            0.0f,
            VGet(0.0f, ModelConfig::TV_MODEL_OFFSET_Y * 1.2f, 0.0f)
        );
    }
}

void GameManager::SwitchSelectedMap(ModelConfig::MapType newMap)
{
    if (m_selectedMap != newMap)
    {
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
        m_selectedMap = newMap;
        m_objManager.LoadStage(m_selectedMap);

        // タイトル画面中の猫の足元高さを再取得
        auto player = m_objManager.GetPlayer();
        if (player)
        {
            player->SetMapType(m_selectedMap);
            // Map1は床(Y≈5)、Map2は中央の円柱台座上(Y=200)に配置
            float gy = (m_selectedMap == ModelConfig::MapType::House) ? 5.0f : 200.0f;
            if (m_selectedMap == ModelConfig::MapType::House) m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), gy);
            player->SetPos(VGet(0.0f, gy, 0.0f));
            player->SetGroundY(gy);
            if (false)
            {
                // unused
                player->SetGroundY(gy);
            }
        }

        // 家具の配置を更新
        m_objManager.Clear();
        if (player)
        {
            m_objManager.SetPlayer(player);
        }
        SetupRoomObstacles();
    }
}

void GameManager::SetupTitle()
{
    m_state = GameState::Title;
    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();
    CustomEffectManager::GetInstance().Clear();

    // 選択中マップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);

    // タイトル画面：マッチョな猫を生成（Rep=6の堂々たる姿）
    auto player = std::make_shared<Player3D>(VGet(0.0f, 0.0f, 0.0f));
    player->AddRep(6); // Lv.6のマッチョ体格
    player->SetMapType(m_selectedMap);

    // Map1は床(Y≈5)、Map2は中央の円柱台座上(Y=200)に配置
    float groundY = (m_selectedMap == ModelConfig::MapType::House) ? 5.0f : 200.0f;
    if (m_selectedMap == ModelConfig::MapType::House) m_objManager.GetStage().GetGroundHeight(VGet(0.0f, 0.0f, 0.0f), groundY);
    player->SetPos(VGet(0.0f, groundY, 0.0f));
    player->SetGroundY(groundY);
    if (false)
    {
        // unused
        player->SetGroundY(groundY);
    }
    m_objManager.SetPlayer(player);

    m_titleCameraAngle = -0.7f;
    m_showHowToPlay = false;
    m_showSoundSettings = false;
    m_titleSoundSettingItem = 0;
    m_isPaused = false;
    m_pauseStartTime = 0;
    m_pauseMenuItem = 0;
    m_startTransitionTimer = 0;
    m_titleAnimTimer = 0;
    m_selectedMenuItem = 0;

    SetupRoomObstacles();
    SoundManager::GetInstance().PlayBGM(BgmType::Title);
}

void GameManager::StartGame()
{
    // ゲーム開始・リトライ時に最新のCSV設定（Statu.csv / Position.csv）を読み込み反映
    CsvManager::GetInstance().ReloadAll();

    m_objManager.Clear();
    EffectManager::GetInstance().StopMuscleAura();
    CustomEffectManager::GetInstance().Clear();
    m_isPaused = false;
    m_pauseStartTime = 0;
    m_pauseMenuItem = 0;

    // 選択されたマップのステージ読み込み
    m_objManager.LoadStage(m_selectedMap);
    const auto& stage = m_objManager.GetStage();

    // プレイヤー生成（Position.csv の Cat 行から初期位置を取得）
    VECTOR defaultStartPos = (m_selectedMap == ModelConfig::MapType::House)
        ? VGet(0.0f, 0.0f, 0.0f)
        : VGet(0.0f, 0.0f, 450.0f);

    VECTOR startPos = defaultStartPos;
    std::vector<VECTOR> catPositions = CsvManager::GetInstance().GetCatPositionsForMap(m_selectedMap);
    if (!catPositions.empty())
    {
        VECTOR candidate = catPositions[rand() % catPositions.size()];
        float testGy = 0.0f;
        if (stage.GetGroundHeight(candidate, testGy))
        {
            startPos = VGet(candidate.x, testGy, candidate.z);
        }
    }

    auto player = std::make_shared<Player3D>(startPos);
    player->SetMapType(m_selectedMap);
    float playerGy = 0.0f;
    if (stage.GetGroundHeight(startPos, playerGy))
    {
        player->SetPos(VGet(startPos.x, playerGy, startPos.z));
        player->SetGroundY(playerGy);
    }
    m_objManager.SetPlayer(player);

    // カメラ初期化（猫の背後から開始）
    m_camera.Init(player->GetPos(), player->GetRotY());
    SetMouseDispFlag(FALSE); // ゲームプレイ中はカーソル非表示（マウス視点移動）

    // 部屋・ステージの家具配置
    SetupRoomObstacles();

    // ネズミの生成（Position.csv の Mouse 行からスポーン候補座標を取得）
    std::vector<VECTOR> spawnRooms = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    if (spawnRooms.empty())
    {
        spawnRooms.push_back(defaultStartPos);
    }
    int roomCount = static_cast<int>(spawnRooms.size());

    // ステージモデルの床上に確実に接地させるスポーン座標決定ヘルパー
    auto FindGuaranteedGroundSpawnPos = [&](int candidateIndex) -> VECTOR
    {
        for (int offsetAttempt = 0; offsetAttempt < roomCount; ++offsetAttempt)
        {
            int rIdx = (candidateIndex + offsetAttempt) % roomCount;
            const VECTOR& basePos = spawnRooms[rIdx];

            for (int jitterAttempt = 0; jitterAttempt < 6; ++jitterAttempt)
            {
                float ox = (jitterAttempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
                float oz = (jitterAttempt == 0) ? 0.0f : static_cast<float>((rand() % 40) - 20);
                VECTOR testPos = VGet(basePos.x + ox, basePos.y, basePos.z + oz);
                float gy = 0.0f;
                if (stage.GetGroundHeight(testPos, gy))
                {
                    testPos.y = gy;
                    return testPos;
                }
            }
        }

        // 万一のセーフティネット：猫の初期位置近傍
        float fallbackGy = 0.0f;
        VECTOR fallbackPos = defaultStartPos;
        if (stage.GetGroundHeight(fallbackPos, fallbackGy))
        {
            fallbackPos.y = fallbackGy;
        }
        return fallbackPos;
    };

    for (int i = 0; i < Config::NORMAL_MOUSE_COUNT; ++i)
    {
        VECTOR spawnPos = FindGuaranteedGroundSpawnPos(i);
        auto mouse = std::make_shared<NormalMouse3D>(spawnPos, player);
        mouse->SetMapType(m_selectedMap);
        m_objManager.AddObject(mouse);
    }

    for (int i = 0; i < Config::FAST_MOUSE_COUNT; ++i)
    {
        VECTOR spawnPos = FindGuaranteedGroundSpawnPos(Config::NORMAL_MOUSE_COUNT + i);
        auto mouse = std::make_shared<FastMouse3D>(spawnPos, player);
        mouse->SetMapType(m_selectedMap);
        m_objManager.AddObject(mouse);
    }

    m_startCount = GetNowCount();
    m_clearCount = 0;
    m_clearTimeSeconds = 0.0f;
    m_remainingTimeSeconds = Config::TIME_LIMIT_SECONDS;
    m_isTimeUp = false;
    m_state = GameState::Playing;
    SoundManager::GetInstance().PlayBGM(BgmType::Stage);
}

void GameManager::Update()
{
    if (m_state == GameState::Title)
    {
        SetMouseDispFlag(TRUE); // タイトル画面ではカーソル表示
        m_titleAnimTimer++;

        // シネマティックカメラ演出：猫を中心に低アングルから優雅に旋回（全身が綺麗に収まる距離感に調整）
        m_titleCameraAngle += 0.0055f;
        auto player = m_objManager.GetPlayer();
        float catY = (player) ? player->GetPos().y : 0.0f;
        float camDist = 240.0f;

        // 猫のアニメーション：スクワット運動と腕立て伏せ（プッシュアップ）を交互にトレーニング！
        int cycle = m_titleAnimTimer % 540;
        bool isPushUp = (cycle >= 270 && cycle < 470);
        float targetY = isPushUp ? (catY + 14.0f) : (catY + 20.0f);
        float camHeight = (isPushUp ? (catY + 34.0f) : (catY + 38.0f)) + std::sin(m_titleAnimTimer * 0.02f) * 5.0f;

        VECTOR catPos = (player) ? player->GetPos() : VGet(0.0f, 0.0f, 0.0f);
        VECTOR center = VGet(catPos.x, targetY, catPos.z);
        VECTOR camPos = VGet(
            catPos.x + std::sin(m_titleCameraAngle) * camDist,
            camHeight,
            catPos.z + std::cos(m_titleCameraAngle) * camDist
        );
        SetCameraPositionAndTarget_UpVecY(camPos, center);

        if (player)
        {
            // 筋トレルーティン：
            // 0〜200F (約3.3秒): スクワット (バーベル保持)
            // 200〜270F (約1.2秒): 待機・息抜きポーズ
            // 270〜470F (約3.3秒): 腕立て伏せ (プッシュアップ)
            // 470〜540F (約1.2秒): 待機・息抜きポーズ
            Player3D::TitleWorkoutType workout = Player3D::TitleWorkoutType::Idle;
            if (cycle < 200)
            {
                workout = Player3D::TitleWorkoutType::Squat;
            }
            else if (isPushUp)
            {
                workout = Player3D::TitleWorkoutType::PushUp;
            }
            player->UpdateTitleAnimation(workout);
        }

        // キー入力
        bool keyH     = (CheckHitKey(KEY_INPUT_H) != 0);
        bool keyTab   = (CheckHitKey(KEY_INPUT_TAB) != 0);
        bool keyEsc   = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
        bool keyUp    = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
        bool keyDown  = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
        bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0);
        bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0);
        bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        bool keyO     = (CheckHitKey(KEY_INPUT_O) != 0);

        bool triggerH     = keyH && !m_prevKeyH;
        bool triggerO     = keyO && !m_prevKeyO;
        bool triggerTab   = keyTab && !m_prevKeyTab;
        bool triggerEsc   = keyEsc && !m_prevKeyEsc;
        bool triggerUp    = keyUp && !m_prevKeyUp;
        bool triggerDown  = keyDown && !m_prevKeyDown;
        bool triggerLeft  = keyLeft && !m_prevKeyLeft;
        bool triggerRight = keyRight && !m_prevKeyRight;
        bool triggerEnter = isEnter && !m_prevKeyEnter;
        bool triggerSpace = isSpace && !m_prevKeySpace;

        m_prevKeyH     = keyH;
        m_prevKeyTab   = keyTab;
        m_prevKeyEsc   = keyEsc;
        m_prevKeyUp    = keyUp;
        m_prevKeyDown  = keyDown;
        m_prevKeyLeft  = keyLeft;
        m_prevKeyRight = keyRight;
        m_prevKeyEnter = isEnter;
        m_prevKeySpace = isSpace;
        m_prevKeyO     = keyO;

        // マウス入力
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
        bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
        m_prevMouseLeft = isLeftClick;

        // スタート演出（ホワイトトランジション）中
        if (m_startTransitionTimer > 0)
        {
            m_startTransitionTimer--;
            if (m_startTransitionTimer <= 0)
            {
                StartGame();
            }
            return;
        }

        // サウンド設定ダイアログ表示中
        if (m_showSoundSettings)
        {
            UpdateTitleSoundSettings();
            return;
        }

        // ゲーム説明 ダイアログ表示中の処理（点滅・即閉じ防止にトリガー判定を使用）
        if (m_showHowToPlay)
        {
            int modalW = 860;
            int modalH = 520;
            int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
            int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;
            int closeBtnX = modalX + modalW - 164;
            int closeBtnY = modalY + modalH - 52;
            int closeBtnW = 140;
            int closeBtnH = 38;

            bool clickCloseBtn = (isMouseTrigger &&
                mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                my >= closeBtnY && my <= closeBtnY + closeBtnH);

            if (triggerEsc || triggerH || triggerTab || ((triggerEnter || triggerSpace) && !isMouseTrigger) || clickCloseBtn)
            {
                m_showHowToPlay = false;
                SoundManager::GetInstance().PlaySE(SeType::Cancel);
            }
            return;
        }

        // Hキー または TABキーで遊び方を開く
        if (triggerH || triggerTab)
        {
            m_showHowToPlay = true;
            SoundManager::GetInstance().PlaySE(SeType::Decide);
            return;
        }

        // タイトル画面でESCが押されたらゲーム終了
        if (triggerEsc && !m_showHowToPlay)
        {
            m_shouldQuit = true;
            return;
        }

        // メニュー項目の上下移動 (0: START, 1: STAGE, 2: ゲーム説明, 3: サウンド設定)
        if (triggerUp)   { m_selectedMenuItem = (m_selectedMenuItem + 3) % 4; SoundManager::GetInstance().PlaySE(SeType::Cursor); }
        if (triggerDown) { m_selectedMenuItem = (m_selectedMenuItem + 1) % 4; SoundManager::GetInstance().PlaySE(SeType::Cursor); }

        // ボタンの当たり判定座標
        int btnW = 420;
        int btnH = 46;
        int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
        int startBtnY = 338;
        int stageBtnY = 390;
        int howBtnY   = 442;
        int soundBtnY = 494;

        bool hoverStart = (mx >= btnX && mx <= btnX + btnW && my >= startBtnY && my <= startBtnY + btnH);
        bool hoverStage = (mx >= btnX && mx <= btnX + btnW && my >= stageBtnY && my <= stageBtnY + btnH);
        bool hoverHow   = (mx >= btnX && mx <= btnX + btnW && my >= howBtnY && my <= howBtnY + btnH);
        bool hoverSound = (mx >= btnX && mx <= btnX + btnW && my >= soundBtnY && my <= soundBtnY + btnH);

        if (hoverStart) m_selectedMenuItem = 0;
        if (hoverStage) m_selectedMenuItem = 1;
        if (hoverHow)   m_selectedMenuItem = 2;
        if (hoverSound) m_selectedMenuItem = 3;

        // 左右キーによるマップ切り替え
        if (triggerLeft)
        {
            SwitchSelectedMap(ModelConfig::MapType::House);
        }
        if (triggerRight)
        {
            SwitchSelectedMap(ModelConfig::MapType::SlopeHills);
        }

        // STAGEボタンクリックまたはEnter/Spaceによるマップ切り替え
        if (hoverStage && isMouseTrigger)
        {
            if (mx < btnX + 70)
            {
                SwitchSelectedMap(ModelConfig::MapType::House);
            }
            else if (mx > btnX + btnW - 70)
            {
                SwitchSelectedMap(ModelConfig::MapType::SlopeHills);
            }
            else
            {
                SwitchSelectedMap((m_selectedMap == ModelConfig::MapType::House)
                    ? ModelConfig::MapType::SlopeHills
                    : ModelConfig::MapType::House);
            }
        }
        else if ((triggerEnter || triggerSpace) && m_selectedMenuItem == 1)
        {
            SwitchSelectedMap((m_selectedMap == ModelConfig::MapType::House)
                ? ModelConfig::MapType::SlopeHills
                : ModelConfig::MapType::House);
        }

        // スタート実行判定
        if ((hoverStart && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_selectedMenuItem == 0))
        {
            SoundManager::GetInstance().PlaySE(SeType::Decide);
            m_startTransitionTimer = 22; // 閃光トランジション開始
            return;
        }

        // ゲーム説明を開く判定
        if ((hoverHow && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_selectedMenuItem == 2))
        {
            m_showHowToPlay = true;
            SoundManager::GetInstance().PlaySE(SeType::Decide);
            return;
        }

        // サウンド設定を開く判定 (Oキー または メニュー3選択)
        if (triggerO || (hoverSound && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_selectedMenuItem == 3))
        {
            m_showSoundSettings = true;
            m_titleSoundSettingItem = 0;
            SoundManager::GetInstance().PlaySE(SeType::Decide);
            return;
        }
    }
    
    else if (m_state == GameState::Playing)
    {
        // ESCキーによるポーズ切替判定
        bool keyEsc = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
        bool triggerEsc = keyEsc && !m_prevKeyEsc;
        m_prevKeyEsc = keyEsc;

        // ポーズ中の更新処理
        if (m_isPaused)
        {
            SetMouseDispFlag(TRUE); // ポーズ中はカーソル表示
            UpdatePause(triggerEsc);
            return;
        }

        // プレイ中にESCが押されたらポーズ開始
        if (triggerEsc)
        {
            SoundManager::GetInstance().PlaySE(SeType::Pause);
            SoundManager::GetInstance().PauseBGM();
            m_isPaused = true;
            m_pauseStartTime = GetNowCount();
            m_pauseMenuItem = 0;
            SetMouseDispFlag(TRUE);
            return;
        }

        SetMouseDispFlag(FALSE); // ゲームプレイ中はカーソル非表示（マウス視点移動）

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            bool isFrontView = player->IsSkillChecking();
            const auto& stage = m_objManager.GetStage();
            m_camera.Update(player->GetPos(), player->GetRotY(), true, isFrontView, stage.GetModelHandle(), &stage.GetMinBounds(), &stage.GetMaxBounds());
        }

        m_camera.Apply();

        // マウス感度クイック調整（[-] / [[] でダウン、[+] / []] / [;] でアップ）
        bool keyMinus = (CheckHitKey(KEY_INPUT_MINUS) != 0 || CheckHitKey(KEY_INPUT_SUBTRACT) != 0 || CheckHitKey(KEY_INPUT_LBRACKET) != 0);
        bool keyPlus  = (CheckHitKey(KEY_INPUT_ADD) != 0 || CheckHitKey(KEY_INPUT_RBRACKET) != 0 || CheckHitKey(KEY_INPUT_SEMICOLON) != 0 || CheckHitKey(KEY_INPUT_COLON) != 0);

        if (keyMinus && !m_prevKeyMinus)
        {
            m_camera.AdjustMouseSensitivity(-0.1f);
            m_sensitivityNoticeTimer = 90; // 約1.5秒通知表示
        }
        if (keyPlus && !m_prevKeyPlus)
        {
            m_camera.AdjustMouseSensitivity(+0.1f);
            m_sensitivityNoticeTimer = 90;
        }
        m_prevKeyMinus = keyMinus;
        m_prevKeyPlus  = keyPlus;

        if (m_sensitivityNoticeTimer > 0)
        {
            m_sensitivityNoticeTimer--;
        }

        // デバッグチート入力処理（F1でON/OFF切り替え）
        bool keyF1 = (CheckHitKey(KEY_INPUT_F1) != 0);

        if (keyF1 && !m_prevKeyF1)
        {
            m_cheatEnabled = !m_cheatEnabled;
        }

        m_prevKeyF1 = keyF1;

        if (m_cheatEnabled && player)
        {
            // [1] 通常ネズミを1体追加
            bool key1 = (CheckHitKey(KEY_INPUT_1) != 0);

            if (key1 && !m_prevKey1)
            {
                m_objManager.SpawnNormalMouse();
            }

            m_prevKey1 = key1;

            // [2] 高速ネズミを1体追加
            bool key2 = (CheckHitKey(KEY_INPUT_2) != 0);

            if (key2 && !m_prevKey2)
            {
                m_objManager.SpawnFastMouse();
            }

            m_prevKey2 = key2;

            // [3] レベル（Rep）を+1
            bool key3 = (CheckHitKey(KEY_INPUT_3) != 0);

            if (key3 && !m_prevKey3)
            {
                player->AddRep(1);
            }

            m_prevKey3 = key3;

            // [4] 時間停止ON/OFF
            bool key4 = (CheckHitKey(KEY_INPUT_4) != 0);

            if (key4 && !m_prevKey4)
            {
                m_objManager.ToggleTimeFrozen();
            }

            m_prevKey4 = key4;

            // [5] クールタイム無効ON/OFF
            bool key5 = (CheckHitKey(KEY_INPUT_5) != 0);

            if (key5 && !m_prevKey5)
            {
                player->ToggleNoCooldown();
            }

            m_prevKey5 = key5;

            // [6] エフェクト表示ON/OFF
            bool key6 = (CheckHitKey(KEY_INPUT_6) != 0);

            if (key6 && !m_prevKey6)
            {
                EffectManager::GetInstance().ToggleEffectEnabled();
                CustomEffectManager::GetInstance().SetEffectEnabled(EffectManager::GetInstance().IsEffectEnabled());
            }

            m_prevKey6 = key6;
        }

        // [7] 10m×10m×10m目測キューブ表示モード切替 / [8] 10m全体目測用・広角カメラ切替
        if (m_cheatEnabled)
        {
            bool key7 = (CheckHitKey(KEY_INPUT_7) != 0);
            if (key7 && !m_prevKey7)
            {
                m_debugCubeMode = (m_debugCubeMode + 1) % 4;
            }
            m_prevKey7 = key7;

            bool key8 = (CheckHitKey(KEY_INPUT_8) != 0);
            if (key8 && !m_prevKey8)
            {
                m_debugWideCamera = !m_debugWideCamera;
            }
            m_prevKey8 = key8;
        }

        m_objManager.Update(m_camera);

        // F1デバッグ広角カメラがONの場合は、10m×10m×10mの■全体と猫の1.8m身長が一目で比較できる俯瞰視点にセット
        if (m_cheatEnabled && m_debugWideCamera && player)
        {
            VECTOR pPos = player->GetPos();
            float yaw = m_camera.GetAngleH();
            float dist = 780.0f; // 約13.4m後方から10m×10m×10mキューブ全体を収める
            float height = 520.0f; // 約9.0m高さから見下ろす
            VECTOR camPos = VGet(pPos.x - std::sin(yaw) * dist, pPos.y + height, pPos.z - std::cos(yaw) * dist);
            VECTOR target = VGet(pPos.x, pPos.y + 180.0f, pPos.z);
            SetCameraNearFar(10.0f, 30000.0f);
            SetCameraPositionAndTarget_UpVecY(camPos, target);
        }

        // クリア判定・制限時間（120秒）タイムアップ判定
        float elapsedSec = (GetNowCount() - m_startCount) / 1000.0f;
        float remainSec  = Config::TIME_LIMIT_SECONDS - elapsedSec;
        if (remainSec < 0.0f) remainSec = 0.0f;

        // 残り時間10秒以下の毎秒カウントダウン警告ビープ
        int prevSec = static_cast<int>(m_remainingTimeSeconds);
        int curSec = static_cast<int>(remainSec);
        if (curSec < prevSec && curSec >= 1 && curSec <= 10)
        {
            SoundManager::GetInstance().PlaySE(SeType::Countdown);
        }

        if (m_objManager.GetRemainingMouseCount() == 0 || (Config::USE_TIME_LIMIT_MODE && remainSec <= 0.0f))
        {
            m_state = GameState::GameClear;
            m_clearCount = GetNowCount();
            m_clearTimeSeconds = elapsedSec;
            m_remainingTimeSeconds = remainSec;
            m_isTimeUp = (Config::USE_TIME_LIMIT_MODE && m_objManager.GetRemainingMouseCount() > 0 && remainSec <= 0.0f);
            if (m_isTimeUp)
            {
                SoundManager::GetInstance().PlayBGM(BgmType::TimeUp, false);
            }
            else
            {
                SoundManager::GetInstance().PlayBGM(BgmType::Clear, false);
            }
            m_clearAnimTimer = 0;
            m_clearSelectedBtn = 0;
            m_clearCamStartPos = m_camera.GetCameraPos();
            m_clearCamStartTarget = m_camera.GetTargetPos();
            m_prevKeyEnter = true;
            m_prevKeySpace = true;
            m_prevKeyR = true;
            m_prevKeyT = true;
            m_prevKeyM = true;
            m_prevMouseLeft = true;
        }
    }
    
    else if (m_state == GameState::GameClear)
    {
        SetMouseDispFlag(TRUE); // クリア画面ではカーソル表示
        m_clearAnimTimer++;

        auto player = m_objManager.GetPlayer();
        if (player)
        {
            // 背景で猫がバーベルを持って筋トレ（スクワット）し続ける演出
            player->UpdateTitleAnimation(true);

            VECTOR pPos = player->GetPos();
            float pRot = player->GetRotY();

            // 猫の斜め前方へ回り込みながらズームインし、全身（頭から足元・バーベルまで）が綺麗に収まる距離に引く
            int orbitFrames = (m_clearAnimTimer > CLEAR_ZOOM_DURATION) ? (m_clearAnimTimer - CLEAR_ZOOM_DURATION) : 0;
            float frontAngle = pRot + MathHelper::PI * 0.82f + std::sin(orbitFrames * 0.012f) * 0.22f;
            float effRep = static_cast<float>(player->GetEffectiveRep());
            float zoomDist = 235.0f + effRep * 3.0f;   // 少し引いて全身・バーベルが見切れない距離
            float zoomHeight = 68.0f + effRep * 1.2f;  // 猫の胸〜顔の高さに合わせてカメラ高さを調整

            VECTOR goalCamPos = VGet(
                pPos.x - std::sin(frontAngle) * zoomDist,
                pPos.y + zoomHeight,
                pPos.z - std::cos(frontAngle) * zoomDist
            );
            VECTOR goalTarget = VGet(pPos.x, pPos.y + 52.0f + effRep * 1.0f, pPos.z); // 猫の体の中央（胸付近）を注視

            // ステージ壁による遮蔽チェック（猫とカメラの間に壁がある場合は手前に寄せる）
            int stageModel = m_objManager.GetStage().GetModelHandle();
            if (stageModel != -1)
            {
                MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(stageModel, -1, goalTarget, goalCamPos);
                if (hit.HitFlag == 1 && std::abs(hit.Normal.y) < 0.85f)
                {
                    VECTOR dir = VSub( hit.HitPosition, goalTarget );
                    float len = VSize(dir);
                    if (len > 38.0f)
                    {
                        goalCamPos = VAdd(goalTarget, VScale(VNorm(dir), len - 10.0f));
                    }
                }
            }

            // イーズアウト補間でクリア直後のカメラ位置から猫へ滑らかにズームイン
            float t = MathHelper::Clamp(static_cast<float>(m_clearAnimTimer) / static_cast<float>(CLEAR_ZOOM_DURATION), 0.0f, 1.0f);
            float smoothT = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

            VECTOR curCamPos = VGet(
                MathHelper::Lerp(m_clearCamStartPos.x, goalCamPos.x, smoothT),
                MathHelper::Lerp(m_clearCamStartPos.y, goalCamPos.y, smoothT),
                MathHelper::Lerp(m_clearCamStartPos.z, goalCamPos.z, smoothT)
            );
            VECTOR curTarget = VGet(
                MathHelper::Lerp(m_clearCamStartTarget.x, goalTarget.x, smoothT),
                MathHelper::Lerp(m_clearCamStartTarget.y, goalTarget.y, smoothT),
                MathHelper::Lerp(m_clearCamStartTarget.z, goalTarget.z, smoothT)
            );

            SetCameraNearFar(5.0f, 90000.0f);
            SetCameraPositionAndTarget_UpVecY(curCamPos, curTarget);

            // 筋トレ燃焼オーラエフェクトも継続更新
            CustomEffectManager::GetInstance().UpdateWorkoutFlame(pPos, true, player->GetRepCount(), 0.7f, true);
            EffectManager::GetInstance().UpdateMuscleAura(pPos, true, player->GetRepCount());
        }

        // マウス入力
        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
        bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
        m_prevMouseLeft = isLeftClick;

        // キー入力
        bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
        bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0 || CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
        bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
        bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
        bool keyR     = (CheckHitKey(KEY_INPUT_R) != 0);
        bool keyT     = (CheckHitKey(KEY_INPUT_T) != 0);
        bool keyM     = (CheckHitKey(KEY_INPUT_M) != 0);

        bool triggerLeft  = keyLeft && !m_prevKeyLeft;
        bool triggerRight = keyRight && !m_prevKeyRight;
        bool triggerEnter = isEnter && !m_prevKeyEnter;
        bool triggerSpace = isSpace && !m_prevKeySpace;
        bool triggerR     = keyR && !m_prevKeyR;
        bool triggerT     = keyT && !m_prevKeyT;
        bool triggerM     = keyM && !m_prevKeyM;

        m_prevKeyLeft  = keyLeft;
        m_prevKeyRight = keyRight;
        m_prevKeyEnter = isEnter;
        m_prevKeySpace = isSpace;
        m_prevKeyR     = keyR;
        m_prevKeyT     = keyT;
        m_prevKeyM     = keyM;

        // ズームイン演出完了の瞬間にスタンプ押印音を再生！
        if (m_clearAnimTimer == CLEAR_ZOOM_DURATION)
        {
            SoundManager::GetInstance().PlaySE(SeType::Stamp);
        }

        // ズームイン演出中（CLEAR_ZOOM_DURATION フレームまで）はリザルトUIのボタン操作を待機（エフェクト更新は止めない）
        if (m_clearAnimTimer > CLEAR_ZOOM_DURATION)
        {
            if (triggerLeft)  { m_clearSelectedBtn = 0; SoundManager::GetInstance().PlaySE(SeType::Cursor); }
            if (triggerRight) { m_clearSelectedBtn = 1; SoundManager::GetInstance().PlaySE(SeType::Cursor); }

            // アクションボタン当たり判定座標
            int btnW = 340;
            int btnH = 66;
            int btnGap = 50;
            int totalBtnsW = btnW * 2 + btnGap;
            int btn1X = (Config::SCREEN_WIDTH - totalBtnsW) / 2;
            int btn2X = btn1X + btnW + btnGap;
            int btnY  = 618;

            bool hoverRetry = (mx >= btn1X && mx <= btn1X + btnW && my >= btnY && my <= btnY + btnH);
            bool hoverMenu  = (mx >= btn2X && mx <= btn2X + btnW && my >= btnY && my <= btnY + btnH);

            if (hoverRetry) m_clearSelectedBtn = 0;
            if (hoverMenu)  m_clearSelectedBtn = 1;

            // リトライ判定（[R]キー、または RETRY ボタンクリック / 選択決定）
            if (triggerR || (hoverRetry && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_clearSelectedBtn == 0))
            {
                SoundManager::GetInstance().PlaySE(SeType::Decide);
                StartGame();
                return;
            }

            // メインメニュー（タイトル）判定（[M]/[T]キー、または MAIN MENU ボタンクリック / 選択決定）
            if (triggerM || triggerT || (hoverMenu && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_clearSelectedBtn == 1))
            {
                SoundManager::GetInstance().PlaySE(SeType::Decide);
                SetupTitle();
                return;
            }
        }
    }

    // 3Dエフェクトの更新（タイトル画面・ポーズ中は更新しない）
    if (m_state != GameState::Title && !m_isPaused)
    {
        EffectManager::GetInstance().Update();
        CustomEffectManager::GetInstance().Update();
    }
}

void GameManager::Draw()
{
    // 1. 3Dシーンの描画 (Zバッファ有効)
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    m_objManager.Draw3D();

    // タイトル画面専用の超ド派手3D演出（ワークアウトステージリング・金粉パーティクル・スポットライト・マッスルオーラ）
    if (m_state == GameState::Title)
    {
        DrawTitle3DScene();
    }

    // F1デバッグモード時の3Dオーバーレイ描画（10m×10m×10m目測キューブ・1.8m身長バー・スポーン位置・当たり判定リング）
    if (m_state == GameState::Playing && m_cheatEnabled)
    {
        DrawDebug3DOverlay();
    }

    // Effekseer 3Dエフェクトおよびカスタム燃焼エフェクト描画（タイトル画面では非表示）
    if (m_state != GameState::Title)
    {
        EffectManager::GetInstance().SyncCamera();
        EffectManager::GetInstance().Draw3D();
        CustomEffectManager::GetInstance().Draw3D();
    }

    // 2. 2D HUD / UI描画 (Zバッファ無効)
    SetUseZBuffer3D(FALSE);
    SetWriteZBuffer3D(FALSE);

    if (m_state == GameState::Playing)
    {
        m_objManager.Draw2D();
    }

    const auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font36 = fm.GetFont36();
    int font48 = fm.GetFont48();

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int yellow = GetColor(255, 240, 60);
    unsigned int gray   = GetColor(180, 180, 180);
    unsigned int cyan   = GetColor(100, 220, 255);

    // FPS & 処理時間計測（0.5秒ごとに更新）
    m_frameCount++;
    int now = GetNowCount();
    if (now - m_fpsTimer >= 500)
    {
        m_currentFps = (m_frameCount * 1000.0f) / static_cast<float>(now - m_fpsTimer);
        m_frameCount = 0;
        m_fpsTimer = now;
    }

    DrawFormatStringToHandle(Config::SCREEN_WIDTH - 170, 14, GetColor(140, 240, 140), font13, "FPS: %.1f (%.1fms)", m_currentFps, m_frameProcessTimeMs);

    if (m_state == GameState::Title)
    {
        DrawTitleScreen();
    }
    
    else if (m_state == GameState::Playing)
    {
        // ゲームプレイHUD（バカゲー風ポップ装飾）
        float currentSec = (m_isPaused ? (m_pauseStartTime - m_startCount) : (GetNowCount() - m_startCount)) / 1000.0f;

        // =========================================================================
        // 1. 左上：プレイヤーステータス ＆ 特大レベルバッジ（ふわっと浮遊する角丸カード）
        // =========================================================================
        float hudFloatY = std::sin(GetNowCount() * 0.0022f) * 3.0f;
        int pX = 14;
        int pY = 14 + static_cast<int>(hudFloatY);
        int pW = 380;
        int pH = 84;

        // 白いステージ床でも優しく浮き立つふわっとした角丸プレート＆ソフトシャドウ
        SoftUI::DrawSoftCard(pX, pY, pW, pH, 18, GetColor(20, 25, 40), 220, GetColor(255, 140, 50), 2);

        auto player = m_objManager.GetPlayer();
        int reps = player ? player->GetRepCount() : 0;
        MuscleState ms = player ? player->GetMuscleState() : MuscleState::Normal;
        bool isGod = (reps >= Player3D::MAX_EFFECTIVE_REP);

        // --- 左側：レベルバッジ（角丸カプセル・Lv.数字を特大表示） ---
        int badgeX = pX + 8;
        int badgeY = pY + 8;
        int badgeW = 96;
        int badgeH = 68;

        unsigned int badgeBg = isGod ? GetColor(70, 22, 28) : GetColor(34, 44, 68);
        unsigned int badgeBorder = isGod ? GetColor(255, 70, 50) : GetColor(255, 150, 40);
        SoftUI::DrawSoftCard(badgeX, badgeY, badgeW, badgeH, 12, badgeBg, 240, badgeBorder, 1);

        DrawStringToHandle(badgeX + 18, badgeY + 4, "LEVEL", GetColor(190, 205, 235), font13);

        char lvBuf[16];
        snprintf(lvBuf, sizeof(lvBuf), "Lv.%d", reps);
        unsigned int lvColor = isGod ? GetColor(255, 70, 50) : (reps >= 4 ? GetColor(255, 150, 40) : white);
        DrawStringToHandle(badgeX + 12, badgeY + 18, lvBuf, lvColor, font24);

        // 筋肉減衰ゲージバー（残り15秒の維持時間を角丸バー表示）
        if (player && reps > 0 && ms != MuscleState::Soreness)
        {
            float decaySec = player->GetPumpDecayRemainingSeconds();
            float gaugeRatio = decaySec / 15.0f;
            if (gaugeRatio > 1.0f) gaugeRatio = 1.0f;
            if (gaugeRatio < 0.0f) gaugeRatio = 0.0f;

            int gBarW = badgeW - 14;
            int gBarH = 6;
            int gBarX = badgeX + 7;
            int gBarY = badgeY + badgeH - 12;

            DrawRoundRect(gBarX, gBarY, gBarX + gBarW, gBarY + gBarH, 3, 3, GetColor(10, 10, 15), TRUE);
            unsigned int gColor = (decaySec <= 4.0f && (GetNowCount() / 200) % 2 == 0) ? GetColor(255, 50, 50) : GetColor(255, 140, 40);
            int fillW = static_cast<int>(gBarW * gaugeRatio);
            if (fillW > 0)
            {
                int rVal = (fillW > 4) ? 3 : 1;
                DrawRoundRect(gBarX, gBarY, gBarX + fillW, gBarY + gBarH, rVal, rVal, gColor, TRUE);
            }
        }
        else if (ms == MuscleState::Soreness)
        {
            DrawStringToHandle(badgeX + 12, badgeY + badgeH - 16, "筋肉痛!", GetColor(100, 180, 255), font13);
        }

        // --- 右側：ミッション＆タイム＆プレイヤースピード情報 ---
        int infoX = badgeX + badgeW + 14;

        int remainMice = m_objManager.GetRemainingMouseCount();
        DrawStringToHandle(infoX, pY + 8, "ネズミ:", GetColor(200, 215, 240), font16);
        char miceBuf[32];
        snprintf(miceBuf, sizeof(miceBuf), "%d 匹", remainMice);
        // ふわっとした角丸ネズミ数タグバッジ
        SoftUI::DrawSoftCard(infoX + 58, pY + 4, 64, 24, 10, GetColor(230, 60, 75), 240, GetColor(255, 180, 190), 1);
        int mTxtW = GetDrawStringWidthToHandle(miceBuf, static_cast<int>(strlen(miceBuf)), font16);
        DrawStringToHandle(infoX + 58 + (64 - mTxtW) / 2, pY + 7, miceBuf, white, font16);

        char timeBuf[64];
        if (Config::USE_TIME_LIMIT_MODE)
        {
            float remainSec = Config::TIME_LIMIT_SECONDS - currentSec;
            if (remainSec < 0.0f) remainSec = 0.0f;
            snprintf(timeBuf, sizeof(timeBuf), "残り時間: %.1f 秒", remainSec);
            unsigned int tCol = (remainSec <= 20.0f && (GetNowCount() / 200) % 2 == 0)
                ? GetColor(255, 70, 70)
                : GetColor(255, 235, 60);
            DrawStringToHandle(infoX, pY + 32, timeBuf, tCol, font16);
        }
        else
        {
            snprintf(timeBuf, sizeof(timeBuf), "タイム: %.1f 秒", currentSec);
            DrawStringToHandle(infoX, pY + 32, timeBuf, white, font16);
        }

        if (player)
        {
            float catSpeed = player->GetCurrentSpeed();
            float actualSpd = player->GetActualMoveSpeed();
            float mPerSec = (catSpeed * 60.0f) / Config::WORLD_UNITS_PER_METER;
            char spdBuf[96];
            snprintf(spdBuf, sizeof(spdBuf), "現在速度: %.2f (実移動:%.2f / %.1fm/s)", catSpeed, actualSpd, mPerSec);
            DrawStringToHandle(infoX, pY + 58, spdBuf, GetColor(100, 245, 140), font13);
        }

        // =========================================================================
        // 2. 画面右下：スマート操作ガイドカード（ふわっと浮遊する角丸カード）
        // =========================================================================
        float guideFloatY = std::sin(GetNowCount() * 0.0020f + 1.2f) * 2.5f;
        int gW = 640;
        int gH = 72;
        int gX = Config::SCREEN_WIDTH - gW - 20;
        int gY = Config::SCREEN_HEIGHT - gH - 20 + static_cast<int>(guideFloatY);

        // ふわっとした角丸プレート＆ソフトシャドウ
        SoftUI::DrawSoftCard(gX, gY, gW, gH, 16, GetColor(18, 24, 38), 220, GetColor(100, 130, 180), 1);

        // 上段：アクションスキル（左クリック: 飛びつき、右クリック: スタン攻撃、SPACE: 筋トレ）
        if (reps >= 4)
        {
            float pounceCd = player ? static_cast<float>(player->GetPounceCooldown()) / 60.0f : 0.0f;
            float tackleCd = player ? static_cast<float>(player->GetTackleCooldown()) / 60.0f : 0.0f;

            if (pounceCd > 0.05f)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "[左] 飛びつき(%.1fs)", pounceCd);
                DrawStringToHandle(gX + 14, gY + 10, buf, GetColor(160, 170, 185), font16);
            }
            else
            {
                DrawStringToHandle(gX + 14, gY + 10, "[左クリック] 飛びつき", GetColor(255, 90, 90), font16);
            }

            if (tackleCd > 0.05f)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "[右] スタン(%.1fs)", tackleCd);
                DrawStringToHandle(gX + 200, gY + 10, buf, GetColor(160, 170, 185), font16);
            }
            else
            {
                DrawStringToHandle(gX + 200, gY + 10, "[右クリック] スタン攻撃", GetColor(255, 150, 40), font16);
            }
        }
        else
        {
            DrawStringToHandle(gX + 14, gY + 10, "[左/右クリック] 攻撃スキル (Lv.4で解放)", GetColor(140, 155, 180), font16);
        }
        DrawStringToHandle(gX + 400, gY + 10, "[SPACE] 筋トレ(長押し)", GetColor(100, 245, 140), font16);

        // 下段：基本移動・カメラ
        char camInfo[128];
        snprintf(camInfo, sizeof(camInfo), "移動: WASD  |  視点: マウス(感度:%.1fx) / 矢印  |  感度変更: [- / +]  |  視点リセット: [F]", m_camera.GetMouseSensitivity());
        DrawStringToHandle(gX + 14, gY + 42, camInfo, GetColor(210, 225, 245), font13);

        // =========================================================================
        // 3. F1 デバッグ情報＆ミニマップ＆10m距離目測オーバーレイ
        // =========================================================================
        if (m_cheatEnabled)
        {
            DrawDebug2DOverlay();
        }
        else
        {
            // F1 OFF時は右上に案内バッジを表示（ふわっとした角丸バッジ）
            int f1W = 265;
            int f1H = 26;
            int f1X = Config::SCREEN_WIDTH - f1W - 12;
            int f1Y = 32;
            SoftUI::DrawSoftCard(f1X, f1Y, f1W, f1H, 10, GetColor(18, 24, 40), 200, GetColor(90, 130, 190), 1);
            DrawStringToHandle(f1X + 8, f1Y + 4, "[F1] デバッグ情報・マップ・10m目測表示", GetColor(170, 220, 255), font13);
        }

        // プレイ中の感度変更通知ポップアップ（ふわっと浮遊する角丸カード）
        if (m_sensitivityNoticeTimer > 0 && !m_isPaused)
        {
            float alphaRatio = (m_sensitivityNoticeTimer < 20) ? (m_sensitivityNoticeTimer / 20.0f) : 1.0f;
            int alpha = static_cast<int>(230 * alphaRatio);

            int notW = 310;
            int notH = 48;
            int notX = (Config::SCREEN_WIDTH - notW) / 2;
            int notY = 24 + static_cast<int>(std::sin(GetNowCount() * 0.005f) * 3.0f);

            SoftUI::DrawSoftCard(notX, notY, notW, notH, 16, GetColor(20, 26, 42), alpha, GetColor(100, 220, 255), 2, true, GetColor(80, 200, 255));

            char notBuf[64];
            snprintf(notBuf, sizeof(notBuf), "🎯 マウス感度: %.1fx", m_camera.GetMouseSensitivity());
            int nW = GetDrawStringWidthToHandle(notBuf, static_cast<int>(strlen(notBuf)), font18);
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
            DrawStringToHandle((Config::SCREEN_WIDTH - nW) / 2, notY + 13, notBuf, GetColor(255, 235, 80), font18);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ポーズ画面描画
        if (m_isPaused)
        {
            DrawPauseModal();
        }
    }

    else if (m_state == GameState::GameClear)
    {
        DrawGameClearScreen();
    }

    if (CheckHitKey(KEY_INPUT_F12))
    {
        SaveDrawScreenToPNG(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, "d:\\MuscleCat\\screen_shot.png");
    }
}

// ============================================================================
// ステージクリア（リザルト）画面の描画（アメコミ・カートゥーン風ポップUIデザイン）
// ============================================================================
void GameManager::DrawGameClearScreen()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font36 = fm.GetFont36();
    int font48 = fm.GetFont48();
    int font60 = fm.GetFont60();
    int font96 = fm.GetFont96();

    // カラーパレット定義（イエロー、オレンジ、ブラック、ホワイト＋ゴールド＆ネオンレッド）
    unsigned int colYellow  = GetColor(255, 235, 0);   // #FFEB00
    unsigned int colOrange  = GetColor(255, 140, 0);   // #FF8C00
    unsigned int colBlack   = GetColor(0, 0, 0);       // #000000
    unsigned int colWhite   = GetColor(255, 255, 255); // #FFFFFF
    unsigned int colGold    = GetColor(255, 215, 0);   // #FFD700
    unsigned int colNeonRed = GetColor(255, 0, 68);    // #FF0044

    float animSec = m_clearAnimTimer / 60.0f;

    // =========================================================================
    // ズームイン演出中（開始〜CLEAR_ZOOM_DURATION フレーム）：
    // まず猫が筋トレ（スクワット）している姿へカメラがズームし、その後リザルトUIを出す
    // =========================================================================
    if (m_clearAnimTimer <= CLEAR_ZOOM_DURATION)
    {
        // 上下のシネマティック黒帯＆「★ STAGE CLEAR!! ★」ロゴだけ先行表示して猫の筋トレを強調
        float barRatio = MathHelper::Clamp(m_clearAnimTimer / 18.0f, 0.0f, 1.0f);
        int barH = static_cast<int>(90 * barRatio);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 190);
        DrawBox(0, 0, Config::SCREEN_WIDTH, barH, colBlack, TRUE);
        DrawBox(0, Config::SCREEN_HEIGHT - barH, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colBlack, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawBox(0, 0, Config::SCREEN_WIDTH, 8, colYellow, TRUE);
        DrawBox(0, Config::SCREEN_HEIGHT - 8, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colYellow, TRUE);

        float headerBounce = std::abs(std::sin(animSec * 5.0f)) * -8.0f;
        int headerY = 20 + static_cast<int>(headerBounce);
        const char* clearLogo = m_isTimeUp ? "★ TIME UP!! ★" : "★ STAGE CLEAR!! ★";
        int logoW = GetDrawStringWidthToHandle(clearLogo, static_cast<int>(strlen(clearLogo)), font60);
        int logoX = (Config::SCREEN_WIDTH - logoW) / 2;

        for (int d = 4; d <= 8; ++d)
        {
            DrawStringToHandle(logoX + d, headerY + d, clearLogo, colBlack, font60);
        }
        for (int dy = -4; dy <= 4; ++dy)
        {
            for (int dx = -4; dx <= 4; ++dx)
            {
                if (dx != 0 || dy != 0)
                {
                    DrawStringToHandle(logoX + dx, headerY + dy, clearLogo, colBlack, font60);
                }
            }
        }
        DrawStringToHandle(logoX, headerY + 2, clearLogo, m_isTimeUp ? colNeonRed : colOrange, font60);
        DrawStringToHandle(logoX, headerY,     clearLogo, colYellow, font60);
        DrawStringToHandle(logoX, headerY - 2, clearLogo, GetColor(255, 255, 190), font60);
        return;
    }

    int uiTimer = m_clearAnimTimer - CLEAR_ZOOM_DURATION;
    float uiSec = uiTimer / 60.0f;

    // =========================================================================
    // 0. 背景オーバーレイ（背後の筋トレ中の猫がよく見える軽めの半透明＋ドット＋集中線）
    // =========================================================================
    // ベース半透明ダークレイヤー（背景の筋トレ中の猫がくっきり透けて見えるよう105に調整）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 105);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(10, 12, 22), TRUE);

    // 上下端をさらに引き締めるビネット風グラデーション帯
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
    DrawBox(0, 0, Config::SCREEN_WIDTH, 135, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 120, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colBlack, TRUE);

    // アメコミ風ハーフトーン・ドットパターン（四隅・上下エリアにドット網点を配置）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 38);
    int dotStep = 28;
    for (int y = dotStep / 2; y < Config::SCREEN_HEIGHT; y += dotStep)
    {
        int rowOffset = ((y / dotStep) % 2) * (dotStep / 2);
        for (int x = rowOffset; x < Config::SCREEN_WIDTH; x += dotStep)
        {
            float dx = static_cast<float>(x - Config::SCREEN_WIDTH / 2) / (Config::SCREEN_WIDTH * 0.5f);
            float dy = static_cast<float>(y - Config::SCREEN_HEIGHT / 2) / (Config::SCREEN_HEIGHT * 0.5f);
            float distSq = dx * dx + dy * dy;
            if (distSq > 0.32f)
            {
                int radius = static_cast<int>(MathHelper::Clamp((distSq - 0.25f) * 6.5f, 2.0f, 6.0f));
                DrawCircle(x, y, radius, colOrange, TRUE);
            }
        }
    }

    // 漫画風の放射集中線エフェクト（画面中央から外周へ向かうダイナミックなスピードライン）
    int centerX = Config::SCREEN_WIDTH / 2;
    int centerY = Config::SCREEN_HEIGHT / 2 - 20;
    float rotBase = animSec * 0.18f;
    int numSpeedLines = 28;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
    for (int i = 0; i < numSpeedLines; ++i)
    {
        float baseAngle = rotBase + (MathHelper::PI * 2.0f * i) / numSpeedLines;
        float halfWidth = 0.018f + (i % 3) * 0.008f;
        float innerR = 340.0f + ((i * 37) % 110) + std::sin(animSec * 6.0f + i) * 20.0f;
        float outerR = 920.0f;

        int x1 = centerX + static_cast<int>(std::cos(baseAngle) * innerR);
        int y1 = centerY + static_cast<int>(std::sin(baseAngle) * innerR);
        int x2 = centerX + static_cast<int>(std::cos(baseAngle - halfWidth) * outerR);
        int y2 = centerY + static_cast<int>(std::sin(baseAngle - halfWidth) * outerR);
        int x3 = centerX + static_cast<int>(std::cos(baseAngle + halfWidth) * outerR);
        int y3 = centerY + static_cast<int>(std::sin(baseAngle + halfWidth) * outerR);

        unsigned int rayCol = (i % 2 == 0) ? colYellow : colWhite;
        DrawTriangle(x1, y1, x2, y2, x3, y3, rayCol, TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // アメコミ風の上下イエロー＆ブラックストライプフレーム
    DrawBox(0, 0, Config::SCREEN_WIDTH, 10, colYellow, TRUE);
    DrawBox(0, 10, Config::SCREEN_WIDTH, 14, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 14, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 10, colBlack, TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 10, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, colYellow, TRUE);

    // =========================================================================
    // 1. ヘッダー（上部）：「★ STAGE CLEAR!! ★」立体ロゴ ＆ 大小の星エフェクト
    // =========================================================================
    float headerBounce = std::abs(std::sin(animSec * 4.5f)) * -8.0f;
    int headerY = 28 + static_cast<int>(headerBounce);

    // 背面の斜めスピードバナー帯（アメコミ風ブラック＆ネオンレッド枠）
    int bannerLeft  = 210;
    int bannerRight = Config::SCREEN_WIDTH - 210;
    int bannerTop   = 22;
    int bannerBot   = 112;
    DrawTriangle(bannerLeft - 25, bannerBot + 6, bannerLeft + 15, bannerTop + 6, bannerRight + 25, bannerTop + 6, colBlack, TRUE);
    DrawTriangle(bannerLeft - 25, bannerBot + 6, bannerRight + 25, bannerTop + 6, bannerRight - 15, bannerBot + 6, colBlack, TRUE);

    DrawTriangle(bannerLeft - 20, bannerBot, bannerLeft + 20, bannerTop, bannerRight + 20, bannerTop, colNeonRed, TRUE);
    DrawTriangle(bannerLeft - 20, bannerBot, bannerRight + 20, bannerTop, bannerRight - 20, bannerBot, colNeonRed, TRUE);

    DrawTriangle(bannerLeft - 12, bannerBot - 5, bannerLeft + 24, bannerTop + 5, bannerRight + 12, bannerTop + 5, GetColor(22, 24, 38), TRUE);
    DrawTriangle(bannerLeft - 12, bannerBot - 5, bannerRight + 12, bannerTop + 5, bannerRight - 24, bannerBot - 5, GetColor(22, 24, 38), TRUE);

    const char* clearLogo = m_isTimeUp ? "★ TIME UP!! ★" : "★ STAGE CLEAR!! ★";
    int logoW = GetDrawStringWidthToHandle(clearLogo, static_cast<int>(strlen(clearLogo)), font60);
    int logoX = (Config::SCREEN_WIDTH - logoW) / 2;

    // 3D押し出しシャドウ（右下ブラック）
    for (int d = 4; d <= 8; ++d)
    {
        DrawStringToHandle(logoX + d, headerY + d, clearLogo, colBlack, font60);
    }
    // 極太ブラックアウトライン
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(logoX + dx, headerY + dy, clearLogo, colBlack, font60);
            }
        }
    }
    // 内側オレンジ立体層 → メインイエロー → ゴールド＆ホワイトハイライト
    DrawStringToHandle(logoX, headerY + 2, clearLogo, m_isTimeUp ? colNeonRed : colOrange, font60);
    DrawStringToHandle(logoX, headerY,     clearLogo, colYellow, font60);
    DrawStringToHandle(logoX, headerY - 2, clearLogo, GetColor(255, 255, 190), font60);

    // 大小の星エフェクト（パルスしながら周囲に輝く）
    int starPulse1 = static_cast<int>(std::sin(animSec * 6.0f) * 6.0f);
    int starPulse2 = static_cast<int>(std::cos(animSec * 6.0f) * 6.0f);
    DrawStringToHandle(bannerLeft - 68, 30 + starPulse1, "★", colGold, font36);
    DrawStringToHandle(bannerLeft - 28, 72 + starPulse2, "✦", colWhite, font24);
    DrawStringToHandle(bannerLeft - 92, 68 - starPulse1, "★", colOrange, font18);

    DrawStringToHandle(bannerRight + 32, 28 + starPulse2, "★", colGold, font36);
    DrawStringToHandle(bannerRight + 10, 74 + starPulse1, "✦", colWhite, font24);
    DrawStringToHandle(bannerRight + 74, 66 - starPulse2, "★", colOrange, font18);

    // =========================================================================
    // 評価ランク・スコアの算出（制限時間モード：残り時間に応じたランク＆スコア算出）
    // =========================================================================
    bool isStage2 = (m_selectedMap == ModelConfig::MapType::NewMap);
    const char* rankLetter = "C";
    const char* rankTitle  = "ROOKIE CAT";
    unsigned int rankMainCol = GetColor(100, 215, 255);
    unsigned int rankSubCol  = GetColor(30, 130, 200);
    char critBuf[96];

    if (Config::USE_TIME_LIMIT_MODE)
    {
        // 120秒制限時間制：残り時間基準（S: 残り90秒以上[Stage2は60秒] / A: 残り75秒以上[45秒] / B: 残り60秒以上[30秒]）
        float sRemain = isStage2 ? 60.0f : 90.0f;
        float aRemain = isStage2 ? 45.0f : 75.0f;
        float bRemain = isStage2 ? 30.0f : 60.0f;

        if (m_isTimeUp)
        {
            rankLetter  = "C";
            rankTitle   = "TIME UP...";
            rankMainCol = GetColor(160, 175, 200);
            rankSubCol  = GetColor(80, 90, 115);
        }
        else if (m_remainingTimeSeconds >= sRemain)
        {
            rankLetter  = "S";
            rankTitle   = "GOD MUSCLE CAT!!";
            rankMainCol = colGold;
            rankSubCol  = colOrange;
        }
        else if (m_remainingTimeSeconds >= aRemain)
        {
            rankLetter  = "A";
            rankTitle   = "GREAT MUSCLE!";
            rankMainCol = colOrange;
            rankSubCol  = colNeonRed;
        }
        else if (m_remainingTimeSeconds >= bRemain)
        {
            rankLetter  = "B";
            rankTitle   = "NICE PUMP!";
            rankMainCol = colYellow;
            rankSubCol  = GetColor(180, 140, 0);
        }

        snprintf(critBuf, sizeof(critBuf), "残り時間基準: S ≧ %.0fs / A ≧ %.0fs / B ≧ %.0fs", sRemain, aRemain, bRemain);
    }
    else
    {
        // 従来の経過タイム制（Config::USE_TIME_LIMIT_MODE = false でいつでも戻せます）
        float sLimit = isStage2 ? 60.0f : 30.0f;
        float aLimit = isStage2 ? 75.0f : 45.0f;
        float bLimit = isStage2 ? 90.0f : 60.0f;

        if (m_clearTimeSeconds <= sLimit)
        {
            rankLetter  = "S";
            rankTitle   = "GOD MUSCLE CAT!!";
            rankMainCol = colGold;
            rankSubCol  = colOrange;
        }
        else if (m_clearTimeSeconds <= aLimit)
        {
            rankLetter  = "A";
            rankTitle   = "GREAT MUSCLE!";
            rankMainCol = colOrange;
            rankSubCol  = colNeonRed;
        }
        else if (m_clearTimeSeconds <= bLimit)
        {
            rankLetter  = "B";
            rankTitle   = "NICE PUMP!";
            rankMainCol = colYellow;
            rankSubCol  = GetColor(180, 140, 0);
        }

        snprintf(critBuf, sizeof(critBuf), "S ≦ %.0fs  /  A ≦ %.0fs  /  B ≦ %.0fs", sLimit, aLimit, bLimit);
    }

    // =========================================================================
    // 2. メインコンテンツ（中央左側：ランク表示 ＆ アメコミ風爆発・閃光エフェクト）
    // =========================================================================
    int rankCX = 310;
    int rankCY = 355;

    // スタンプが「ドカン！」と叩きつけられるアニメーション（ズーム完了後に拡大→着地バウンス）
    float stampProgress = MathHelper::Clamp(uiTimer / 14.0f, 0.0f, 1.0f);
    float stampBounce = (uiTimer >= 14 && uiTimer < 26)
        ? std::sin((uiTimer - 14) * 0.5f) * 0.08f
        : 0.0f;
    float burstScale = stampProgress + stampBounce;

    // アメコミ風ギザギザ爆発・閃光エフェクト（Boom / Flash ポリゴン）
    // ※ 頂点ごとに半径が食い違って「ひげ線」が出ないよう、各頂点の半径を配列で事前確定してから隣接頂点同士を結合
    constexpr int NUM_VERTICES = 28; // 14本のトゲ（偶数=外トゲ、奇数=内谷）
    for (int layer = 0; layer < 4; ++layer)
    {
        float rOuterBase = 200.0f;
        float rInnerBase = 132.0f;
        unsigned int layerCol = colBlack;
        float angleOffset = animSec * 0.22f;

        if (layer == 0)
        {
            // ドロップシャドウ＆極太黒縁
            rOuterBase = 208.0f * burstScale;
            rInnerBase = 140.0f * burstScale;
            layerCol = colBlack;
        }
        else if (layer == 1)
        {
            // 外側：ネオンレッド〜オレンジの爆発ギザギザ
            rOuterBase = 196.0f * burstScale;
            rInnerBase = 128.0f * burstScale;
            layerCol = (rankLetter[0] == 'S') ? colNeonRed : colOrange;
        }
        else if (layer == 2)
        {
            // 中間：イエローの閃光バースト（逆回転オフセット）
            rOuterBase = 168.0f * burstScale;
            rInnerBase = 108.0f * burstScale;
            layerCol = colYellow;
            angleOffset = -animSec * 0.30f + 0.22f;
        }
        else
        {
            // 内側：ホワイトのフラッシュコア
            rOuterBase = 132.0f * burstScale;
            rInnerBase = 90.0f * burstScale;
            layerCol = colWhite;
            angleOffset = animSec * 0.15f;
        }

        int vx[NUM_VERTICES];
        int vy[NUM_VERTICES];
        int ox = (layer == 0) ? 6 : 0;
        int oy = (layer == 0) ? 6 : 0;

        for (int i = 0; i < NUM_VERTICES; ++i)
        {
            float angle = angleOffset + (MathHelper::PI * 2.0f * i) / NUM_VERTICES;
            float r = (i % 2 == 0) ? rOuterBase : rInnerBase;
            if (i % 4 == 0)
            {
                r *= 1.07f; // 1つおきに長いトゲを作る（前後の三角形で同一座標を共有するため段差線が一切出ない）
            }
            vx[i] = rankCX + ox + static_cast<int>(std::cos(angle) * r);
            vy[i] = rankCY + oy + static_cast<int>(std::sin(angle) * r);
        }

        for (int i = 0; i < NUM_VERTICES; ++i)
        {
            int next = (i + 1) % NUM_VERTICES;
            DrawTriangle(rankCX + ox, rankCY + oy, vx[i], vy[i], vx[next], vy[next], layerCol, TRUE);
        }
    }

    // スタンププレート（ふわっとした角丸スタンプ）
    int stampW = static_cast<int>(220 * burstScale);
    int stampH = static_cast<int>(210 * burstScale);
    int stampX = rankCX - stampW / 2;
    int stampY = rankCY - stampH / 2;

    SoftUI::DrawSoftCard(stampX, stampY, stampW, stampH, 20, GetColor(20, 22, 34), 245, rankMainCol, 2, true, rankMainCol);
    DrawRoundRect(stampX + 6, stampY + 6, stampX + stampW - 6, stampY + stampH - 6, 14, 14, rankMainCol, FALSE);

    // 左上：「BOOM!!」アメコミ擬音バッジ
    int boomX = stampX - 28;
    int boomY = stampY - 22;
    SoftUI::DrawSoftCard(boomX, boomY, 110, 32, 12, colNeonRed, 255, colYellow, 1, true, colYellow);
    DrawStringToHandle(boomX + 12, boomY + 5, "BOOM!!", colYellow, font24);

    // 特大ランク文字（S / A / B / C）の3D立体描画
    int rLetterW = GetDrawStringWidthToHandle(rankLetter, static_cast<int>(strlen(rankLetter)), font96);
    int rLetterX = rankCX - rLetterW / 2;
    int rLetterY = stampY + 18;

    // 3D押し出し影
    for (int d = 4; d <= 8; ++d)
    {
        DrawStringToHandle(rLetterX + d, rLetterY + d, rankLetter, colBlack, font96);
    }
    // 極太ブラック縁取り
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(rLetterX + dx, rLetterY + dy, rankLetter, colBlack, font96);
            }
        }
    }
    // 本体カラー＆ハイライト
    DrawStringToHandle(rLetterX, rLetterY + 3, rankLetter, rankSubCol, font96);
    DrawStringToHandle(rLetterX, rLetterY,     rankLetter, rankMainCol, font96);
    DrawStringToHandle(rLetterX, rLetterY - 2, rankLetter, colWhite, font96);

    // 「S RANK」スタンプ下部バー
    char rankLabelBuf[32];
    snprintf(rankLabelBuf, sizeof(rankLabelBuf), "%s RANK", rankLetter);
    int rLabelW = GetDrawStringWidthToHandle(rankLabelBuf, static_cast<int>(strlen(rankLabelBuf)), font36);
    int rLabelX = rankCX - rLabelW / 2;
    int rLabelY = stampY + stampH - 64;

    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(rLabelX + dx, rLabelY + dy, rankLabelBuf, colBlack, font36);
            }
        }
    }
    DrawStringToHandle(rLabelX + 3, rLabelY + 3, rankLabelBuf, colBlack, font36);
    DrawStringToHandle(rLabelX, rLabelY, rankLabelBuf, colWhite, font36);

    // ランク称号リボン（スタンプ直下）
    int titleRibbonW = 260;
    int titleRibbonH = 34;
    int titleRibbonX = rankCX - titleRibbonW / 2;
    int titleRibbonY = stampY + stampH + 18;

    DrawBox(titleRibbonX + 4, titleRibbonY + 4, titleRibbonX + titleRibbonW + 4, titleRibbonY + titleRibbonH + 4, colBlack, TRUE);
    DrawBox(titleRibbonX - 2, titleRibbonY - 2, titleRibbonX + titleRibbonW + 2, titleRibbonY + titleRibbonH + 2, colBlack, TRUE);
    DrawBox(titleRibbonX, titleRibbonY, titleRibbonX + titleRibbonW, titleRibbonY + titleRibbonH, rankMainCol, TRUE);

    int rTitleW = GetDrawStringWidthToHandle(rankTitle, static_cast<int>(strlen(rankTitle)), font18);
    DrawStringToHandle(rankCX - rTitleW / 2, titleRibbonY + 8, rankTitle, colBlack, font18);

    // ランク基準目安テキスト
    int critW = GetDrawStringWidthToHandle(critBuf, static_cast<int>(strlen(critBuf)), font13);
    DrawBox(rankCX - critW / 2 - 10, titleRibbonY + 42, rankCX + critW / 2 + 10, titleRibbonY + 62, colBlack, TRUE);
    DrawStringToHandle(rankCX - critW / 2, titleRibbonY + 46, critBuf, colYellow, font13);

    // =========================================================================
    // 3. サブコンテンツ（中央右側：ふわっとした角丸プレート枠 ＆ 報酬アイコン）
    // =========================================================================
    int panelX = 560;
    int panelY = 148;
    int panelW = 640;
    int panelH = 428;

    // ふわっとしたコミック結果プレート（角丸＋ソフトシャドウ）
    SoftUI::DrawSoftCard(panelX, panelY, panelW, panelH, 20, GetColor(22, 26, 40), 240, colYellow, 2);

    // パネル上部コミックヘッダー帯（角丸）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
    DrawRoundRect(panelX + 2, panelY + 2, panelX + panelW - 2, panelY + 44, 18, 18, colOrange, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(panelX + 16, panelY + 44, panelX + panelW - 16, panelY + 44, colBlack);
    DrawStringToHandle(panelX + 20, panelY + 10, "★ MISSION RESULT & REWARDS ★", colBlack, font24);
    DrawStringToHandle(panelX + 18, panelY + 8,  "★ MISSION RESULT & REWARDS ★", colWhite, font24);

    // プレイヤー・捕獲情報・各ボーナスポイントの取得
    auto player = m_objManager.GetPlayer();
    int achievedLv = player ? player->GetRepCount() : 0;
    int totalMice  = Config::NORMAL_MOUSE_COUNT + Config::FAST_MOUSE_COUNT;
    int caughtMice = m_objManager.GetCaughtCount();
    if (!m_isTimeUp && caughtMice < totalMice) caughtMice = totalMice; // クリア時は全捕獲
    int caughtPct  = (totalMice > 0) ? (caughtMice * 100 / totalMice) : 100;

    // 各ボーナススコアの算出（残り時間によって大きくスコアが変動）
    int dPts = achievedLv * 150 + 300;
    int cPts = caughtMice * 200;
    int sPts = Config::USE_TIME_LIMIT_MODE
        ? static_cast<int>(m_remainingTimeSeconds * 30.0f) // 残り時間1秒につき30PTSボーナス（最大3600PTS）
        : ((rankLetter[0] == 'S') ? 1500 : (rankLetter[0] == 'A' ? 1000 : (rankLetter[0] == 'B' ? 600 : 300)));
    int totalScore = dPts + cPts + sPts;

    // パネルヘッダー右側に TOTAL SCORE をインパクト表示
    char scoreHeaderBuf[64];
    snprintf(scoreHeaderBuf, sizeof(scoreHeaderBuf), "TOTAL SCORE: %d PTS", totalScore);
    int shW = GetDrawStringWidthToHandle(scoreHeaderBuf, static_cast<int>(strlen(scoreHeaderBuf)), font18);
    int shX = panelX + panelW - shW - 22;
    SoftUI::DrawSoftCard(shX - 10, panelY + 6, shW + 20, 30, 10, colBlack, 220, colYellow, 1);
    DrawStringToHandle(shX, panelY + 12, scoreHeaderBuf, colYellow, font18);

    // 表示対象タイム（制限時間モード時は残り時間、従来モード時は経過クリアタイム）を MM:SS.ms に変換
    float displaySeconds = Config::USE_TIME_LIMIT_MODE ? m_remainingTimeSeconds : m_clearTimeSeconds;
    int totalCentisec = static_cast<int>(displaySeconds * 100.0f + 0.5f);
    int minutes  = totalCentisec / 6000;
    int seconds  = (totalCentisec / 100) % 60;
    int centisec = totalCentisec % 100;

    int rowX = panelX + 24;
    int rowW = panelW - 48;

    // --- 項目1: TIME LEFT (残り時間) または CLEAR TIME: 01:24.45 ---
    int row1Y = panelY + 60;
    int row1H = 74;
    SoftUI::DrawSoftCard(rowX, row1Y, rowW, row1H, 14, GetColor(12, 14, 24), 235, colYellow, 1);
    DrawRoundRect(rowX + 3, row1Y + 3, rowX + 11, row1Y + row1H - 3, 4, 4, colYellow, TRUE); // 左アクセントバー

    if (Config::USE_TIME_LIMIT_MODE)
    {
        DrawStringToHandle(rowX + 24, row1Y + 14, "TIME LEFT", colYellow, font18);
        char subTimeBuf[64];
        snprintf(subTimeBuf, sizeof(subTimeBuf), "残り時間 (クリアタイム: %.2f秒 / 制限120秒)", m_clearTimeSeconds);
        DrawStringToHandle(rowX + 24, row1Y + 40, subTimeBuf, GetColor(180, 195, 220), font13);
    }
    else
    {
        DrawStringToHandle(rowX + 24, row1Y + 14, "CLEAR TIME", colYellow, font18);
        DrawStringToHandle(rowX + 24, row1Y + 38, "クリアタイム", GetColor(180, 195, 220), font13);
    }

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d.%02d", minutes, seconds, centisec);
    int timeW = GetDrawStringWidthToHandle(timeStr, static_cast<int>(strlen(timeStr)), font48);
    int timeX = rowX + rowW - timeW - 24;
    int timeY = row1Y + 13;
    DrawStringToHandle(timeX + 3, timeY + 3, timeStr, colBlack, font48);
    DrawStringToHandle(timeX, timeY, timeStr, colWhite, font48);

    // --- 項目2: LEVEL ACHIEVED: Lv. 9 ---
    int row2Y = row1Y + row1H + 12;
    int row2H = 62;
    SoftUI::DrawSoftCard(rowX, row2Y, rowW, row2H, 14, GetColor(12, 14, 24), 235, colOrange, 1);
    DrawRoundRect(rowX + 3, row2Y + 3, rowX + 11, row2Y + row2H - 3, 4, 4, colOrange, TRUE);

    DrawStringToHandle(rowX + 24, row2Y + 10, "LEVEL ACHIEVED", colOrange, font18);
    DrawStringToHandle(rowX + 24, row2Y + 34, "到達マッスルレベル", GetColor(180, 195, 220), font13);

    char lvStr[32];
    snprintf(lvStr, sizeof(lvStr), "Lv. %d", achievedLv);
    int lvW = GetDrawStringWidthToHandle(lvStr, static_cast<int>(strlen(lvStr)), font36);
    int lvX = rowX + rowW - lvW - 24;
    int lvY = row2Y + 13;
    DrawStringToHandle(lvX + 3, lvY + 3, lvStr, colBlack, font36);
    DrawStringToHandle(lvX, lvY, lvStr, colGold, font36);

    // --- 項目3: CATS CAUGHT: 5 / 5 (100%) ---
    int row3Y = row2Y + row2H + 12;
    int row3H = 62;
    SoftUI::DrawSoftCard(rowX, row3Y, rowW, row3H, 14, GetColor(12, 14, 24), 235, colNeonRed, 1);
    DrawRoundRect(rowX + 3, row3Y + 3, rowX + 11, row3Y + row3H - 3, 4, 4, colNeonRed, TRUE);

    DrawStringToHandle(rowX + 24, row3Y + 10, "CATS CAUGHT", GetColor(255, 110, 140), font18);
    DrawStringToHandle(rowX + 24, row3Y + 34, "捕獲コンプリート率", GetColor(180, 195, 220), font13);

    char caughtStr[48];
    snprintf(caughtStr, sizeof(caughtStr), "%d / %d (%d%%)", caughtMice, totalMice, caughtPct);
    int caughtW = GetDrawStringWidthToHandle(caughtStr, static_cast<int>(strlen(caughtStr)), font36);
    int caughtX = rowX + rowW - caughtW - 24;
    int caughtY = row3Y + 13;
    DrawStringToHandle(caughtX + 3, caughtY + 3, caughtStr, colBlack, font36);
    DrawStringToHandle(caughtX, caughtY, caughtStr, colWhite, font36);

    // --- 項目4: 獲得報酬・ボーナスポイントアイコン領域（ダンベル、猫缶、スター） ---
    int rewY = row3Y + row3H + 14;
    int rewH = 120;
    SoftUI::DrawSoftCard(rowX, rewY, rowW, rewH, 14, GetColor(28, 32, 50), 235, colGold, 1);
    DrawStringToHandle(rowX + 14, rewY + 7, "★ BONUS REWARDS (獲得報酬ボーナス)", colYellow, font16);

    int iconCardW = 178;
    int iconCardH = 78;
    int iconGap   = 18;
    int iconStartX = rowX + 12;
    int iconCardY  = rewY + 34;

    // [アイコン1: ダンベル (POWER BONUS)]
    {
        int icX = iconStartX;
        SoftUI::DrawSoftCard(icX, iconCardY, iconCardW, iconCardH, 12, GetColor(18, 22, 34), 235, colOrange, 1);

        // ダンベルのグラフィカル描画（ベベル付きプレート＆シャフト）
        int dbCX = icX + 38;
        int dbCY = iconCardY + 38;
        DrawBox(dbCX - 22, dbCY - 4, dbCX + 22, dbCY + 4, colBlack, TRUE);
        DrawBox(dbCX - 20, dbCY - 2, dbCX + 20, dbCY + 2, GetColor(200, 210, 225), TRUE);
        // 左右ウェイトプレート
        DrawBox(dbCX - 24, dbCY - 18, dbCX - 12, dbCY + 18, colBlack, TRUE);
        DrawBox(dbCX - 22, dbCY - 16, dbCX - 14, dbCY + 16, colOrange, TRUE);
        DrawBox(dbCX - 29, dbCY - 12, dbCX - 23, dbCY + 12, colYellow, TRUE);

        DrawBox(dbCX + 12, dbCY - 18, dbCX + 24, dbCY + 18, colBlack, TRUE);
        DrawBox(dbCX + 14, dbCY - 16, dbCX + 22, dbCY + 16, colOrange, TRUE);
        DrawBox(dbCX + 23, dbCY - 12, dbCX + 29, dbCY + 12, colYellow, TRUE);

        DrawStringToHandle(icX + 76, iconCardY + 12, "DUMBBELL", colYellow, font16);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d PTS", dPts);
        DrawBox(icX + 74, iconCardY + 38, icX + iconCardW - 8, iconCardY + 64, colNeonRed, TRUE);
        DrawStringToHandle(icX + 80, iconCardY + 43, ptBuf, colWhite, font16);
    }

    // [アイコン2: 猫缶 (CAT CAN BONUS)]
    {
        int icX = iconStartX + iconCardW + iconGap;
        SoftUI::DrawSoftCard(icX, iconCardY, iconCardW, iconCardH, 12, GetColor(18, 22, 34), 235, colYellow, 1);

        // 猫缶のグラフィカル描画（ゴールド＆レッドラベル缶）
        int canCX = icX + 38;
        int canCY = iconCardY + 40;
        DrawBox(canCX - 22, canCY - 18, canCX + 22, canCY + 18, colBlack, TRUE);
        DrawBox(canCX - 20, canCY - 16, canCX + 20, canCY + 16, colNeonRed, TRUE);
        DrawBox(canCX - 20, canCY - 6,  canCX + 20, canCY + 8,  colYellow, TRUE); // 中央イエロー帯
        DrawOval(canCX, canCY - 16, 20, 6, colBlack, FALSE);
        DrawOval(canCX, canCY - 16, 19, 5, colGold, TRUE); // 缶の上蓋
        DrawOval(canCX, canCY - 16, 12, 3, GetColor(255, 245, 190), FALSE);

        DrawStringToHandle(icX + 76, iconCardY + 12, "CAT CAN", colYellow, font16);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d PTS", cPts);
        DrawBox(icX + 74, iconCardY + 38, icX + iconCardW - 8, iconCardY + 64, colOrange, TRUE);
        DrawStringToHandle(icX + 80, iconCardY + 43, ptBuf, colBlack, font16);
    }

    // [アイコン3: スター (TIME BONUS / CLEAR STAR)]
    {
        int icX = iconStartX + (iconCardW + iconGap) * 2;
        SoftUI::DrawSoftCard(icX, iconCardY, iconCardW, iconCardH, 12, GetColor(18, 22, 34), 235, colGold, 1);

        // スターのグラフィカル描画
        int stCX = icX + 38;
        int stCY = iconCardY + 38;
        DrawCircle(stCX, stCY, 22, colBlack, TRUE);
        DrawCircle(stCX, stCY, 19, colNeonRed, TRUE);
        DrawStringToHandle(stCX - 17, stCY - 18, "★", colBlack, font36);
        DrawStringToHandle(stCX - 18, stCY - 19, "★", colGold, font36);

        DrawStringToHandle(icX + 76, iconCardY + 12, Config::USE_TIME_LIMIT_MODE ? "TIME BONUS" : "CLEAR STAR", colGold, font16);
        char ptBuf[32];
        snprintf(ptBuf, sizeof(ptBuf), "+%d PTS", sPts);
        DrawBox(icX + 74, iconCardY + 38, icX + iconCardW - 8, iconCardY + 64, colGold, TRUE);
        DrawStringToHandle(icX + 80, iconCardY + 43, ptBuf, colBlack, font16);
    }

    // =========================================================================
    // 4. アクションボタン（下部：ふわっと浮遊する角丸ピルボタン）
    // =========================================================================
    int btnW = 340;
    int btnH = 66;
    int btnGap = 50;
    int totalBtnsW = btnW * 2 + btnGap;
    int btn1X = (Config::SCREEN_WIDTH - totalBtnsW) / 2;
    int btn2X = btn1X + btnW + btnGap;
    int btnY  = 618;

    float btnPulse = (std::sin(animSec * 8.0f) + 1.0f) * 0.5f;

    // ふわっとした角丸カプセルボタン描画ラムダ
    auto DrawArcadeButton = [&](int bx, int by, bool isSelected, const char* label, const char* keyBadge, unsigned int activeMainCol)
    {
        float floatY = isSelected ? (std::sin(animSec * 6.0f) * 3.0f) : 0.0f;
        int curBy = by + static_cast<int>(floatY);

        unsigned int fillCol = isSelected ? activeMainCol : GetColor(45, 52, 75);
        unsigned int borderCol = isSelected ? colGold : GetColor(70, 80, 110);

        SoftUI::DrawSoftCard(bx, curBy, btnW, btnH, 28, fillCol, 255, borderCol, 2, isSelected, borderCol);

        if (isSelected)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(bx + 6, curBy + 3, bx + btnW - 6, curBy + 18, 14, 14, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 左側：入力対応キーアイコンバッジ（角丸ピル）
        int keyW = 76;
        int keyH = 40;
        int keyX = bx + 16;
        int keyY = curBy + (btnH - keyH) / 2;

        SoftUI::DrawSoftCard(keyX, keyY, keyW, keyH, 12, colNeonRed, 255, colWhite, 1);

        int kbTextW = GetDrawStringWidthToHandle(keyBadge, static_cast<int>(strlen(keyBadge)), font18);
        DrawStringToHandle(keyX + (keyW - kbTextW) / 2, keyY + 11, keyBadge, colWhite, font18);

        // 右側：ボタンメインラベル（太字アウトライン付き）
        int labelAreaX = keyX + keyW + 12;
        int labelAreaW = (bx + btnW - 12) - labelAreaX;
        int lbTextW = GetDrawStringWidthToHandle(label, static_cast<int>(strlen(label)), font24);
        int lbX = labelAreaX + (labelAreaW - lbTextW) / 2;
        int lbY = curBy + (btnH - 24) / 2;

        for (int dy = -2; dy <= 2; ++dy)
        {
            for (int dx = -2; dx <= 2; ++dx)
            {
                if (dx != 0 || dy != 0)
                {
                    DrawStringToHandle(lbX + dx, lbY + dy, label, colBlack, font24);
                }
            }
        }
        DrawStringToHandle(lbX + 2, lbY + 2, label, colBlack, font24);
        DrawStringToHandle(lbX, lbY, label, isSelected ? colWhite : GetColor(215, 225, 245), font24);
    };

    // ボタン1: RETRY [R / SPACE]
    DrawArcadeButton(btn1X, btnY, (m_clearSelectedBtn == 0), "RETRY !!", "[R]", colOrange);

    // ボタン2: MAIN MENU [M / T]
    DrawArcadeButton(btn2X, btnY, (m_clearSelectedBtn == 1), "MAIN MENU", "[M]", colYellow);
}

// ============================================================================
// タイトル画面専用の超豪華3Dステージ演出（ゴールデンステージリング・金粉・ライト柱・マッスルオーラ）
// ============================================================================
void GameManager::DrawTitle3DScene()
{
    auto player = m_objManager.GetPlayer();
    VECTOR catPos = (player) ? player->GetPos() : VGet(0.0f, 0.0f, 0.0f);
    float catY = catPos.y;

    int cycle = m_titleAnimTimer % 540;
    bool isSquat = (cycle < 200);
    bool isPushUp = (cycle >= 270 && cycle < 470);
    bool isTraining = isSquat || isPushUp;

    // ------------------------------------------------------------------------
    // 1. 床面：ゴールデン・マッスル・ステージリング（同心円＆回転放射スポーク）
    // ------------------------------------------------------------------------
    float ringY = catY + 0.35f; // 地面とのZファイティング防止
    int segments = 32;

    SetDrawBlendMode(DX_BLENDMODE_ADD, 160);

    // 内円リング (半径 32)
    for (int i = 0; i < segments; ++i)
    {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / segments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / segments);
        VECTOR p1 = VGet(catPos.x + std::cos(a1) * 32.0f, ringY, catPos.z + std::sin(a1) * 32.0f);
        VECTOR p2 = VGet(catPos.x + std::cos(a2) * 32.0f, ringY, catPos.z + std::sin(a2) * 32.0f);
        DrawLine3D(p1, p2, GetColor(255, 215, 60));
    }

    // 中円リング (半径 55)
    for (int i = 0; i < segments; ++i)
    {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / segments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / segments);
        VECTOR p1 = VGet(catPos.x + std::cos(a1) * 55.0f, ringY, catPos.z + std::sin(a1) * 55.0f);
        VECTOR p2 = VGet(catPos.x + std::cos(a2) * 55.0f, ringY, catPos.z + std::sin(a2) * 55.0f);
        DrawLine3D(p1, p2, GetColor(255, 140, 40));
    }

    // 外円リング (半径 78) ＆ 回転放射スポーク
    float rotSpoke = m_titleAnimTimer * 0.012f;
    for (int i = 0; i < segments; ++i)
    {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / segments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / segments);
        VECTOR p1 = VGet(catPos.x + std::cos(a1) * 78.0f, ringY, catPos.z + std::sin(a1) * 78.0f);
        VECTOR p2 = VGet(catPos.x + std::cos(a2) * 78.0f, ringY, catPos.z + std::sin(a2) * 78.0f);
        DrawLine3D(p1, p2, GetColor(80, 220, 255));
    }

    // 16本の回転放射スポーク線
    for (int i = 0; i < 16; ++i)
    {
        float a = rotSpoke + static_cast<float>(i) * (2.0f * MathHelper::PI / 16.0f);
        VECTOR pInner = VGet(catPos.x + std::cos(a) * 32.0f, ringY, catPos.z + std::sin(a) * 32.0f);
        VECTOR pOuter = VGet(catPos.x + std::cos(a) * 78.0f, ringY, catPos.z + std::sin(a) * 78.0f);
        DrawLine3D(pInner, pOuter, (i % 2 == 0) ? GetColor(255, 230, 80) : GetColor(255, 120, 30));
    }

    // ------------------------------------------------------------------------
    // 2. 筋トレ連動・床面拡散ショックウェーブ（レップのリズムで広がる光の輪）
    // ------------------------------------------------------------------------
    for (int w = 0; w < 2; ++w)
    {
        float waveProgress = std::fmod(m_titleAnimTimer * 2.2f + static_cast<float>(w) * 45.0f, 90.0f);
        float waveRadius = waveProgress;
        int waveAlpha = static_cast<int>((1.0f - waveProgress / 90.0f) * 190.0f);
        if (waveAlpha > 0)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, waveAlpha);
            unsigned int waveColor = isPushUp ? GetColor(80, 220, 255) : GetColor(255, 215, 60);
            for (int i = 0; i < segments; ++i)
            {
                float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / segments);
                float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / segments);
                VECTOR p1 = VGet(catPos.x + std::cos(a1) * waveRadius, ringY + 0.1f, catPos.z + std::sin(a1) * waveRadius);
                VECTOR p2 = VGet(catPos.x + std::cos(a2) * waveRadius, ringY + 0.1f, catPos.z + std::sin(a2) * waveRadius);
                DrawLine3D(p1, p2, waveColor);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 3. 天空へ昇るゴールド・ライトシャフト（8本の光の柱）
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
    for (int i = 0; i < 8; ++i)
    {
        float beamAngle = rotSpoke * 0.7f + static_cast<float>(i) * (2.0f * MathHelper::PI / 8.0f);
        float bx = catPos.x + std::cos(beamAngle) * 55.0f;
        float bz = catPos.z + std::sin(beamAngle) * 55.0f;
        float beamHeight = 90.0f + std::sin(m_titleAnimTimer * 0.05f + static_cast<float>(i)) * 20.0f;

        VECTOR bBase = VGet(bx, catY, bz);
        VECTOR bTop = VGet(bx, catY + beamHeight, bz);
        unsigned int bCol = (i % 2 == 0) ? GetColor(255, 220, 70) : GetColor(100, 230, 255);
        DrawLine3D(bBase, bTop, bCol);
        DrawLine3D(VGet(bx + 0.6f, catY, bz), VGet(bx + 0.6f, catY + beamHeight, bz), bCol);
        DrawLine3D(VGet(bx, catY, bz + 0.6f), VGet(bx, catY + beamHeight, bz + 0.6f), bCol);
    }

    // ------------------------------------------------------------------------
    // 4. マッスル・パワーオーラ（猫の体から立ち昇るエネルギースフィア）
    // ------------------------------------------------------------------------
    if (isTraining)
    {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
        for (int i = 0; i < 8; ++i)
        {
            float phase = m_titleAnimTimer * 0.06f + static_cast<float>(i) * 0.785f;
            float orbR = 12.0f + std::sin(phase * 1.5f) * 4.0f;
            float orbX = catPos.x + std::sin(phase) * orbR;
            float orbZ = catPos.z + std::cos(phase) * orbR;
            float orbY = catY + 4.0f + std::fmod(m_titleAnimTimer * 1.1f + static_cast<float>(i) * 6.0f, 38.0f);
            float orbRadius = 2.0f + std::sin(phase * 2.0f) * 0.7f;
            unsigned int orbCol = (i % 2 == 0) ? GetColor(255, 190, 40) : GetColor(255, 90, 20);
            DrawSphere3D(VGet(orbX, orbY, orbZ), orbRadius, 6, orbCol, orbCol, TRUE);
        }
    }

    // ------------------------------------------------------------------------
    // 5. 3D空間を舞う金粉・紙吹雪（50個のパーティクル）
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ADD, 190);
    for (int i = 0; i < 50; ++i)
    {
        float pAngle = static_cast<float>(i) * 0.628f + m_titleAnimTimer * 0.003f;
        float pDist = 35.0f + std::fmod(static_cast<float>(i * 37 + 13), 130.0f);
        float px = catPos.x + std::sin(pAngle) * pDist;
        float pz = catPos.z + std::cos(pAngle) * pDist;
        float py = catY + std::fmod(static_cast<float>(i * 13) + m_titleAnimTimer * 0.65f, 110.0f);

        unsigned int pCol;
        int colType = i % 4;
        if (colType == 0) pCol = GetColor(255, 220, 60);      // ゴールド
        else if (colType == 1) pCol = GetColor(255, 80, 180); // ホットピンク
        else if (colType == 2) pCol = GetColor(60, 220, 255); // スカイブルー
        else pCol = GetColor(100, 255, 120);                   // ネオングリーン

        float pSize = 1.3f + std::fmod(static_cast<float>(i), 3.0f) * 0.5f;
        DrawSphere3D(VGet(px, py, pz), pSize, 4, pCol, pCol, TRUE);
    }

    // ------------------------------------------------------------------------
    // 6. 猫の頭上を回るチャンピオン・オービット星（3つの光る星）
    // ------------------------------------------------------------------------
    float starBaseY = (isPushUp) ? (catY + 22.0f) : (catY + 38.0f);
    for (int i = 0; i < 3; ++i)
    {
        float starAngle = m_titleAnimTimer * 0.045f + static_cast<float>(i) * (2.0f * MathHelper::PI / 3.0f);
        float sx = catPos.x + std::sin(starAngle) * 16.0f;
        float sz = catPos.z + std::cos(starAngle) * 16.0f;
        float sy = starBaseY + std::sin(m_titleAnimTimer * 0.08f + static_cast<float>(i)) * 2.5f;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 230);
        DrawSphere3D(VGet(sx, sy, sz), 2.2f, 6, GetColor(255, 230, 80), GetColor(255, 200, 40), TRUE);
        DrawLine3D(VGet(sx - 3.5f, sy, sz), VGet(sx + 3.5f, sy, sz), GetColor(255, 255, 220));
        DrawLine3D(VGet(sx, sy - 3.5f, sz), VGet(sx, sy + 3.5f, sz), GetColor(255, 255, 220));
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ============================================================================
// タイトル画面の描画（超ド派手ポップ＆カートゥーン風デザイン・アメコミ演出）
// ============================================================================
void GameManager::DrawTitleScreen()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    int font60 = fm.GetFont60();

    // ------------------------------------------------------------------------
    // 1. デュアル・シアター・サーチライト（底面左右の角から大空へ交差する超巨大光線）
    // ------------------------------------------------------------------------
    {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 48);
        // 左サーチライト（スカイシアン）
        float aL = -0.72f + std::sin(m_titleAnimTimer * 0.016f) * 0.32f;
        int lx1 = 0, ly1 = Config::SCREEN_HEIGHT;
        int lx2 = static_cast<int>(std::cos(aL - 0.12f) * 1300.0f);
        int ly2 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aL - 0.12f) * 1300.0f);
        int lx3 = static_cast<int>(std::cos(aL + 0.12f) * 1300.0f);
        int ly3 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aL + 0.12f) * 1300.0f);
        DrawTriangle(lx1, ly1, lx2, ly2, lx3, ly3, GetColor(60, 200, 255), TRUE);

        // 右サーチライト（ゴールデンアンバー）
        float aR = -2.42f + std::sin(m_titleAnimTimer * 0.019f + 1.2f) * 0.32f;
        int rx1 = Config::SCREEN_WIDTH, ry1 = Config::SCREEN_HEIGHT;
        int rx2 = Config::SCREEN_WIDTH + static_cast<int>(std::cos(aR - 0.12f) * 1300.0f);
        int ry2 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aR - 0.12f) * 1300.0f);
        int rx3 = Config::SCREEN_WIDTH + static_cast<int>(std::cos(aR + 0.12f) * 1300.0f);
        int ry3 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aR + 0.12f) * 1300.0f);
        DrawTriangle(rx1, ry1, rx2, ry2, rx3, ry3, GetColor(255, 210, 50), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ------------------------------------------------------------------------
    // 2. 超ド派手ダブル・ハイパー・サンバースト（2層の逆回転光条＆中心グロー）
    // ------------------------------------------------------------------------
    int sunburstCX = Config::SCREEN_WIDTH / 2;
    int sunburstCY = 150;

    // A. 外層サンバースト（20光条・正回転・シアン＆マゼンタ）
    float sunAngleOuter = m_titleAnimTimer * 0.005f;
    int rayCountOuter = 20;
    float rayRadiusOuter = 720.0f;
    SetDrawBlendMode(DX_BLENDMODE_ADD, 40);
    for (int r = 0; r < rayCountOuter; ++r)
    {
        float a1 = sunAngleOuter + static_cast<float>(r * 2) * (MathHelper::PI / rayCountOuter);
        float a2 = a1 + (MathHelper::PI / rayCountOuter) * 0.85f;
        int p1x = sunburstCX + static_cast<int>(std::cos(a1) * rayRadiusOuter);
        int p1y = sunburstCY + static_cast<int>(std::sin(a1) * rayRadiusOuter);
        int p2x = sunburstCX + static_cast<int>(std::cos(a2) * rayRadiusOuter);
        int p2y = sunburstCY + static_cast<int>(std::sin(a2) * rayRadiusOuter);

        unsigned int col = (r % 2 == 0) ? GetColor(100, 220, 255) : GetColor(255, 80, 200);
        DrawTriangle(sunburstCX, sunburstCY, p1x, p1y, p2x, p2y, col, TRUE);
    }

    // B. 内層サンバースト（16光条・逆回転・ビタミンゴールド）
    float sunAngleInner = -m_titleAnimTimer * 0.008f;
    int rayCountInner = 16;
    float rayRadiusInner = 500.0f;
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 85);
    for (int r = 0; r < rayCountInner; ++r)
    {
        float a1 = sunAngleInner + static_cast<float>(r * 2) * (MathHelper::PI / rayCountInner);
        float a2 = a1 + (MathHelper::PI / rayCountInner) * 0.85f;
        int p1x = sunburstCX + static_cast<int>(std::cos(a1) * rayRadiusInner);
        int p1y = sunburstCY + static_cast<int>(std::sin(a1) * rayRadiusInner);
        int p2x = sunburstCX + static_cast<int>(std::cos(a2) * rayRadiusInner);
        int p2y = sunburstCY + static_cast<int>(std::sin(a2) * rayRadiusInner);

        DrawTriangle(sunburstCX, sunburstCY, p1x, p1y, p2x, p2y, GetColor(255, 220, 50), TRUE);
    }

    // C. 中心ラジアルブルーム
    SetDrawBlendMode(DX_BLENDMODE_ADD, 55);
    DrawCircle(sunburstCX, sunburstCY, 190, GetColor(255, 230, 100), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ------------------------------------------------------------------------
    // 3. 画面を飛び交うポップな2D漫符・星・キラキラ（24個の浮遊パーティクル）
    // ------------------------------------------------------------------------
    for (int i = 0; i < 24; ++i)
    {
        float pX = std::fmod(static_cast<float>(i * 57 + 35) + std::sin(m_titleAnimTimer * 0.02f + static_cast<float>(i)) * 22.0f, static_cast<float>(Config::SCREEN_WIDTH));
        float pY = static_cast<float>(Config::SCREEN_HEIGHT) - std::fmod(static_cast<float>(i * 42) + m_titleAnimTimer * 1.25f, static_cast<float>(Config::SCREEN_HEIGHT + 30));

        const char* pGlyph = (i % 5 == 0) ? "★" : (i % 5 == 1 ? "✦" : (i % 5 == 2 ? "⚡" : (i % 5 == 3 ? "◆" : "💥")));
        unsigned int pCol = (i % 3 == 0) ? GetColor(255, 230, 60) : (i % 3 == 1 ? GetColor(255, 100, 180) : GetColor(80, 220, 255));
        int pFont = (i % 2 == 0) ? font18 : font24;

        if (pY > 48.0f && pY < static_cast<float>(Config::SCREEN_HEIGHT - 50))
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 175);
            DrawStringToHandle(static_cast<int>(pX), static_cast<int>(pY), pGlyph, pCol, pFont);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }

    // ------------------------------------------------------------------------
    // 4. ポップなコミック風ヘッダー＆フッターバー
    // ------------------------------------------------------------------------
    DrawBox(0, 0, Config::SCREEN_WIDTH, 48, GetColor(20, 24, 45), TRUE);
    DrawBox(0, 44, Config::SCREEN_WIDTH, 48, GetColor(255, 215, 30), TRUE);
    DrawBox(0, 48, Config::SCREEN_WIDTH, 50, GetColor(0, 0, 0), TRUE);

    DrawBox(0, Config::SCREEN_HEIGHT - 50, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 48, GetColor(0, 0, 0), TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 48, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT - 44, GetColor(255, 215, 30), TRUE);
    DrawBox(0, Config::SCREEN_HEIGHT - 44, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(20, 24, 45), TRUE);

    DrawStringToHandle(25, 14, "★ THE MOST PUMPED FELINE ADVENTURE! ★", GetColor(255, 230, 60), font16);
    DrawStringToHandle(Config::SCREEN_WIDTH - 280, 14, "★ WORLD MUSCLE LEAGUE 2026 ★", GetColor(100, 230, 255), font16);

    const char* helpStr = "[ ↑ / ↓ ] SELECT   [ ← / → ] CHANGE STAGE   [ ENTER / SPACE ] DECIDE   [ H / TAB ] HOW TO PLAY";
    int helpW = GetDrawStringWidthToHandle(helpStr, static_cast<int>(strlen(helpStr)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - helpW) / 2, Config::SCREEN_HEIGHT - 32, helpStr, GetColor(255, 255, 255), font16);
    DrawStringToHandle(Config::SCREEN_WIDTH - 100, Config::SCREEN_HEIGHT - 30, "VER 1.0", GetColor(100, 220, 255), font13);

    // ------------------------------------------------------------------------
    // 5. タイトルロゴ背面の半透明コミックプレート
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 115);
    DrawBox(0, 65, Config::SCREEN_WIDTH, 275, GetColor(15, 18, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    // 6. 元気で弾む「マッスルにゃんこ」カートゥーンロゴ（ボヨヨンバウンス＋極太ブラックアウトライン＋ビビッドカラー）
    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    float t = GetNowCount() * 0.005f;
    float bounceOffset = std::abs(std::sin(t)) * -12.0f; // ポンポン弾むバウンス
    int logoBaseY = static_cast<int>(88.0f + bounceOffset);

    const char* txtMuscle = "マッスル";
    const char* txtCat    = "にゃんこ";
    const char* txt3D     = "";
    int wMuscle = GetDrawStringWidthToHandle(txtMuscle, static_cast<int>(strlen(txtMuscle)), font60);
    int wCat    = GetDrawStringWidthToHandle(txtCat, static_cast<int>(strlen(txtCat)), font60);
    int w3D     = GetDrawStringWidthToHandle(txt3D, static_cast<int>(strlen(txt3D)), font60);
    int totalLogoW = wMuscle + wCat + w3D;

    int logoStartX = (Config::SCREEN_WIDTH - totalLogoW) / 2;
    int xMuscle = logoStartX;
    int xCat    = logoStartX + wMuscle;
    int x3D     = logoStartX + wMuscle + wCat;

    // A. コミック太ブラックシャドウ（右下オフセット +6, +7）
    for (int dy = 5; dy <= 8; ++dy)
    {
        for (int dx = 5; dx <= 8; ++dx)
        {
            DrawStringToHandle(xMuscle + dx, logoBaseY + dy, txtMuscle, GetColor(0, 0, 0), font60);
            DrawStringToHandle(xCat + dx,    logoBaseY + dy, txtCat,    GetColor(0, 0, 0), font60);
            DrawStringToHandle(x3D + dx,     logoBaseY + dy, txt3D,     GetColor(0, 0, 0), font60);
        }
    }

    // B. 極太ブラックアウトライン（コミック境界線: 半径-4〜+4）
    for (int dy = -4; dy <= 4; ++dy)
    {
        for (int dx = -4; dx <= 4; ++dx)
        {
            if (dx != 0 || dy != 0)
            {
                DrawStringToHandle(xMuscle + dx, logoBaseY + dy, txtMuscle, GetColor(0, 0, 0), font60);
                DrawStringToHandle(xCat + dx,    logoBaseY + dy, txtCat,    GetColor(0, 0, 0), font60);
                DrawStringToHandle(x3D + dx,     logoBaseY + dy, txt3D,     GetColor(0, 0, 0), font60);
            }
        }
    }

    // C. カートゥーン本体カラー
    // MUSCLE: ホットオレンジ〜ビタミンイエロー
    DrawStringToHandle(xMuscle, logoBaseY + 2, txtMuscle, GetColor(255, 120, 20), font60);
    DrawStringToHandle(xMuscle, logoBaseY,     txtMuscle, GetColor(255, 215, 30), font60);
    DrawStringToHandle(xMuscle, logoBaseY - 2, txtMuscle, GetColor(255, 255, 180), font60); // トップハイライト

    // CAT: スカイブルー〜ピュアホワイト
    DrawStringToHandle(xCat, logoBaseY + 2, txtCat, GetColor(40, 160, 240), font60);
    DrawStringToHandle(xCat, logoBaseY,     txtCat, GetColor(120, 230, 255), font60);
    DrawStringToHandle(xCat, logoBaseY - 2, txtCat, GetColor(240, 255, 255), font60);

    // 3D: ビビッドホットピンク〜イエロー
    DrawStringToHandle(x3D, logoBaseY + 2, txt3D, GetColor(240, 40, 120), font60);
    DrawStringToHandle(x3D, logoBaseY,     txt3D, GetColor(255, 80, 180), font60);
    DrawStringToHandle(x3D, logoBaseY - 2, txt3D, GetColor(255, 220, 240), font60);

    // D. 光の筋・シネマティックスペキュラースイープ（ロゴを横切る白い閃光）
    int glintCycle = m_titleAnimTimer % 180;
    if (glintCycle < 40)
    {
        float glintProgress = static_cast<float>(glintCycle) / 40.0f;
        int glintX = logoStartX - 60 + static_cast<int>(glintProgress * (totalLogoW + 120));
        int glintY1 = logoBaseY - 18;
        int glintY2 = logoBaseY + 75;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 175);
        DrawTriangle(glintX - 18, glintY2, glintX + 18, glintY1, glintX + 42, glintY1, GetColor(255, 255, 255), TRUE);
        DrawTriangle(glintX - 18, glintY2, glintX + 42, glintY1, glintX + 6, glintY2, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // E. ロゴ左右のダイナミック・コミックバッジ
    {
        float badgePulse = std::sin(m_titleAnimTimer * 0.08f) * 4.0f;

        // 左バッジ: [ ★ PUMP ★ ]（ふわっとした丸みカプセル）
        int lBadgeX = logoStartX - 105;
        int lBadgeY = logoBaseY + 14 + static_cast<int>(badgePulse);
        SoftUI::DrawSoftCard(lBadgeX, lBadgeY, 92, 30, 15, GetColor(255, 120, 20), 245, GetColor(255, 220, 80), 1);
        DrawStringToHandle(lBadgeX + 7, lBadgeY + 6, "★ PUMP ★", GetColor(255, 255, 255), font16);

        // 右バッジ: [ ★ BULK ★ ]（ふわっとした丸みカプセル）
        int rBadgeX = logoStartX + totalLogoW + 15;
        int rBadgeY = logoBaseY + 14 - static_cast<int>(badgePulse);
        SoftUI::DrawSoftCard(rBadgeX, rBadgeY, 92, 30, 15, GetColor(240, 40, 120), 245, GetColor(255, 180, 230), 1);
        DrawStringToHandle(rBadgeX + 7, rBadgeY + 6, "★ BULK ★", GetColor(255, 255, 255), font16);
    }

    // F. サブタイトル（アメコミバナー風）
    const char* subText = "★ MAXIMUM CARTOON PUMP CHASE ★";
    int subW = GetDrawStringWidthToHandle(subText, static_cast<int>(strlen(subText)), font18);
    int subX = (Config::SCREEN_WIDTH - subW) / 2;
    int subY = logoBaseY + 76;

    // サブタイトルの黒縁＋ビビッドイエロー
    DrawStringToHandle(subX + 2, subY + 2, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX - 1, subY,     subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX + 1, subY,     subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY - 1, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY + 1, subText, GetColor(0, 0, 0), font18);
    DrawStringToHandle(subX,     subY,     subText, GetColor(255, 245, 150), font18);

    // ------------------------------------------------------------------------
    // 7. 爆笑・ボディービル掛け声ポップアップ演出（ふわふわマシュマロ雲吹き出し！）
    // ------------------------------------------------------------------------
    {
        // A. 状況別の多彩な筋肉コール・ライブラリ（合計48種類以上！）
        // 1. スクワット中 (0〜200F)
        static const char* squatShouts[] = {
            "しゃがみが深い！ナイススクワット！",
            "大腿四頭筋が咆哮してる！",
            "脚が丸太！樹齢一千年！",
            "重力に勝ってるぞ猫ちゃん！",
            "地球を押し下げろ！",
            "大殿筋のパワー全開！",
            "バーベルが羽毛に見える！",
            "フルスクワットの神！",
            "太ももに恐竜飼ってんのかい！",
            "スクワットで床が鳴いてる！",
            "ニャイススクワット！！",
            "脚のキレが異次元！"
        };
        // 2. 腕立て伏せ中 (270〜470F)
        static const char* pushUpShouts[] = {
            "床を押すな！地球を押せ！",
            "大胸筋で床を割れ！",
            "プッシュ！限界突破！",
            "腕立ての鬼神降臨！",
            "上腕三頭筋が爆発寸前！",
            "前足が油圧ジャッキ！",
            "胸筋が巨大メロンパン！",
            "床がビビって逃げてるぞ！",
            "プッシュアップの極致！",
            "大胸筋が歩き出しそう！",
            "ニャイスプッシュアップ！！",
            "鋼鉄の肉球プレス！"
        };
        // 3. インターバル・ポーズ中 (200〜270F & 470〜540F)
        static const char* poseShouts[] = {
            "ナイスバルク！！",
            "キレてるよ！キレてる！",
            "肩にちっちゃい重機乗せてんのかい！",
            "大胸筋が歩いてる！",
            "背中に鬼の顔！",
            "腹筋が板チョコ！",
            "筋肉大爆発！！",
            "パワーーーッ！！",
            "肩メロン！！巨大メロン！",
            "広背筋で空飛べそう！",
            "僧帽筋が富士山！",
            "巨塔！スカイツリー！",
            "血管が高速道路網！",
            "新時代のダビデ猫像！",
            "筋肉の猫国宝！！",
            "大胸筋が喜びの歌を歌ってる！",
            "マッチョの最高峰！",
            "バリバリにキレてる！",
            "上腕二頭筋がチョモランマ！",
            "ニャイスバルク！！",
            "筋肉が怒涛の進撃！",
            "仕上がりすぎてる！！",
            "筋肉界のファンタジスタ！",
            "カリカリ食ってパンプアップ！"
        };

        int cycle = m_titleAnimTimer % 540;
        const char** activePool = poseShouts;
        int activeCount = sizeof(poseShouts) / sizeof(poseShouts[0]);

        if (cycle < 200)
        {
            activePool = squatShouts;
            activeCount = sizeof(squatShouts) / sizeof(squatShouts[0]);
        }
        else if (cycle >= 270 && cycle < 470)
        {
            activePool = pushUpShouts;
            activeCount = sizeof(pushUpShouts) / sizeof(pushUpShouts[0]);
        }

        // --- 6個の掛け声吹き出し（左右3箇所ずつ配置、約3秒スパンでリズミカルに入れ替え） ---
        const int SHOUT_SPAN = 180; // 約3秒スパン (60fps * 3s)

        struct BubbleSlot
        {
            bool isRight;
            int baseY;
            int timerOffset;
            unsigned int bgCol;
            unsigned int textCol;
        };

        BubbleSlot slots[6] = {
            // 左側3箇所 (上・中・下) - パステルマシュマロカラー
            { false, 125,   0, GetColor(255, 240, 140), GetColor(35, 30, 20) }, // クリームバター
            { false, 280,  60, GetColor(165, 235, 255), GetColor(20, 35, 45) }, // パステルシアン
            { false, 435, 120, GetColor(175, 245, 195), GetColor(20, 40, 25) }, // ミントグリーン

            // 右側3箇所 (上・中・下) - パステルマシュマロカラー
            { true,  125,  30, GetColor(255, 185, 215), GetColor(45, 20, 35) }, // サクラピンク
            { true,  280,  90, GetColor(255, 200, 150), GetColor(45, 30, 20) }, // アプリコットピーチ
            { true,  435, 150, GetColor(220, 190, 255), GetColor(35, 20, 45) }  // パステルラベンダー
        };

        for (int i = 0; i < 6; ++i)
        {
            const auto& slot = slots[i];
            int timer = m_titleAnimTimer + slot.timerOffset;
            int step = timer / SHOUT_SPAN;
            int frame = timer % SHOUT_SPAN;

            // アニメーション：3秒ごとの切り替え時に弾むポップバウンス
            float bounce = (frame < 18) ? (std::sin((static_cast<float>(frame) / 18.0f) * MathHelper::PI) * -8.0f) : 0.0f;
            // ふわふわ浮遊アニメーション（スロットごとに独立した周期で優しく揺れる）
            float floatY = std::sin(m_titleAnimTimer * 0.045f + i * 1.05f) * 3.5f;

            // 6個が被らないようスロットごとのオフセットを付与
            int idx = (step * 5 + i * 2) % activeCount;
            const char* shoutText = activePool[idx];

            int textW = GetDrawStringWidthToHandle(shoutText, static_cast<int>(strlen(shoutText)), font16);
            int sW = textW + 30;
            int sH = 38;
            int sX = slot.isRight ? (Config::SCREEN_WIDTH - sW - 32) : 32;
            int sY = slot.baseY + static_cast<int>(floatY + bounce);

            // ふわふわの雲・マシュマロ吹き出しを描画
            SoftUI::DrawFluffySpeechBubble(sX, sY, sW, sH, 18, slot.bgCol, slot.textCol, slot.isRight, shoutText, font16);
        }
    }

    // ------------------------------------------------------------------------
    // 8. ふわふわカプセル・メニューボタン（4項目・ソフトピルデザイン＆浮遊感）
    // ------------------------------------------------------------------------
    int btnW = 420;
    int btnH = 46;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int startBtnY = 338;
    int stageBtnY = 390;
    int howBtnY   = 442;
    int soundBtnY = 494;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: GAME START ---
    bool isStartSelected = (m_selectedMenuItem == 0);
    {
        float floatY = isStartSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = startBtnY + static_cast<int>(floatY);

        if (isStartSelected)
        {
            unsigned int neonColor = ((m_titleAnimTimer / 15) % 2 == 0) ? GetColor(255, 220, 60) : GetColor(255, 140, 40);
            unsigned int bodyColor = GetColor(255, 220 + static_cast<int>(30 * btnPulse), 35);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            // 上部ハイライトピル
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ボタン上を滑る光の筋（スペキュラーハイライト）
            float glintX = btnX + std::fmod(m_titleAnimTimer * 5.0f, static_cast<float>(btnW + 80)) - 40.0f;
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            DrawRoundRect(static_cast<int>(glintX), curY + 3, static_cast<int>(glintX + 28), curY + btnH - 3, 10, 10, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 元気な矢印アイコン
            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 13, "▶", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 13, "◀", GetColor(25, 25, 30), font24);

            const char* startTxt = "GAME START !!";
            int tW = GetDrawStringWidthToHandle(startTxt, static_cast<int>(strlen(startTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, startTxt, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 13, startTxt, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            const char* startTxt = "GAME START";
            int tW = GetDrawStringWidthToHandle(startTxt, static_cast<int>(strlen(startTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, startTxt, GetColor(210, 230, 255), font24);
        }
    }

    // --- ボタン2: STAGE SELECT (MAP SWITCH) ---
    bool isStageSelected = (m_selectedMenuItem == 1);
    const char* stageNameStr = (m_selectedMap == ModelConfig::MapType::House)
        ? "STAGE : MAP 1 (HOUSE)"
        : "STAGE : MAP 2 (NEW MAP)";
    {
        float floatY = isStageSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = stageBtnY + static_cast<int>(floatY);

        if (isStageSelected)
        {
            unsigned int neonColor = ((m_titleAnimTimer / 15) % 2 == 0) ? GetColor(255, 170, 50) : GetColor(255, 70, 50);
            unsigned int bodyColor = GetColor(255, 140 + static_cast<int>(30 * btnPulse), 45);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            float glintX = btnX + std::fmod(m_titleAnimTimer * 5.0f, static_cast<float>(btnW + 80)) - 40.0f;
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            DrawRoundRect(static_cast<int>(glintX), curY + 3, static_cast<int>(glintX + 28), curY + btnH - 3, 10, 10, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int arrowPulse = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 4.0f);
            DrawStringToHandle(btnX + 22 - arrowPulse, curY + 13, "◀", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 + arrowPulse, curY + 13, "▶", GetColor(25, 25, 30), font24);

            int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, stageNameStr, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 13, stageNameStr, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            DrawStringToHandle(btnX + 22, curY + 13, "‹", GetColor(140, 170, 210), font24);
            DrawStringToHandle(btnX + btnW - 34, curY + 13, "›", GetColor(140, 170, 210), font24);

            int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, stageNameStr, GetColor(210, 230, 255), font24);
        }
    }

    // --- ボタン3: ゲーム説明 ---
    bool isHowSelected = (m_selectedMenuItem == 2);
    {
        float floatY = isHowSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = howBtnY + static_cast<int>(floatY);

        if (isHowSelected)
        {
            unsigned int neonColor = ((m_titleAnimTimer / 15) % 2 == 0) ? GetColor(80, 220, 255) : GetColor(60, 150, 255);
            unsigned int bodyColor = GetColor(80, 220 + static_cast<int>(30 * btnPulse), 255);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            float glintX = btnX + std::fmod(m_titleAnimTimer * 5.0f, static_cast<float>(btnW + 80)) - 40.0f;
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            DrawRoundRect(static_cast<int>(glintX), curY + 3, static_cast<int>(glintX + 28), curY + btnH - 3, 10, 10, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 13, "▶", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 13, "◀", GetColor(25, 25, 30), font24);

            const char* howTxt = "ゲーム説明 !!";
            int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, howTxt, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 13, howTxt, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            const char* howTxt = "ゲーム説明";
            int tW = GetDrawStringWidthToHandle(howTxt, static_cast<int>(strlen(howTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, howTxt, GetColor(210, 230, 255), font24);
        }
    }

    // --- ボタン4: サウンド設定 (BGM/SE音量・マウス感度) ---
    bool isSoundSelected = (m_selectedMenuItem == 3);
    {
        float floatY = isSoundSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = soundBtnY + static_cast<int>(floatY);

        if (isSoundSelected)
        {
            unsigned int neonColor = ((m_titleAnimTimer / 15) % 2 == 0) ? GetColor(160, 255, 120) : GetColor(80, 220, 100);
            unsigned int bodyColor = GetColor(100, 225 + static_cast<int>(30 * btnPulse), 110);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            float glintX = btnX + std::fmod(m_titleAnimTimer * 5.0f, static_cast<float>(btnW + 80)) - 40.0f;
            SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
            DrawRoundRect(static_cast<int>(glintX), curY + 3, static_cast<int>(glintX + 28), curY + btnH - 3, 10, 10, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 11, "▶", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 11, "◀", GetColor(25, 25, 30), font24);

            const char* soundTxt = "🔊 サウンド設定 (O) !!";
            int tW = GetDrawStringWidthToHandle(soundTxt, static_cast<int>(strlen(soundTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, soundTxt, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 11, soundTxt, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 22, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            const char* soundTxt = "🔊 サウンド設定 (O)";
            int tW = GetDrawStringWidthToHandle(soundTxt, static_cast<int>(strlen(soundTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, soundTxt, GetColor(210, 230, 255), font24);
        }
    }

    // ステージ切り替えガイドヒント（ふわっとした半透明角丸プレート）
    const char* stageHint = (m_selectedMap == ModelConfig::MapType::House)
        ? "★ [ ← / → ] またはクリックで MAP 2: NEW MAP に切替"
        : "★ [ ← / → ] またはクリックで MAP 1: 通常ハウス に切替";
    int hintW = GetDrawStringWidthToHandle(stageHint, static_cast<int>(strlen(stageHint)), font13);
    int hintX = (Config::SCREEN_WIDTH - hintW) / 2;
    int hintY = 552;

    SoftUI::DrawSoftCard(hintX - 18, hintY - 4, hintW + 36, 26, 13, GetColor(18, 24, 40), 200, GetColor(255, 215, 60), 1);
    DrawStringToHandle(hintX, hintY, stageHint, GetColor(255, 235, 120), font13);

    // ------------------------------------------------------------------------
    // 9. 豪華チャンピオン・ワークアウトHUD（画面左下：ふわっと浮遊する角丸HUD）
    // ------------------------------------------------------------------------
    {
        int cycle = m_titleAnimTimer % 540;
        float workoutFloat = std::sin(m_titleAnimTimer * 0.035f) * 2.5f;
        int bX = 32;
        int bY = Config::SCREEN_HEIGHT - 124 + static_cast<int>(workoutFloat);
        int bW = 314;
        int bH = 64;

        unsigned int borderCol = (cycle < 200)
            ? GetColor(255, 150, 40)
            : ((cycle >= 270 && cycle < 470) ? GetColor(80, 220, 255) : GetColor(120, 240, 160));

        SoftUI::DrawSoftCard(bX, bY, bW, bH, 16, GetColor(20, 26, 44), 225, borderCol, 2);

        // ヘッダー小見出し
        DrawStringToHandle(bX + 14, bY + 6, "★ LIVE TRAINING ROUTINE ★", GetColor(255, 230, 90), font13);

        if (cycle < 200)
        {
            // スクワットセット (0〜200F)
            int rep = (cycle < 65) ? 1 : ((cycle < 130) ? 2 : 3);
            float repProgress = std::fmod(static_cast<float>(cycle), 65.0f) / 65.0f;

            char buf[64];
            snprintf(buf, sizeof(buf), "SQUAT SET : REP %d / 3", rep);
            DrawStringToHandle(bX + 14, bY + 24, buf, GetColor(255, 220, 60), font18);

            // ゲージバー（角丸）
            int gX = bX + 14, gY = bY + 49, gW = bW - 28, gH = 6;
            DrawRoundRect(gX, gY, gX + gW, gY + gH, 3, 3, GetColor(12, 14, 20), TRUE);
            int fillW = static_cast<int>(gW * repProgress);
            if (fillW > 0)
            {
                int rVal = (fillW > 4) ? 3 : 1;
                DrawRoundRect(gX, gY, gX + fillW, gY + gH, rVal, rVal, GetColor(255, 145, 35), TRUE);
            }
        }
        else if (cycle >= 270 && cycle < 470)
        {
            // 腕立て伏せセット (270〜470F)
            int pCycle = cycle - 270;
            int rep = (pCycle < 50) ? 1 : ((pCycle < 100) ? 2 : ((pCycle < 150) ? 3 : 4));
            float repProgress = std::fmod(static_cast<float>(pCycle), 50.0f) / 50.0f;

            char buf[64];
            snprintf(buf, sizeof(buf), "PUSH-UP SET : REP %d / 4", rep);
            DrawStringToHandle(bX + 14, bY + 24, buf, GetColor(110, 235, 255), font18);

            // ゲージバー（角丸）
            int gX = bX + 14, gY = bY + 49, gW = bW - 28, gH = 6;
            DrawRoundRect(gX, gY, gX + gW, gY + gH, 3, 3, GetColor(12, 14, 20), TRUE);
            int fillW = static_cast<int>(gW * repProgress);
            if (fillW > 0)
            {
                int rVal = (fillW > 4) ? 3 : 1;
                DrawRoundRect(gX, gY, gX + fillW, gY + gH, rVal, rVal, GetColor(80, 220, 255), TRUE);
            }
        }
        else
        {
            // インターバル・ポーズ
            DrawStringToHandle(bX + 14, bY + 24, "INTERVAL : POSING TIME!", GetColor(140, 255, 180), font18);

            float restRatio = (cycle < 270) ? (static_cast<float>(cycle - 200) / 70.0f) : (static_cast<float>(cycle - 470) / 70.0f);
            int gX = bX + 14, gY = bY + 49, gW = bW - 28, gH = 6;
            DrawRoundRect(gX, gY, gX + gW, gY + gH, 3, 3, GetColor(12, 14, 20), TRUE);
            int fillW = static_cast<int>(gW * (1.0f - restRatio));
            if (fillW > 0)
            {
                int rVal = (fillW > 4) ? 3 : 1;
                DrawRoundRect(gX, gY, gX + fillW, gY + gH, rVal, rVal, GetColor(120, 240, 160), TRUE);
            }
        }
    }

    // ------------------------------------------------------------------------
    // 10. 遊び方モーダルが開いていれば描画
    // ------------------------------------------------------------------------
    if (m_showHowToPlay)
    {
        DrawHowToPlayModal();
    }

    // サウンド設定モーダルが開いていれば描画
    if (m_showSoundSettings)
    {
        DrawSoundSettingsModal();
    }

    // ------------------------------------------------------------------------
    // 11. ゲーム開始ホワイトトランジション
    // ------------------------------------------------------------------------
    if (m_startTransitionTimer > 0)
    {
        float alphaRatio = (22.0f - m_startTransitionTimer) / 22.0f;
        int alphaVal = static_cast<int>(alphaRatio * 255.0f);
        if (alphaVal > 255) alphaVal = 255;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaVal);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}

// ============================================================================
// ゲーム説明モーダルダイアログの描画（ゲーム画面UIと統一されたシンプル＆洗練デザイン）
// ============================================================================
void GameManager::DrawHowToPlayModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー（ゲーム中HUDと同じシックな半透明ダーク）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 16), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ本体（ふわっとしたソフト角丸ウィンドウ）
    int modalW = 860;
    int modalH = 520;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 22, GetColor(18, 22, 34), 235, GetColor(255, 130, 40), 2);

    // ヘッダーバー（角丸）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 52, 20, 20, GetColor(32, 42, 68), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 24, modalY + 52, modalX + modalW - 24, modalY + 52, GetColor(255, 130, 40));

    const char* modalHeader = "💥 ゲーム説明 - 筋肉ネズミ大捕獲作戦!! 💥";
    int hW = GetDrawStringWidthToHandle(modalHeader, static_cast<int>(strlen(modalHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 13, modalHeader, GetColor(255, 230, 80), font24);

    // 3. 上段：3ステップ基本ルールカード（ふわっとした角丸カード）
    int cardY = modalY + 68;
    int cardH = 196;
    int cardW = 260;
    int gap = 16;
    int cardStartX = modalX + 24;

    // --- CARD 1: 筋トレ ---
    int c1X = cardStartX;
    SoftUI::DrawSoftCard(c1X, cardY, cardW, cardH, 14, GetColor(24, 30, 46), 235, GetColor(100, 245, 140), 1);
    DrawStringToHandle(c1X + 14, cardY + 8, "① 筋トレ (パワーUP)", GetColor(100, 245, 140), font16);

    DrawStringToHandle(c1X + 14, cardY + 46, "・[SPACE] 長押し ➔ 緑で離す", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c1X + 14, cardY + 74, "・成功: 巨大化＆スピードUP!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c1X + 14, cardY + 106, "・失敗: 5秒間 筋肉痛 (停止)", GetColor(255, 110, 110), font13);
    DrawStringToHandle(c1X + 14, cardY + 134, "・放置: 15秒ごとに -3 レベル", GetColor(255, 160, 100), font13);

    // --- CARD 2: 必殺技 ---
    int c2X = cardStartX + cardW + gap;
    SoftUI::DrawSoftCard(c2X, cardY, cardW, cardH, 14, GetColor(24, 30, 46), 235, GetColor(255, 150, 40), 1);
    DrawStringToHandle(c2X + 14, cardY + 8, "② アクション (Lv.4~)", GetColor(255, 150, 40), font16);

    DrawStringToHandle(c2X + 14, cardY + 46, "・[右クリック / E] スタン攻撃", GetColor(255, 150, 40), font13);
    DrawStringToHandle(c2X + 26, cardY + 68, "➔ 突進して気絶! (Lv.4以上)", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c2X + 14, cardY + 104, "・[左クリック / SHIFT] 飛びつき", GetColor(255, 90, 90), font13);
    DrawStringToHandle(c2X + 26, cardY + 126, "➔ 前方へ跳躍捕獲! (Lv.4以上)", GetColor(255, 255, 255), font13);

    // --- CARD 3: 目標 ---
    int c3X = cardStartX + (cardW + gap) * 2;
    SoftUI::DrawSoftCard(c3X, cardY, cardW, cardH, 14, GetColor(24, 30, 46), 235, GetColor(80, 210, 255), 1);
    DrawStringToHandle(c3X + 14, cardY + 8, "③ 目標 (ネズミ全滅)", GetColor(80, 210, 255), font16);

    DrawStringToHandle(c3X + 14, cardY + 46, "・部屋のネズミを全員捕獲!", GetColor(255, 255, 255), font13);
    DrawStringToHandle(c3X + 14, cardY + 74, "・気絶したネズミは捕まえやすい!", GetColor(100, 245, 140), font13);
    DrawStringToHandle(c3X + 14, cardY + 106, "・数が減るとネズミがスピードUP!", GetColor(255, 150, 150), font13);
    DrawStringToHandle(c3X + 14, cardY + 134, "・早いクリアで高ランク獲得!", GetColor(255, 215, 40), font13);

    // 4. 下段：操作一覧（ふわっとした角丸カード）
    int keyTableY = modalY + 278;
    int keyTableW = 812;
    int keyTableH = 180;
    int keyTableX = modalX + 24;

    SoftUI::DrawSoftCard(keyTableX, keyTableY, keyTableW, keyTableH, 14, GetColor(20, 25, 38), 235, GetColor(255, 130, 40), 1);
    DrawStringToHandle(keyTableX + 16, keyTableY + 8, "🎮 操作方法", GetColor(255, 220, 60), font16);

    // 2列レイアウト
    int col1X = keyTableX + 24;
    int col2X = keyTableX + 430;
    int row1Y = keyTableY + 42;
    int rowStep = 42;

    // 左列: アクション
    DrawStringToHandle(col1X, row1Y + rowStep * 0, "[SPACE] 筋トレ (長押し)", GetColor(100, 245, 140), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 0 + 20, "➔ 長押しして緑ゾーンで離す", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 1, "[右クリック / E] スタン攻撃 (Lv.4~)", GetColor(255, 150, 40), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 1 + 20, "➔ 突進して気絶 (4 Rep以上で解放)", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col1X, row1Y + rowStep * 2, "[左クリック / SHIFT] 飛びつき (Lv.4~)", GetColor(255, 90, 90), font16);
    DrawStringToHandle(col1X + 18, row1Y + rowStep * 2 + 20, "➔ 跳躍して即捕獲 (4 Rep以上で解放)", GetColor(210, 225, 245), font13);

    // 右列: 移動・カメラ
    DrawStringToHandle(col2X, row1Y + rowStep * 0, "移動: [W] [A] [S] [D]", GetColor(255, 255, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 0 + 20, "➔ キャラクターの前後左右移動", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 1, "カメラ回転: マウス移動 / 矢印 [←→↑↓]", GetColor(100, 220, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 1 + 20, "➔ 感度調整: [- / +] または ポーズ画面(ESC)", GetColor(210, 225, 245), font13);

    DrawStringToHandle(col2X, row1Y + rowStep * 2, "視点リセット: [F]  |  一時停止: [ESC]", GetColor(180, 210, 255), font16);
    DrawStringToHandle(col2X + 18, row1Y + rowStep * 2 + 20, "➔ カメラを猫背後に戻す / ポーズ開閉", GetColor(210, 225, 245), font13);

    // 5. 閉じるボタン（角丸カプセル）
    int closeBtnW = 140;
    int closeBtnH = 38;
    int closeBtnX = modalX + modalW - 164;
    int closeBtnY = modalY + modalH - 52;

    int mx, my;
    GetMousePoint(&mx, &my);
    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                       my >= closeBtnY && my <= closeBtnY + closeBtnH);

    unsigned int closeBg = hoverClose ? GetColor(255, 75, 75) : GetColor(210, 45, 45);
    unsigned int closeBorder = hoverClose ? GetColor(255, 200, 200) : GetColor(240, 100, 100);
    SoftUI::DrawSoftCard(closeBtnX, closeBtnY, closeBtnW, closeBtnH, 19, closeBg, 245, closeBorder, 1, hoverClose, GetColor(255, 80, 80));
    DrawStringToHandle(closeBtnX + 22, closeBtnY + 10, "閉じる (ESC)", GetColor(255, 255, 255), font16);
}

// ============================================================================
// ポーズ中の更新処理（ESC / リトライ / タイトルに戻る / マウス感度調整）
// ============================================================================
void GameManager::UpdatePause(bool triggerEsc)
{
    // マウス入力
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
    m_prevMouseLeft = isLeftClick;

    // キー入力
    bool keyUp    = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool keyDown  = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_MINUS) != 0 || CheckHitKey(KEY_INPUT_SUBTRACT) != 0 || CheckHitKey(KEY_INPUT_LBRACKET) != 0);
    bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0 || CheckHitKey(KEY_INPUT_ADD) != 0 || CheckHitKey(KEY_INPUT_RBRACKET) != 0 || CheckHitKey(KEY_INPUT_SEMICOLON) != 0 || CheckHitKey(KEY_INPUT_COLON) != 0);
    bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
    bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);
    bool keyR     = (CheckHitKey(KEY_INPUT_R) != 0);
    bool keyT     = (CheckHitKey(KEY_INPUT_T) != 0);

    bool triggerUp    = keyUp && !m_prevKeyUp;
    bool triggerDown  = keyDown && !m_prevKeyDown;
    bool triggerLeft  = keyLeft && !m_prevKeyLeft;
    bool triggerRight = keyRight && !m_prevKeyRight;
    bool triggerEnter = isEnter && !m_prevKeyEnter;
    bool triggerSpace = isSpace && !m_prevKeySpace;
    bool triggerR     = keyR && !m_prevKeyR;
    bool triggerT     = keyT && !m_prevKeyT;

    m_prevKeyUp    = keyUp;
    m_prevKeyDown  = keyDown;
    m_prevKeyLeft  = keyLeft;
    m_prevKeyRight = keyRight;
    m_prevKeyEnter = isEnter;
    m_prevKeySpace = isSpace;
    m_prevKeyR     = keyR;
    m_prevKeyT     = keyT;

    // ESCキーでポーズ解除（ゲーム再開）
    if (triggerEsc)
    {
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
        SoundManager::GetInstance().ResumeBGM();
        m_startCount += (GetNowCount() - m_pauseStartTime);
        m_isPaused = false;
        m_camera.ResetMouseToCenter();
        SetMouseDispFlag(FALSE);
        return;
    }

    // 上下キーで選択切り替え (0: リトライ, 1: タイトル, 2: マウス感度, 3: BGM音量, 4: SE音量)
    if (triggerUp)
    {
        m_pauseMenuItem = (m_pauseMenuItem + 4) % 5;
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (triggerDown)
    {
        m_pauseMenuItem = (m_pauseMenuItem + 1) % 5;
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }

    // モーダルレイアウト寸法
    int modalW = 500;
    int modalH = 550;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    int btnW = 380;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;

    int retryBtnY = modalY + 58;
    int retryBtnH = 44;

    int titleBtnY = modalY + 108;
    int titleBtnH = 44;

    int sensRowY  = modalY + 158;
    int sensRowH  = 66;

    int bgmRowY   = modalY + 230;
    int bgmRowH   = 66;

    int seRowY    = modalY + 302;
    int seRowH    = 66;

    int resumeBtnW = 280;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 376;

    int minusBtnX  = btnX + 10;
    int minusBtnW  = 46;
    int minusBtnH  = 26;
    int plusBtnX   = btnX + btnW - 56;
    int plusBtnW   = 46;
    int plusBtnH   = 26;
    int sliderBarX = btnX + 64;
    int sliderBarW = btnW - 128;
    int sliderBarH = 12;

    // ホバー判定
    bool hoverRetry  = (mx >= btnX && mx <= btnX + btnW && my >= retryBtnY && my <= retryBtnY + retryBtnH);
    bool hoverTitle  = (mx >= btnX && mx <= btnX + btnW && my >= titleBtnY && my <= titleBtnY + titleBtnH);
    bool hoverSens   = (mx >= btnX && mx <= btnX + btnW && my >= sensRowY && my <= sensRowY + sensRowH);
    bool hoverBgm    = (mx >= btnX && mx <= btnX + btnW && my >= bgmRowY && my <= bgmRowY + bgmRowH);
    bool hoverSe     = (mx >= btnX && mx <= btnX + btnW && my >= seRowY && my <= seRowY + seRowH);
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW && my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    if (hoverRetry) m_pauseMenuItem = 0;
    if (hoverTitle) m_pauseMenuItem = 1;
    if (hoverSens)  m_pauseMenuItem = 2;
    if (hoverBgm)   m_pauseMenuItem = 3;
    if (hoverSe)    m_pauseMenuItem = 4;

    // 1. リトライ判定（ボタンクリック / 選択中Enter/Space / Rキー）
    if ((hoverRetry && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_pauseMenuItem == 0) || triggerR)
    {
        SoundManager::GetInstance().PlaySE(SeType::Decide);
        m_isPaused = false;
        StartGame();
        return;
    }

    // 2. タイトルに戻る判定（ボタンクリック / 選択中Enter/Space / Tキー）
    if ((hoverTitle && isMouseTrigger) || ((triggerEnter || triggerSpace) && m_pauseMenuItem == 1) || triggerT)
    {
        SoundManager::GetInstance().PlaySE(SeType::Decide);
        m_isPaused = false;
        SetupTitle();
        return;
    }

    // 3. マウス感度操作 (Item 2)
    int sensMinusY = sensRowY + 33;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);
    bool hoverSensBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= sensRowY + 36 && my <= sensRowY + 54);

    if (m_pauseMenuItem == 2 && triggerLeft)
    {
        m_camera.AdjustMouseSensitivity(-0.1f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f);
    }
    if (m_pauseMenuItem == 2 && triggerRight)
    {
        m_camera.AdjustMouseSensitivity(+0.1f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f);
    }
    if (hoverSensMinus && isMouseTrigger)
    {
        m_camera.AdjustMouseSensitivity(-0.1f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f);
    }
    if (hoverSensPlus && isMouseTrigger)
    {
        m_camera.AdjustMouseSensitivity(+0.1f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f);
    }
    if (hoverSensBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        m_camera.SetMouseSensitivity(0.2f + MathHelper::Clamp(ratio, 0.0f, 1.0f) * (3.0f - 0.2f));
    }
    if ((triggerEnter || triggerSpace) && m_pauseMenuItem == 2 && !hoverSensMinus && !hoverSensPlus && !hoverSensBar)
    {
        m_camera.SetMouseSensitivity(1.0f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f);
    }

    // 4. BGM音量操作 (Item 3)
    int bgmMinusY = bgmRowY + 33;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);
    bool hoverBgmBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= bgmRowY + 36 && my <= bgmRowY + 54);

    if (m_pauseMenuItem == 3 && triggerLeft)
    {
        SoundManager::GetInstance().AdjustBGMVolume(-0.05f);
    }
    if (m_pauseMenuItem == 3 && triggerRight)
    {
        SoundManager::GetInstance().AdjustBGMVolume(+0.05f);
    }
    if (hoverBgmMinus && isMouseTrigger)
    {
        SoundManager::GetInstance().AdjustBGMVolume(-0.05f);
    }
    if (hoverBgmPlus && isMouseTrigger)
    {
        SoundManager::GetInstance().AdjustBGMVolume(+0.05f);
    }
    if (hoverBgmBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        SoundManager::GetInstance().SetBGMVolume(MathHelper::Clamp(ratio, 0.0f, 1.0f));
        SoundManager::GetInstance().SaveSettings();
    }

    // 5. SE音量操作 (Item 4)
    int seMinusY = seRowY + 33;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);
    bool hoverSeBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= seRowY + 36 && my <= seRowY + 54);

    if (m_pauseMenuItem == 4 && triggerLeft)
    {
        SoundManager::GetInstance().AdjustSEVolume(-0.05f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (m_pauseMenuItem == 4 && triggerRight)
    {
        SoundManager::GetInstance().AdjustSEVolume(+0.05f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (hoverSeMinus && isMouseTrigger)
    {
        SoundManager::GetInstance().AdjustSEVolume(-0.05f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (hoverSePlus && isMouseTrigger)
    {
        SoundManager::GetInstance().AdjustSEVolume(+0.05f);
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (hoverSeBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        SoundManager::GetInstance().SetSEVolume(MathHelper::Clamp(ratio, 0.0f, 1.0f));
        SoundManager::GetInstance().SaveSettings();
        if (isMouseTrigger)
        {
            SoundManager::GetInstance().PlaySE(SeType::Cursor);
        }
    }

    // 6. ゲームに戻る判定（再開ボタンクリック）
    if (hoverResume && isMouseTrigger)
    {
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
        SoundManager::GetInstance().ResumeBGM();
        m_startCount += (GetNowCount() - m_pauseStartTime);
        m_isPaused = false;
        m_camera.ResetMouseToCenter();
        SetMouseDispFlag(FALSE);
        return;
    }
}

void GameManager::DrawPauseModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 195);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダル本体
    int modalW = 500;
    int modalH = 550;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 22, GetColor(20, 24, 38), 240, GetColor(255, 125, 35), 2);

    // ヘッダーバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 48, 20, 20, GetColor(32, 42, 68), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 20, modalY + 48, modalX + modalW - 20, modalY + 48, GetColor(255, 125, 35));

    const char* pauseHeader = "⏸ PAUSE / 設定";
    int hW = GetDrawStringWidthToHandle(pauseHeader, static_cast<int>(strlen(pauseHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 11, pauseHeader, GetColor(255, 220, 60), font24);

    int btnW = 380;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;

    int retryBtnY = modalY + 58;
    int retryBtnH = 44;
    int titleBtnY = modalY + 108;
    int titleBtnH = 44;
    int sensRowY  = modalY + 158;
    int sensRowH  = 66;
    int bgmRowY   = modalY + 230;
    int bgmRowH   = 66;
    int seRowY    = modalY + 302;
    int seRowH    = 66;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: リトライ ---
    bool isRetrySelected = (m_pauseMenuItem == 0);
    if (isRetrySelected)
    {
        unsigned int col = GetColor(100, 230 + static_cast<int>(25 * btnPulse), 120);
        SoftUI::DrawSoftCard(btnX, retryBtnY, btnW, retryBtnH, 22, col, 255, GetColor(160, 255, 180), 2, true, GetColor(100, 240, 130));
        DrawStringToHandle(btnX + 22, retryBtnY + 9, "🔄", GetColor(20, 20, 20), font24);
        const char* retryTxt = "リトライ (R)";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 10, retryTxt, GetColor(15, 25, 20), font24);
    }
    else
    {
        SoftUI::DrawSoftCard(btnX, retryBtnY, btnW, retryBtnH, 22, GetColor(32, 42, 65), 235, GetColor(60, 75, 110), 1);
        DrawStringToHandle(btnX + 22, retryBtnY + 9, "🔄", GetColor(140, 170, 210), font24);
        const char* retryTxt = "リトライ";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 10, retryTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: タイトルに戻る ---
    bool isTitleSelected = (m_pauseMenuItem == 1);
    if (isTitleSelected)
    {
        unsigned int col = GetColor(70, 190 + static_cast<int>(35 * btnPulse), 255);
        SoftUI::DrawSoftCard(btnX, titleBtnY, btnW, titleBtnH, 22, col, 255, GetColor(160, 230, 255), 2, true, GetColor(80, 200, 255));
        DrawStringToHandle(btnX + 22, titleBtnY + 9, "🏠", GetColor(20, 20, 20), font24);
        const char* titleTxt = "タイトルに戻る (T)";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 10, titleTxt, GetColor(15, 20, 30), font24);
    }
    else
    {
        SoftUI::DrawSoftCard(btnX, titleBtnY, btnW, titleBtnH, 22, GetColor(32, 42, 65), 235, GetColor(60, 75, 110), 1);
        DrawStringToHandle(btnX + 22, titleBtnY + 9, "🏠", GetColor(140, 170, 210), font24);
        const char* titleTxt = "タイトルに戻る";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 10, titleTxt, GetColor(210, 230, 255), font24);
    }

    int mx, my;
    GetMousePoint(&mx, &my);
    int minusBtnX  = btnX + 10;
    int minusBtnW  = 46;
    int minusBtnH  = 26;
    int plusBtnX   = btnX + btnW - 56;
    int plusBtnW   = 46;
    int plusBtnH   = 26;

    // --- コントロール3: マウス感度 ---
    bool isSensSelected = (m_pauseMenuItem == 2);
    float currentSens = m_camera.GetMouseSensitivity();
    char sensValStr[32];
    snprintf(sensValStr, sizeof(sensValStr), "%.1fx", currentSens);
    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    int sensMinusY = sensRowY + 33;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, sensRowY, btnW, sensRowH,
        isSensSelected, "🎯 マウス感度", sensValStr, sensRatio,
        (currentSens >= 2.0f) ? GetColor(255, 120, 60) : ((currentSens >= 1.0f) ? GetColor(100, 220, 255) : GetColor(70, 180, 240)),
        hoverSensMinus, hoverSensPlus, font16, font13
    );

    // --- コントロール4: BGM音量 ---
    bool isBgmSelected = (m_pauseMenuItem == 3);
    int bgmPercent = SoundManager::GetInstance().GetBGMVolumePercent();
    char bgmValStr[32];
    snprintf(bgmValStr, sizeof(bgmValStr), "%d%%", bgmPercent);
    float bgmRatio = SoundManager::GetInstance().GetBGMVolume();
    int bgmMinusY = bgmRowY + 33;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, bgmRowY, btnW, bgmRowH,
        isBgmSelected, "🎵 BGM音量", bgmValStr, bgmRatio,
        GetColor(255, 170, 50),
        hoverBgmMinus, hoverBgmPlus, font16, font13
    );

    // --- コントロール5: SE音量 ---
    bool isSeSelected = (m_pauseMenuItem == 4);
    int sePercent = SoundManager::GetInstance().GetSEVolumePercent();
    char seValStr[32];
    snprintf(seValStr, sizeof(seValStr), "%d%%", sePercent);
    float seRatio = SoundManager::GetInstance().GetSEVolume();
    int seMinusY = seRowY + 33;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, seRowY, btnW, seRowH,
        isSeSelected, "🔊 SE音量 (効果音)", seValStr, seRatio,
        GetColor(80, 220, 120),
        hoverSeMinus, hoverSePlus, font16, font13
    );

    // --- 下部: ゲームに戻る (ESC) ボタン ---
    int resumeBtnW = 280;
    int resumeBtnH = 36;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 376;
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW && my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    SoftUI::DrawSoftCard(resumeBtnX, resumeBtnY, resumeBtnW, resumeBtnH, 18,
                         hoverResume ? GetColor(45, 60, 90) : GetColor(24, 30, 46), 235,
                         hoverResume ? GetColor(100, 220, 255) : GetColor(60, 75, 110), 1,
                         hoverResume, GetColor(80, 200, 255));

    const char* resumeTxt = "▶ ゲームに戻る (ESC)";
    int rW = GetDrawStringWidthToHandle(resumeTxt, static_cast<int>(strlen(resumeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - rW) / 2, resumeBtnY + 9, resumeTxt, hoverResume ? GetColor(255, 255, 255) : GetColor(180, 205, 235), font16);

    // 操作説明ガイド
    const char* guideTxt1 = "選択: [↑/↓]  感度・音量変更: [←/→] または クリック/ドラッグ";
    int gW1 = GetDrawStringWidthToHandle(guideTxt1, static_cast<int>(strlen(guideTxt1)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW1) / 2, modalY + 430, guideTxt1, GetColor(160, 185, 220), font13);

    const char* guideTxt2 = "決定: [ENTER] / [SPACE]  |  閉じる: [ESC]";
    int gW2 = GetDrawStringWidthToHandle(guideTxt2, static_cast<int>(strlen(guideTxt2)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW2) / 2, modalY + 454, guideTxt2, GetColor(130, 150, 180), font13);
}

// ============================================================================
// タイトル画面：サウンド設定モーダル更新処理
// ============================================================================
void GameManager::UpdateTitleSoundSettings()
{
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    bool isLeftClick = ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0);
    bool isMouseTrigger = isLeftClick && !m_prevMouseLeft;
    m_prevMouseLeft = isLeftClick;

    bool keyEsc   = (CheckHitKey(KEY_INPUT_ESCAPE) != 0);
    bool keyUp    = (CheckHitKey(KEY_INPUT_UP) != 0 || CheckHitKey(KEY_INPUT_W) != 0);
    bool keyDown  = (CheckHitKey(KEY_INPUT_DOWN) != 0 || CheckHitKey(KEY_INPUT_S) != 0);
    bool keyLeft  = (CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_MINUS) != 0 || CheckHitKey(KEY_INPUT_SUBTRACT) != 0);
    bool keyRight = (CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0 || CheckHitKey(KEY_INPUT_ADD) != 0);
    bool isEnter  = (CheckHitKey(KEY_INPUT_RETURN) != 0);
    bool isSpace  = (CheckHitKey(KEY_INPUT_SPACE) != 0);

    bool triggerEsc   = keyEsc && !m_prevKeyEsc;
    bool triggerUp    = keyUp && !m_prevKeyUp;
    bool triggerDown  = keyDown && !m_prevKeyDown;
    bool triggerLeft  = keyLeft && !m_prevKeyLeft;
    bool triggerRight = keyRight && !m_prevKeyRight;

    m_prevKeyEsc   = keyEsc;
    m_prevKeyUp    = keyUp;
    m_prevKeyDown  = keyDown;
    m_prevKeyLeft  = keyLeft;
    m_prevKeyRight = keyRight;
    m_prevKeyEnter = isEnter;
    m_prevKeySpace = isSpace;

    int modalW = 500;
    int modalH = 430;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    int closeBtnW = 160;
    int closeBtnH = 38;
    int closeBtnX = (Config::SCREEN_WIDTH - closeBtnW) / 2;
    int closeBtnY = modalY + modalH - 52;

    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW && my >= closeBtnY && my <= closeBtnY + closeBtnH);

    // ESC または 閉じるボタンクリックで閉じる
    if (triggerEsc || (hoverClose && isMouseTrigger))
    {
        m_showSoundSettings = false;
        SoundManager::GetInstance().PlaySE(SeType::Cancel);
        return;
    }

    // 上下キーで項目移動 (0: BGM, 1: SE, 2: 感度)
    if (triggerUp)
    {
        m_titleSoundSettingItem = (m_titleSoundSettingItem + 2) % 3;
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }
    if (triggerDown)
    {
        m_titleSoundSettingItem = (m_titleSoundSettingItem + 1) % 3;
        SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }

    int btnW = 380;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int bgmRowY  = modalY + 68;
    int bgmRowH  = 66;
    int seRowY   = modalY + 144;
    int seRowH   = 66;
    int sensRowY = modalY + 220;
    int sensRowH = 66;

    int minusBtnX  = btnX + 10;
    int minusBtnW  = 46;
    int minusBtnH  = 26;
    int plusBtnX   = btnX + btnW - 56;
    int plusBtnW   = 46;
    int plusBtnH   = 26;
    int sliderBarX = btnX + 64;
    int sliderBarW = btnW - 128;

    bool hoverBgm  = (mx >= btnX && mx <= btnX + btnW && my >= bgmRowY && my <= bgmRowY + bgmRowH);
    bool hoverSe   = (mx >= btnX && mx <= btnX + btnW && my >= seRowY && my <= seRowY + seRowH);
    bool hoverSens = (mx >= btnX && mx <= btnX + btnW && my >= sensRowY && my <= sensRowY + sensRowH);

    if (hoverBgm)  m_titleSoundSettingItem = 0;
    if (hoverSe)   m_titleSoundSettingItem = 1;
    if (hoverSens) m_titleSoundSettingItem = 2;

    // 1. BGM音量操作
    int bgmMinusY = bgmRowY + 33;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);
    bool hoverBgmBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= bgmRowY + 36 && my <= bgmRowY + 54);

    if (m_titleSoundSettingItem == 0 && triggerLeft)  SoundManager::GetInstance().AdjustBGMVolume(-0.05f);
    if (m_titleSoundSettingItem == 0 && triggerRight) SoundManager::GetInstance().AdjustBGMVolume(+0.05f);
    if (hoverBgmMinus && isMouseTrigger) SoundManager::GetInstance().AdjustBGMVolume(-0.05f);
    if (hoverBgmPlus && isMouseTrigger)  SoundManager::GetInstance().AdjustBGMVolume(+0.05f);
    if (hoverBgmBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        SoundManager::GetInstance().SetBGMVolume(MathHelper::Clamp(ratio, 0.0f, 1.0f));
        SoundManager::GetInstance().SaveSettings();
    }

    // 2. SE音量操作
    int seMinusY = seRowY + 33;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);
    bool hoverSeBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= seRowY + 36 && my <= seRowY + 54);

    if (m_titleSoundSettingItem == 1 && triggerLeft)  { SoundManager::GetInstance().AdjustSEVolume(-0.05f); SoundManager::GetInstance().PlaySE(SeType::Cursor); }
    if (m_titleSoundSettingItem == 1 && triggerRight) { SoundManager::GetInstance().AdjustSEVolume(+0.05f); SoundManager::GetInstance().PlaySE(SeType::Cursor); }
    if (hoverSeMinus && isMouseTrigger) { SoundManager::GetInstance().AdjustSEVolume(-0.05f); SoundManager::GetInstance().PlaySE(SeType::Cursor); }
    if (hoverSePlus && isMouseTrigger)  { SoundManager::GetInstance().AdjustSEVolume(+0.05f); SoundManager::GetInstance().PlaySE(SeType::Cursor); }
    if (hoverSeBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        SoundManager::GetInstance().SetSEVolume(MathHelper::Clamp(ratio, 0.0f, 1.0f));
        SoundManager::GetInstance().SaveSettings();
        if (isMouseTrigger) SoundManager::GetInstance().PlaySE(SeType::Cursor);
    }

    // 3. マウス感度操作
    int sensMinusY = sensRowY + 33;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);
    bool hoverSensBar   = (mx >= sliderBarX - 4 && mx <= sliderBarX + sliderBarW + 4 && my >= sensRowY + 36 && my <= sensRowY + 54);

    if (m_titleSoundSettingItem == 2 && triggerLeft)  { m_camera.AdjustMouseSensitivity(-0.1f); SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f); }
    if (m_titleSoundSettingItem == 2 && triggerRight) { m_camera.AdjustMouseSensitivity(+0.1f); SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f); }
    if (hoverSensMinus && isMouseTrigger) { m_camera.AdjustMouseSensitivity(-0.1f); SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f); }
    if (hoverSensPlus && isMouseTrigger)  { m_camera.AdjustMouseSensitivity(+0.1f); SoundManager::GetInstance().PlaySE(SeType::Cursor, 0.7f); }
    if (hoverSensBar && isLeftClick)
    {
        float ratio = static_cast<float>(mx - sliderBarX) / static_cast<float>(sliderBarW);
        m_camera.SetMouseSensitivity(0.2f + MathHelper::Clamp(ratio, 0.0f, 1.0f) * (3.0f - 0.2f));
    }
}

// ============================================================================
// タイトル画面：サウンド設定モーダル描画
// ============================================================================
void GameManager::DrawSoundSettingsModal()
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font24 = fm.GetFont24();

    // 1. 全画面暗転レイヤー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダル本体
    int modalW = 500;
    int modalH = 430;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 22, GetColor(20, 24, 38), 245, GetColor(80, 220, 100), 2);

    // ヘッダーバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 48, 20, 20, GetColor(32, 48, 68), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 20, modalY + 48, modalX + modalW - 20, modalY + 48, GetColor(80, 220, 100));

    const char* headerTxt = "🔊 サウンド＆操作設定";
    int hW = GetDrawStringWidthToHandle(headerTxt, static_cast<int>(strlen(headerTxt)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 11, headerTxt, GetColor(255, 230, 90), font24);

    int btnW = 380;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int bgmRowY  = modalY + 68;
    int bgmRowH  = 66;
    int seRowY   = modalY + 144;
    int seRowH   = 66;
    int sensRowY = modalY + 220;
    int sensRowH = 66;

    int mx, my;
    GetMousePoint(&mx, &my);
    int minusBtnX  = btnX + 10;
    int minusBtnW  = 46;
    int minusBtnH  = 26;
    int plusBtnX   = btnX + btnW - 56;
    int plusBtnW   = 46;
    int plusBtnH   = 26;

    // --- 1. BGM音量 ---
    bool isBgmSelected = (m_titleSoundSettingItem == 0);
    int bgmPercent = SoundManager::GetInstance().GetBGMVolumePercent();
    char bgmValStr[32];
    snprintf(bgmValStr, sizeof(bgmValStr), "%d%%", bgmPercent);
    int bgmMinusY = bgmRowY + 33;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, bgmRowY, btnW, bgmRowH,
        isBgmSelected, "🎵 BGM音量", bgmValStr, SoundManager::GetInstance().GetBGMVolume(),
        GetColor(255, 170, 50),
        hoverBgmMinus, hoverBgmPlus, font16, font13
    );

    // --- 2. SE音量 ---
    bool isSeSelected = (m_titleSoundSettingItem == 1);
    int sePercent = SoundManager::GetInstance().GetSEVolumePercent();
    char seValStr[32];
    snprintf(seValStr, sizeof(seValStr), "%d%%", sePercent);
    int seMinusY = seRowY + 33;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, seRowY, btnW, seRowH,
        isSeSelected, "🔊 SE音量 (効果音)", seValStr, SoundManager::GetInstance().GetSEVolume(),
        GetColor(80, 220, 120),
        hoverSeMinus, hoverSePlus, font16, font13
    );

    // --- 3. マウス感度 ---
    bool isSensSelected = (m_titleSoundSettingItem == 2);
    float currentSens = m_camera.GetMouseSensitivity();
    char sensValStr[32];
    snprintf(sensValStr, sizeof(sensValStr), "%.1fx", currentSens);
    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    int sensMinusY = sensRowY + 33;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, sensRowY, btnW, sensRowH,
        isSensSelected, "🎯 マウス感度", sensValStr, sensRatio,
        (currentSens >= 2.0f) ? GetColor(255, 120, 60) : ((currentSens >= 1.0f) ? GetColor(100, 220, 255) : GetColor(70, 180, 240)),
        hoverSensMinus, hoverSensPlus, font16, font13
    );

    // 閉じるボタン
    int closeBtnW = 160;
    int closeBtnH = 38;
    int closeBtnX = (Config::SCREEN_WIDTH - closeBtnW) / 2;
    int closeBtnY = modalY + modalH - 52;
    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW && my >= closeBtnY && my <= closeBtnY + closeBtnH);

    SoftUI::DrawSoftCard(closeBtnX, closeBtnY, closeBtnW, closeBtnH, 19,
                         hoverClose ? GetColor(45, 60, 90) : GetColor(28, 36, 56), 235,
                         hoverClose ? GetColor(100, 220, 255) : GetColor(70, 85, 120), 1,
                         hoverClose, GetColor(80, 200, 255));
    DrawStringToHandle(closeBtnX + 24, closeBtnY + 10, "閉じる (ESC)", hoverClose ? GetColor(255, 255, 255) : GetColor(200, 225, 255), font16);

    const char* hintTxt = "項目選択: [↑/↓]  音量変更: [←/→] または クリック/ドラッグ";
    int hintW = GetDrawStringWidthToHandle(hintTxt, static_cast<int>(strlen(hintTxt)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - hintW) / 2, modalY + 312, hintTxt, GetColor(160, 185, 220), font13);
}
void GameManager::DrawDebug3DOverlay()
{
    auto player = m_objManager.GetPlayer();
    if (!player)
    {
        return;
    }

    // 3Dデバッグ辺（DrawCapsule3D）がステージ照明の影で暗くならないようライティングを一時OFFにして鮮やかな色で描画
    SetUseLighting(FALSE);

    const auto& stage = m_objManager.GetStage();
    const VECTOR pPos = player->GetPos();
    const float unitPerMeter = Config::WORLD_UNITS_PER_METER; // 58.0533f (猫身長1.8m = 104.496u 基準)
    const float cubeSize = Config::CUBE_10M_SIZE_UNITS;       // 580.5333f (10m)
    const float halfCube = cubeSize * 0.5f;                   // 290.2667f (5m)

    // 白いステージ床の上でもクッキリ視認できる3Dソリッド辺描画ヘルパー (DrawCapsule3D)
    auto drawBeam3D = [](VECTOR a, VECTOR b, float radius, unsigned int col)
    {
        DrawCapsule3D(a, b, radius, 4, col, col, TRUE);
    };

    // 3D水平サークル描画ヘルパー
    auto drawCircle3D = [](const VECTOR& center, float radius, float yOffset, unsigned int col, int segments = 32)
    {
        const float step = (DX_PI_F * 2.0f) / static_cast<float>(segments);
        for (int i = 0; i < segments; ++i)
        {
            float a0 = step * static_cast<float>(i);
            float a1 = step * static_cast<float>(i + 1);
            VECTOR p0 = VGet(center.x + std::cos(a0) * radius, center.y + yOffset, center.z + std::sin(a0) * radius);
            VECTOR p1 = VGet(center.x + std::cos(a1) * radius, center.y + yOffset, center.z + std::sin(a1) * radius);
            DrawLine3D(p0, p1, col);
        }
    };

    // =========================================================================
    // 1. ステージ全体の10mグリッド目盛り（モード0, 1, 2のとき床面に10m間隔ラインを描画）
    // =========================================================================
    if (m_debugCubeMode != 3)
    {
        VECTOR minB = stage.GetMinBounds();
        VECTOR maxB = stage.GetMaxBounds();
        float floorY = pPos.y + 1.5f;

        unsigned int stageGridCol = GetColor(70, 130, 180);
        unsigned int stageAxisCol = GetColor(20, 90, 210);

        int minIdxX = static_cast<int>(std::floor(minB.x / cubeSize));
        int maxIdxX = static_cast<int>(std::ceil(maxB.x / cubeSize));
        int minIdxZ = static_cast<int>(std::floor(minB.z / cubeSize));
        int maxIdxZ = static_cast<int>(std::ceil(maxB.z / cubeSize));

        for (int ix = minIdxX; ix <= maxIdxX; ++ix)
        {
            float x = static_cast<float>(ix) * cubeSize;
            if (x < minB.x || x > maxB.x) continue;
            unsigned int c = (ix == 0) ? stageAxisCol : stageGridCol;
            DrawLine3D(VGet(x, floorY, minB.z), VGet(x, floorY, maxB.z), c);
        }
        for (int iz = minIdxZ; iz <= maxIdxZ; ++iz)
        {
            float z = static_cast<float>(iz) * cubeSize;
            if (z < minB.z || z > maxB.z) continue;
            unsigned int c = (iz == 0) ? stageAxisCol : stageGridCol;
            DrawLine3D(VGet(minB.x, floorY, z), VGet(maxB.x, floorY, z), c);
        }
    }

    // =========================================================================
    // 2. 10m × 10m × 10m の■（ワイヤーフレーム立方体）を12本の辺で再現（距離目測用）
    //    猫の基本身長 104.496u = 1.8m としたとき、1m = 58.053u、10m = 580.533u
    // =========================================================================
    if (m_debugCubeMode != 3)
    {
        VECTOR cubeCenter = pPos;
        if (m_debugCubeMode == 1)
        {
            // モード1: カメラ前方へ10m伸びる配置（通常カメラでも足元0m〜前方10mの奥行き全体が一目で見える）
            float yaw = m_camera.GetAngleH();
            cubeCenter = VGet(pPos.x + std::sin(yaw) * halfCube, pPos.y, pPos.z + std::cos(yaw) * halfCube);
        }
        else if (m_debugCubeMode == 2)
        {
            // モード2: ステージ原点(0,0,0)中心
            cubeCenter = VGet(0.0f, pPos.y, 0.0f);
        }

        float baseY = cubeCenter.y + 2.2f; // 床と重ならない高さ
        float topY  = baseY + cubeSize;    // +10m (580.53u)
        float midY  = baseY + halfCube;    // +5m  (290.27u)

        float x0 = cubeCenter.x - halfCube;
        float x1 = cubeCenter.x + halfCube;
        float z0 = cubeCenter.z - halfCube;
        float z1 = cubeCenter.z + halfCube;

        // 底面4頂点 (0m) と 上面4頂点 (+10m)
        VECTOR b00 = VGet(x0, baseY, z0);
        VECTOR b10 = VGet(x1, baseY, z0);
        VECTOR b11 = VGet(x1, baseY, z1);
        VECTOR b01 = VGet(x0, baseY, z1);

        VECTOR t00 = VGet(x0, topY, z0);
        VECTOR t10 = VGet(x1, topY, z0);
        VECTOR t11 = VGet(x1, topY, z1);
        VECTOR t01 = VGet(x0, topY, z1);

        // 中間5m高さの4頂点 (+5m)
        VECTOR m00 = VGet(x0, midY, z0);
        VECTOR m10 = VGet(x1, midY, z0);
        VECTOR m11 = VGet(x1, midY, z1);
        VECTOR m01 = VGet(x0, midY, z1);

        // 底面1m刻みグリッド（10m×10mの内部を1m=58.05u間隔で分割して距離目測しやすくする）
        unsigned int grid1mCol = GetColor(30, 145, 175);
        unsigned int grid5mCol = GetColor(0, 110, 235);
        for (int i = 1; i < 10; ++i)
        {
            float offset = -halfCube + static_cast<float>(i) * unitPerMeter;
            VECTOR gx0 = VGet(x0, baseY, cubeCenter.z + offset);
            VECTOR gx1 = VGet(x1, baseY, cubeCenter.z + offset);
            VECTOR gz0 = VGet(cubeCenter.x + offset, baseY, z0);
            VECTOR gz1 = VGet(cubeCenter.x + offset, baseY, z1);

            if (i == 5)
            {
                // 中心5m十字ラインは太めのビームで強調
                drawBeam3D(gx0, gx1, 1.3f, grid5mCol);
                drawBeam3D(gz0, gz1, 1.3f, grid5mCol);
            }
            else
            {
                DrawLine3D(gx0, gx1, grid1mCol);
                DrawLine3D(gz0, gz1, grid1mCol);
            }
        }

        // 10m×10m×10m 立方体の12本のメイン辺をソリッド3Dビームで描画（白い床でも鮮明に見える高コントラスト色）
        unsigned int bottomEdgeCol = GetColor(0, 95, 255);    // 底面4辺(10m×10m): 鮮やかなエレクトリックブルー
        unsigned int pillarEdgeCol = GetColor(255, 55, 20);   // 垂直4辺(高さ10m): 鮮やかなクリムゾンレッド
        unsigned int topEdgeCol    = GetColor(0, 170, 240);   // 上面4辺(10m天井): ディープシアン
        unsigned int midBeltCol    = GetColor(235, 135, 0);   // 中間5mベルト: アンバーオレンジ

        const float mainEdgeR = 2.6f;
        const float subEdgeR  = 1.3f;

        // 底面4辺 (10m × 10m)
        drawBeam3D(b00, b10, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b10, b11, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b11, b01, mainEdgeR, bottomEdgeCol);
        drawBeam3D(b01, b00, mainEdgeR, bottomEdgeCol);

        // 上面4辺 (高さ10m)
        drawBeam3D(t00, t10, mainEdgeR, topEdgeCol);
        drawBeam3D(t10, t11, mainEdgeR, topEdgeCol);
        drawBeam3D(t11, t01, mainEdgeR, topEdgeCol);
        drawBeam3D(t01, t00, mainEdgeR, topEdgeCol);

        // 垂直の4柱 (0m -> 10m)
        drawBeam3D(b00, t00, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b10, t10, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b11, t11, mainEdgeR, pillarEdgeCol);
        drawBeam3D(b01, t01, mainEdgeR, pillarEdgeCol);

        // 高さ5mの中間ベルト
        drawBeam3D(m00, m10, subEdgeR, midBeltCol);
        drawBeam3D(m10, m11, subEdgeR, midBeltCol);
        drawBeam3D(m11, m01, subEdgeR, midBeltCol);
        drawBeam3D(m01, m00, subEdgeR, midBeltCol);

        // 垂直4柱に1mごとの高さ目盛りティック（1m〜9m）を入れる
        float tickLen = 16.0f;
        for (int m = 1; m < 10; ++m)
        {
            float ty = baseY + static_cast<float>(m) * unitPerMeter;
            float tLen = (m == 5) ? tickLen * 1.8f : tickLen;
            unsigned int tCol = (m == 5) ? midBeltCol : pillarEdgeCol;

            drawBeam3D(VGet(x0, ty, z0), VGet(x0 + tLen, ty, z0 + tLen), 1.0f, tCol);
            drawBeam3D(VGet(x1, ty, z0), VGet(x1 - tLen, ty, z0 + tLen), 1.0f, tCol);
            drawBeam3D(VGet(x1, ty, z1), VGet(x1 - tLen, ty, z1 - tLen), 1.0f, tCol);
            drawBeam3D(VGet(x0, ty, z1), VGet(x0 + tLen, ty, z1 - tLen), 1.0f, tCol);
        }
    }

    // =========================================================================
    // 3. 猫の横の「身長1.8m基準ルーラー（物差し）」と当たり判定・攻撃範囲リング
    // =========================================================================
    {
        float rulerX = pPos.x + player->GetBodyRadius() + 26.0f;
        float rulerZ = pPos.z;
        float baseH  = Config::CAT_BASE_HEIGHT_UNITS; // 104.496u (1.80m)
        float curH   = baseH * (player->GetCurrentHeightMeters() / Config::CAT_BASE_HEIGHT_METERS);

        unsigned int rulerCol = GetColor(215, 0, 160);
        drawBeam3D(VGet(rulerX, pPos.y, rulerZ), VGet(rulerX, pPos.y + baseH, rulerZ), 1.5f, rulerCol);

        // 0m, 0.5m, 1.0m, 1.5m, 1.8m の目盛り横線
        const float marksM[] = { 0.0f, 0.5f, 1.0f, 1.5f, 1.8f };
        for (float m : marksM)
        {
            float my = pPos.y + m * unitPerMeter;
            float w = (m == 1.8f || m == 0.0f) ? 16.0f : 9.0f;
            drawBeam3D(VGet(rulerX - w, my, rulerZ), VGet(rulerX + w, my, rulerZ), 1.0f, rulerCol);
        }

        // パンプアップで1.8mより大きくなっている場合は現在身長までオレンジ線を延長
        if (curH > baseH + 1.0f)
        {
            unsigned int pumpCol = GetColor(255, 120, 0);
            drawBeam3D(VGet(rulerX, pPos.y + baseH, rulerZ), VGet(rulerX, pPos.y + curH, rulerZ), 1.5f, pumpCol);
            drawBeam3D(VGet(rulerX - 18.0f, pPos.y + curH, rulerZ), VGet(rulerX + 18.0f, pPos.y + curH, rulerZ), 1.1f, pumpCol);
        }

        // プレイヤーの体当たり半径・飛びつき半径・タックル半径リング
        drawCircle3D(pPos, player->GetBodyRadius(), 2.5f, GetColor(0, 190, 90), 28);
        if (player->GetRepCount() >= 4)
        {
            drawCircle3D(pPos, player->GetPounceRadius(), 3.0f, GetColor(240, 40, 40), 32);
            drawCircle3D(pPos, player->GetTackleRadius(), 3.5f, GetColor(240, 120, 0), 36);
        }
    }

    // =========================================================================
    // 4. Position.csv のスポーン/巡回座標 (Pos1〜Pos8) の3Dピン表示
    // =========================================================================
    std::vector<VECTOR> csvPositions = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    {
        unsigned int posCol = GetColor(20, 190, 60);
        for (size_t i = 0; i < csvPositions.size(); ++i)
        {
            VECTOR wp = csvPositions[i];
            float gy = wp.y;
            stage.GetGroundHeight(VGet(wp.x, wp.y + 100.0f, wp.z), gy);

            VECTOR groundPt = VGet(wp.x, gy + 2.0f, wp.z);
            VECTOR topPt    = VGet(wp.x, gy + 120.0f, wp.z);
            drawBeam3D(groundPt, topPt, 1.8f, posCol);
            drawCircle3D(VGet(wp.x, gy, wp.z), 28.0f, 2.5f, posCol, 20);
        }
    }

    // =========================================================================
    // 5. 生存ネズミへの3D距離ライン＆逃走感知リング
    // =========================================================================
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            VECTOR mPos = obj->GetPos();
            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            unsigned int mCol = isFast ? GetColor(235, 45, 45) : GetColor(220, 140, 0);

            // 猫からネズミへの3D直線
            DrawLine3D(VGet(pPos.x, pPos.y + 25.0f, pPos.z), VGet(mPos.x, mPos.y + 15.0f, mPos.z), mCol);
            // ネズミの当たり判定リング
            drawCircle3D(mPos, obj->GetRadius(), 2.5f, mCol, 20);
            // FastMouseの逃走開始距離リング (160u = 2.76m)
            if (isFast)
            {
                drawCircle3D(mPos, 160.0f, 2.5f, GetColor(210, 40, 150), 28);
            }
        }
    }

    // =========================================================================
    // 6. 見えない壁（AddWallQuad補助壁 & 外周ClampToBounds）および壁コリジョンの3Dフレーム表示
    // =========================================================================
    {
        unsigned int invWallBeamCol = GetColor(255, 30, 210); // 見えない壁(AddWallQuad): 鮮やかなマゼンタ
        unsigned int invWallDiagCol = GetColor(255, 230, 60); // 見えない壁の対角フレーム: イエロー
        unsigned int meshWallCol    = GetColor(80, 235, 150); // モデル壁コリジョン: ライトグリーン

        for (const auto& wp : stage.GetWallPolygons())
        {
            if (wp.isInvisibleWall)
            {
                // 見えない壁（AddWallQuad）は視認しやすいよう立体ビームで3Dフレーム表示
                drawBeam3D(wp.v0, wp.v1, 2.0f, invWallBeamCol);
                drawBeam3D(wp.v1, wp.v2, 2.0f, invWallBeamCol);
                drawBeam3D(wp.v2, wp.v0, 1.4f, invWallDiagCol);
            }
            else
            {
                // 通常壁コリジョンポリゴンも3Dワイヤーフレーム表示
                DrawLine3D(wp.v0, wp.v1, meshWallCol);
                DrawLine3D(wp.v1, wp.v2, meshWallCol);
                DrawLine3D(wp.v2, wp.v0, meshWallCol);
            }
        }

        // ステージ外周のセーフティネット透明境界（ClampToBounds）の3Dフレーム表示
        VECTOR minB = stage.GetMinBounds();
        VECTOR maxB = stage.GetMaxBounds();
        float bY0 = -10.0f;
        float bY1 = 80.0f;
        unsigned int boundsCol = GetColor(200, 80, 255);
        VECTOR c00 = VGet(minB.x, bY0, minB.z), c10 = VGet(maxB.x, bY0, minB.z);
        VECTOR c11 = VGet(maxB.x, bY0, maxB.z), c01 = VGet(minB.x, bY0, maxB.z);
        VECTOR u00 = VGet(minB.x, bY1, minB.z), u10 = VGet(maxB.x, bY1, minB.z);
        VECTOR u11 = VGet(maxB.x, bY1, maxB.z), u01 = VGet(minB.x, bY1, maxB.z);
        DrawLine3D(c00, c10, boundsCol); DrawLine3D(c10, c11, boundsCol);
        DrawLine3D(c11, c01, boundsCol); DrawLine3D(c01, c00, boundsCol);
        DrawLine3D(u00, u10, boundsCol); DrawLine3D(u10, u11, boundsCol);
        DrawLine3D(u11, u01, boundsCol); DrawLine3D(u01, u00, boundsCol);
        DrawLine3D(c00, u00, boundsCol); DrawLine3D(c10, u10, boundsCol);
        DrawLine3D(c11, u11, boundsCol); DrawLine3D(c01, u01, boundsCol);
    }

    SetUseLighting(TRUE);
}

void GameManager::DrawDebug2DOverlay()
{
    auto player = m_objManager.GetPlayer();
    if (!player)
    {
        return;
    }

    const auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();

    const auto& stage = m_objManager.GetStage();
    const VECTOR pPos = player->GetPos();
    const float unitPerMeter = Config::WORLD_UNITS_PER_METER; // 58.0533f
    const float cubeSize = Config::CUBE_10M_SIZE_UNITS;       // 580.5333f (10m)
    const float halfCube = cubeSize * 0.5f;                   // 290.2667f (5m)

    // 3Dワールド座標を2D画面上にタグ描画するヘルパー（左右のUIパネル領域と重なる場合はスキップ）
    auto drawWorldLabel = [&](const VECTOR& worldPos, const char* text, unsigned int textCol, unsigned int bgCol = GetColor(10, 16, 28))
    {
        VECTOR sp = ConvWorldPosToScreenPos(worldPos);
        if (sp.z > 0.0f && sp.z < 1.0f &&
            sp.x >= 0.0f && sp.x <= static_cast<float>(Config::SCREEN_WIDTH) &&
            sp.y >= 0.0f && sp.y <= static_cast<float>(Config::SCREEN_HEIGHT))
        {
            int tw = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), font13);
            int tx = static_cast<int>(sp.x) - tw / 2;
            int ty = static_cast<int>(sp.y) - 10;

            // 左側ステータス/デバッグパネル領域または右側ミニマップ領域と重なる場合は描画しない
            if (tx < 435 && ty < 608) return;
            if (tx + tw > Config::SCREEN_WIDTH - 365 && ty < 530) return;
            if (tx + tw > Config::SCREEN_WIDTH - 615 && ty > Config::SCREEN_HEIGHT - 95) return;

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 205);
            DrawBox(tx - 4, ty - 2, tx + tw + 4, ty + 17, bgCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            DrawBox(tx - 4, ty - 2, tx + tw + 4, ty + 17, textCol, FALSE);
            DrawStringToHandle(tx, ty, text, textCol, font13);
        }
    };

    // =========================================================================
    // 1. 3D空間上の距離目測ラベル投影（10m×10m×10mキューブ・猫1.8m身長・Pos1〜8・ネズミ距離）
    // =========================================================================
    VECTOR cubeCenter = pPos;
    if (m_debugCubeMode == 1)
    {
        float yaw = m_camera.GetAngleH();
        cubeCenter = VGet(pPos.x + std::sin(yaw) * halfCube, pPos.y, pPos.z + std::cos(yaw) * halfCube);
    }
    else if (m_debugCubeMode == 2)
    {
        cubeCenter = VGet(0.0f, pPos.y, 0.0f);
    }

    if (m_debugCubeMode != 3)
    {
        float baseY = cubeCenter.y + 2.2f;
        float topY  = baseY + cubeSize;
        float midY  = baseY + halfCube;

        unsigned int cyanCol   = GetColor(80, 235, 255);
        unsigned int yellowCol = GetColor(255, 220, 80);

        // 底面4辺の中央に「10m (580.5u)」ラベルを表示
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z - halfCube), "◀ 10m辺 (580.5u) ▶", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z + halfCube), "◀ 10m辺 (580.5u) ▶", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x - halfCube, baseY, cubeCenter.z), "▲ 10m辺 (580.5u) ▼", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x + halfCube, baseY, cubeCenter.z), "▲ 10m辺 (580.5u) ▼", cyanCol);

        // 垂直柱に「高さ 5m」「高さ 10m」ラベルを表示
        drawWorldLabel(VGet(cubeCenter.x - halfCube, midY, cubeCenter.z - halfCube), "高さ 5.0m (290.3u)", yellowCol);
        drawWorldLabel(VGet(cubeCenter.x - halfCube, topY, cubeCenter.z - halfCube), "■ 高さ 10.0m (580.5u)", yellowCol);
        drawWorldLabel(VGet(cubeCenter.x + halfCube, topY, cubeCenter.z + halfCube), "■ 10m×10m×10m 頂点", yellowCol);

        // 中心から前後左右2m・4m地点の目盛りラベル
        for (int m : { -4, -2, 2, 4 })
        {
            char mBuf[24];
            snprintf(mBuf, sizeof(mBuf), "%+dm", m);
            drawWorldLabel(VGet(cubeCenter.x + m * unitPerMeter, baseY, cubeCenter.z), mBuf, GetColor(160, 240, 255));
            drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z + m * unitPerMeter), mBuf, GetColor(160, 240, 255));
        }
    }

    // 猫の1.8m身長ルーラーのラベル
    {
        float rulerX = pPos.x + player->GetBodyRadius() + 26.0f;
        float baseH  = Config::CAT_BASE_HEIGHT_UNITS;
        float curHm  = player->GetCurrentHeightMeters();
        char hBuf[64];
        if (curHm > Config::CAT_BASE_HEIGHT_METERS + 0.02f)
        {
            snprintf(hBuf, sizeof(hBuf), "猫身長 1.80m基準 (現在:%.2fm)", curHm);
        }
        else
        {
            snprintf(hBuf, sizeof(hBuf), "猫身長 1.80m (104.5u)");
        }
        drawWorldLabel(VGet(rulerX, pPos.y + baseH + 8.0f, pPos.z), hBuf, GetColor(255, 150, 235));
    }

    // Position.csv (Pos1〜Pos8) の3Dラベル
    std::vector<VECTOR> csvPositions = CsvManager::GetInstance().GetMousePositionsForMap(m_selectedMap);
    for (size_t i = 0; i < csvPositions.size(); ++i)
    {
        VECTOR wp = csvPositions[i];
        float gy = wp.y;
        stage.GetGroundHeight(VGet(wp.x, wp.y + 100.0f, wp.z), gy);
        char pLabel[48];
        snprintf(pLabel, sizeof(pLabel), "Pos%zu (%.0f, %.0f)", i + 1, wp.x, wp.z);
        drawWorldLabel(VGet(wp.x, gy + 125.0f, wp.z), pLabel, GetColor(130, 255, 130));
    }

    // 生存ネズミ頭上のスピード＆距離タグ
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (!mouse) continue;

            VECTOR mPos = mouse->GetPos();
            float dx = mPos.x - pPos.x;
            float dy = mPos.y - pPos.y;
            float dz = mPos.z - pPos.z;
            float distU = std::sqrt(dx * dx + dy * dy + dz * dz);
            float distM = distU / unitPerMeter;

            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            float mSpd = mouse->GetCurrentSpeed();
            char mTag[64];
            snprintf(mTag, sizeof(mTag), "%s SPD:%.2f (%.1fm)", isFast ? "Fast" : "Norm", mSpd, distM);
            drawWorldLabel(VGet(mPos.x, mPos.y + 42.0f, mPos.z), mTag, isFast ? GetColor(255, 120, 120) : GetColor(255, 220, 100));
        }
    }

    // =========================================================================
    // 2. 左側：詳細デバッグパネル（スピード数値・10m目測スケール・座標・ステージ＆CSV情報）
    // =========================================================================
    int panelX = 14;
    int panelY = 106;
    int panelW = 415;
    int panelH = 495;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(10, 16, 28), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(panelX, panelY, panelX + panelW, panelY + panelH, GetColor(0, 220, 200), FALSE);

    int ty = panelY + 8;
    int lx = panelX + 10;

    DrawStringToHandle(lx, ty, "■ [F1] 統合デバッグ情報 (速度・10m目測・マップ)", GetColor(80, 255, 230), font16);
    ty += 24;
    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (A) プレイヤー現在スピード数値セクション ---
    float cfgSpd    = player->GetCurrentSpeed();
    float actSpd    = player->GetActualMoveSpeed();
    float cfgUnitS  = cfgSpd * 60.0f;
    float cfgMeterS = cfgUnitS / unitPerMeter;
    float cfgKmH    = cfgMeterS * 3.6f;
    float actMeterS = (actSpd * 60.0f) / unitPerMeter;

    DrawStringToHandle(lx, ty, "【プレイヤー現在スピード (数値)】", GetColor(255, 230, 80), font13);
    ty += 18;

    char spdLine1[96];
    snprintf(spdLine1, sizeof(spdLine1), "・設定スピード : %.2f u/f  (%.1f u/s)", cfgSpd, cfgUnitS);
    DrawStringToHandle(lx, ty, spdLine1, GetColor(120, 255, 150), font16);
    ty += 21;

    char spdLine2[96];
    snprintf(spdLine2, sizeof(spdLine2), "・実移動スピード: %.2f u/f  (%.2f m/s)", actSpd, actMeterS);
    DrawStringToHandle(lx, ty, spdLine2, GetColor(180, 240, 255), font13);
    ty += 18;

    char spdLine3[96];
    snprintf(spdLine3, sizeof(spdLine3), "・1.8m換算速度 : 秒速 %.2f m/s (時速 %.1f km/h)", cfgMeterS, cfgKmH);
    DrawStringToHandle(lx, ty, spdLine3, GetColor(255, 210, 110), font13);
    ty += 18;

    // スピード視覚バー (0.0 〜 6.0 u/f)
    int barX = lx + 4;
    int barW = panelW - 28;
    int barH = 10;
    DrawBox(barX, ty, barX + barW, ty + barH, GetColor(22, 30, 48), TRUE);
    float spdRatio = MathHelper::Clamp(cfgSpd / 6.0f, 0.0f, 1.0f);
    float actRatio = MathHelper::Clamp(actSpd / 6.0f, 0.0f, 1.0f);
    DrawBox(barX, ty, barX + static_cast<int>(barW * spdRatio), ty + barH, GetColor(40, 180, 100), TRUE);
    DrawBox(barX, ty + 2, barX + static_cast<int>(barW * actRatio), ty + barH - 2, GetColor(120, 255, 160), TRUE);
    DrawBox(barX, ty, barX + barW, ty + barH, GetColor(80, 120, 150), FALSE);
    ty += 15;

    const auto& catCsv = CsvManager::GetInstance().GetCatStatus();
    {
        char csvCatBuf[96];
        snprintf(csvCatBuf, sizeof(csvCatBuf), "・Statu.csv[Cat]: 初速%.2f / +%.2f(1Lv) / 上限%.2f",
                 catCsv.baseSpeed, catCsv.speedGrowthPerUnit, catCsv.maxSpeed);
        DrawStringToHandle(lx, ty, csvCatBuf, GetColor(180, 205, 230), font13);
        ty += 17;
    }

    char skillSpdBuf[96];
    snprintf(skillSpdBuf, sizeof(skillSpdBuf), "・スキル突進速度: 飛びつき 14.0u/f | タックル 11.5u/f");
    DrawStringToHandle(lx, ty, skillSpdBuf, GetColor(180, 205, 230), font13);
    ty += 20;

    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (B) 10m×10m×10m 距離目測スケール ＆ 座標セクション ---
    const char* cubeModeNames[] = {
        "猫中心 10m×10m×10m ■ + 10mグリッド",
        "猫前方 10m×10m×10m ■ + 10mグリッド",
        "原点(0,0,0) 10m×10m×10m ■ + グリッド",
        "OFF (非表示)"
    };
    DrawStringToHandle(lx, ty, "【距離目測スケール (猫の身長=1.8m基準)】", GetColor(255, 230, 80), font13);
    ty += 18;

    char scaleBuf1[96];
    snprintf(scaleBuf1, sizeof(scaleBuf1), "・基準: 猫身長 1.80m = %.1fu (現在身長: %.2fm)",
             Config::CAT_BASE_HEIGHT_UNITS, player->GetCurrentHeightMeters());
    DrawStringToHandle(lx, ty, scaleBuf1, GetColor(255, 180, 235), font13);
    ty += 17;

    char scaleBuf2[96];
    snprintf(scaleBuf2, sizeof(scaleBuf2), "・1m = %.2fu  |  ■10m×10m×10m 1辺 = %.1fu", unitPerMeter, cubeSize);
    DrawStringToHandle(lx, ty, scaleBuf2, GetColor(100, 255, 230), font13);
    ty += 17;

    char modeBuf[112];
    snprintf(modeBuf, sizeof(modeBuf), "・[7] ■配置: %s", cubeModeNames[m_debugCubeMode % 4]);
    DrawStringToHandle(lx, ty, modeBuf, GetColor(255, 240, 130), font13);
    ty += 18;

    char posBuf[112];
    snprintf(posBuf, sizeof(posBuf), "・現在座標: X=%.1f, Y=%.1f, Z=%.1f (CSV用 %d:%d:%d)",
             pPos.x, pPos.y, pPos.z,
             static_cast<int>(std::round(pPos.x)),
             static_cast<int>(std::round(pPos.y)),
             static_cast<int>(std::round(pPos.z)));
    DrawStringToHandle(lx, ty, posBuf, GetColor(220, 240, 255), font13);
    ty += 18;

    VECTOR minB = stage.GetMinBounds();
    VECTOR maxB = stage.GetMaxBounds();
    float mapW_U = maxB.x - minB.x;
    float mapD_U = maxB.z - minB.z;
    float mapW_M = mapW_U / unitPerMeter;
    float mapD_M = mapD_U / unitPerMeter;
    const char* mapName = (m_selectedMap == ModelConfig::MapType::NewMap) ? "Map2(NewMap)" : "Map1(House)";

    char stageBuf[112];
    snprintf(stageBuf, sizeof(stageBuf), "・%s: %.0f×%.0fu (約 %.1fm × %.1fm) 壁:%zu枚",
             mapName, mapW_U, mapD_U, mapW_M, mapD_M, stage.GetWallPolygons().size());
    DrawStringToHandle(lx, ty, stageBuf, GetColor(190, 220, 250), font13);
    ty += 20;

    DrawLine(lx, ty, panelX + panelW - 10, ty, GetColor(50, 90, 110));
    ty += 6;

    // --- (C) ネズミ(Mouse) CSV設定＆個体別スピード・距離一覧 ---
    DrawStringToHandle(lx, ty, "【ネズミ速度 (Statu.csv) & 生存個体との距離】", GetColor(255, 230, 80), font13);
    ty += 18;

    const auto& mouseCsv = CsvManager::GetInstance().GetMouseStatus();
    {
        char mCsvBuf[96];
        snprintf(mCsvBuf, sizeof(mCsvBuf), "・Statu.csv[Mouse]: 初速%.2f / +%.2f(1匹) / 上限%.2f",
                 mouseCsv.baseSpeed, mouseCsv.speedGrowthPerUnit, mouseCsv.maxSpeed);
        DrawStringToHandle(lx, ty, mCsvBuf, GetColor(200, 220, 240), font13);
        ty += 17;
    }

    int mouseIdx = 1;
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            auto mouse = std::dynamic_pointer_cast<MouseBase3D>(obj);
            if (!mouse) continue;

            VECTOR mPos = mouse->GetPos();
            float dx = mPos.x - pPos.x;
            float dy = mPos.y - pPos.y;
            float dz = mPos.z - pPos.z;
            float distU = std::sqrt(dx * dx + dy * dy + dz * dz);
            float distM = distU / unitPerMeter;
            float mSpd  = mouse->GetCurrentSpeed();
            float mSpdM = (mSpd * 60.0f) / unitPerMeter;
            bool isFast = (obj->GetType() == ObjectType::FastMouse);

            char mLine[112];
            snprintf(mLine, sizeof(mLine), " #%d [%s] 速度:%.2fu/f(%.1fm/s) | 距離:%.1fm(%.0fu)%s",
                     mouseIdx,
                     isFast ? "Fast" : "Norm",
                     mSpd, mSpdM, distM, distU,
                     mouse->IsStunned() ? " [スタン]" : "");
            DrawStringToHandle(lx, ty, mLine, isFast ? GetColor(255, 150, 150) : GetColor(240, 230, 160), font13);
            ty += 16;
            mouseIdx++;
            if (ty > panelY + panelH - 18) break;
        }
    }

    // =========================================================================
    // 3. 右上：2Dトップダウン・ミニマップ（壁形状・10mグリッド・Pos1〜8・ネズミ・10m×10m枠）
    // =========================================================================
    int mapBoxW = 345;
    int mapBoxH = 265;
    int mapBoxX = Config::SCREEN_WIDTH - mapBoxW - 16;
    int mapBoxY = 38;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
    DrawBox(mapBoxX, mapBoxY, mapBoxX + mapBoxW, mapBoxY + mapBoxH, GetColor(10, 16, 28), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(mapBoxX, mapBoxY, mapBoxX + mapBoxW, mapBoxY + mapBoxH, GetColor(0, 220, 200), FALSE);

    DrawStringToHandle(mapBoxX + 10, mapBoxY + 6, "■ 2D マップ & 10m×10m レーダー", GetColor(80, 255, 230), font16);

    int drawAreaX = mapBoxX + 14;
    int drawAreaY = mapBoxY + 30;
    int drawAreaW = mapBoxW - 28;
    int drawAreaH = mapBoxH - 54;

    DrawBox(drawAreaX, drawAreaY, drawAreaX + drawAreaW, drawAreaY + drawAreaH, GetColor(6, 10, 18), TRUE);

    float spanX = (maxB.x - minB.x);
    float spanZ = (maxB.z - minB.z);
    if (spanX < 100.0f) spanX = 1000.0f;
    if (spanZ < 100.0f) spanZ = 1000.0f;

    float scaleX = static_cast<float>(drawAreaW - 12) / spanX;
    float scaleZ = static_cast<float>(drawAreaH - 12) / spanZ;
    float mapScale = (scaleX < scaleZ) ? scaleX : scaleZ;

    float centerWorldX = (minB.x + maxB.x) * 0.5f;
    float centerWorldZ = (minB.z + maxB.z) * 0.5f;
    int centerScreenX = drawAreaX + drawAreaW / 2;
    int centerScreenY = drawAreaY + drawAreaH / 2;

    auto worldToMinimap = [&](float wx, float wz, int& outSx, int& outSy)
    {
        outSx = centerScreenX + static_cast<int>((wx - centerWorldX) * mapScale);
        // 3D空間の+Zが上になるようにY座標を反転
        outSy = centerScreenY - static_cast<int>((wz - centerWorldZ) * mapScale);
    };

    // ミニマップ上の10mグリッド描画
    int minIdxX = static_cast<int>(std::floor(minB.x / cubeSize));
    int maxIdxX = static_cast<int>(std::ceil(maxB.x / cubeSize));
    int minIdxZ = static_cast<int>(std::floor(minB.z / cubeSize));
    int maxIdxZ = static_cast<int>(std::ceil(maxB.z / cubeSize));

    for (int ix = minIdxX; ix <= maxIdxX; ++ix)
    {
        float wx = static_cast<float>(ix) * cubeSize;
        if (wx < minB.x || wx > maxB.x) continue;
        int sx0, sy0, sx1, sy1;
        worldToMinimap(wx, minB.z, sx0, sy0);
        worldToMinimap(wx, maxB.z, sx1, sy1);
        DrawLine(sx0, sy0, sx1, sy1, (ix == 0) ? GetColor(60, 120, 150) : GetColor(28, 52, 70));
    }
    for (int iz = minIdxZ; iz <= maxIdxZ; ++iz)
    {
        float wz = static_cast<float>(iz) * cubeSize;
        if (wz < minB.z || wz > maxB.z) continue;
        int sx0, sy0, sx1, sy1;
        worldToMinimap(minB.x, wz, sx0, sy0);
        worldToMinimap(maxB.x, wz, sx1, sy1);
        DrawLine(sx0, sy0, sx1, sy1, (iz == 0) ? GetColor(60, 120, 150) : GetColor(28, 52, 70));
    }

    // ステージ外周バウンディングボックス
    int bMinX, bMinY, bMaxX, bMaxY;
    worldToMinimap(minB.x, maxB.z, bMinX, bMinY);
    worldToMinimap(maxB.x, minB.z, bMaxX, bMaxY);
    DrawBox(bMinX, bMinY, bMaxX, bMaxY, GetColor(70, 110, 150), FALSE);

    // ステージ壁ポリゴンの描画
    unsigned int wallMapCol = GetColor(90, 165, 220);
    for (const auto& wp : stage.GetWallPolygons())
    {
        int x0, y0, x1, y1;
        worldToMinimap(wp.v0.x, wp.v0.z, x0, y0);
        worldToMinimap(wp.v1.x, wp.v1.z, x1, y1);
        unsigned int col = wp.isInvisibleWall ? GetColor(255, 50, 220) : wallMapCol;
        DrawLine(x0, y0, x1, y1, col);
        int x2, y2;
        worldToMinimap(wp.v2.x, wp.v2.z, x2, y2);
        DrawLine(x1, y1, x2, y2, col);
        DrawLine(x2, y2, x0, y0, col);
    }

    // Position.csv (Pos1〜Pos8) のミニマップピン
    for (size_t i = 0; i < csvPositions.size(); ++i)
    {
        int px, py;
        worldToMinimap(csvPositions[i].x, csvPositions[i].z, px, py);
        DrawCircle(px, py, 3, GetColor(80, 230, 100), TRUE);
        char pNum[8];
        snprintf(pNum, sizeof(pNum), "P%zu", i + 1);
        DrawStringToHandle(px + 4, py - 6, pNum, GetColor(130, 255, 140), font13);
    }

    // ミニマップ上の10m×10mキューブ領域（シアン枠）
    if (m_debugCubeMode != 3)
    {
        int cx0, cy0, cx1, cy1;
        worldToMinimap(cubeCenter.x - halfCube, cubeCenter.z + halfCube, cx0, cy0);
        worldToMinimap(cubeCenter.x + halfCube, cubeCenter.z - halfCube, cx1, cy1);
        DrawBox(cx0, cy0, cx1, cy1, GetColor(0, 255, 220), FALSE);
    }

    // 生存ネズミのミニマップ位置
    for (const auto& obj : m_objManager.GetObjects())
    {
        if (!obj || !obj->IsAlive()) continue;
        if (obj->GetType() == ObjectType::NormalMouse || obj->GetType() == ObjectType::FastMouse)
        {
            VECTOR mPos = obj->GetPos();
            int mx, my;
            worldToMinimap(mPos.x, mPos.z, mx, my);
            bool isFast = (obj->GetType() == ObjectType::FastMouse);
            DrawCircle(mx, my, 4, isFast ? GetColor(255, 70, 70) : GetColor(255, 210, 60), TRUE);
        }
    }

    // プレイヤー(猫)のミニマップ位置と向きベクトル
    int pMapX, pMapY;
    worldToMinimap(pPos.x, pPos.z, pMapX, pMapY);
    float rotY = player->GetRotY();
    int dirX = pMapX + static_cast<int>(std::sin(rotY) * 12.0f);
    int dirY = pMapY - static_cast<int>(std::cos(rotY) * 12.0f);
    DrawLine(pMapX, pMapY, dirX, dirY, GetColor(255, 255, 255));
    DrawCircle(pMapX, pMapY, 5, GetColor(0, 255, 180), TRUE);
    DrawCircle(pMapX, pMapY, 5, GetColor(255, 255, 255), FALSE);

    DrawBox(drawAreaX, drawAreaY, drawAreaX + drawAreaW, drawAreaY + drawAreaH, GetColor(60, 100, 130), FALSE);
    DrawStringToHandle(mapBoxX + 12, mapBoxY + mapBoxH - 20, "緑:猫  水色枠:10m×10m  赤/黄:鼠  P1-8:CSV座標", GetColor(170, 205, 230), font13);

    // =========================================================================
    // 4. 右側中段：F1 デバッグ＆チートショートカット操作パネル
    // =========================================================================
    int cheatW = 345;
    int cheatH = 212;
    int cheatX = Config::SCREEN_WIDTH - cheatW - 16;
    int cheatY = mapBoxY + mapBoxH + 8;

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(cheatX, cheatY, cheatX + cheatW, cheatY + cheatH, GetColor(15, 18, 30), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawBox(cheatX, cheatY, cheatX + cheatW, cheatY + cheatH, GetColor(255, 220, 40), FALSE);

    int cy = cheatY + 8;
    DrawStringToHandle(cheatX + 10, cy, "★ F1 デバッグ＆目測・チート操作", GetColor(255, 240, 60), font16);
    cy += 23;
    DrawStringToHandle(cheatX + 12, cy, "[1] 通常ネズミ +1体追加スポーン", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[2] 高速ネズミ +1体追加スポーン", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[3] パンプアップ Lv +1 (速度・身長UP)", GetColor(220, 230, 245), font13);
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[4] ネズミ時間停止: %s", m_objManager.IsTimeFrozen() ? "ON" : "OFF");
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[5] スキルCT無効  : %s", player->IsNoCooldown() ? "ON" : "OFF");
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(220, 230, 245), font13,
                             "[6] エフェクト表示: %s", EffectManager::GetInstance().IsEffectEnabled() ? "ON" : "OFF");
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[7] 10m×10m×10m ■表示モード切替", GetColor(100, 255, 220), font13);
    cy += 18;
    DrawFormatStringToHandle(cheatX + 12, cy, GetColor(255, 220, 100), font13,
                             "[8] 10m全体目測・広角カメラ: %s", m_debugWideCamera ? "ON (俯瞰)" : "OFF (通常)");
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[F1] デバッグオーバーレイ ON/OFF", GetColor(180, 195, 215), font13);
}

