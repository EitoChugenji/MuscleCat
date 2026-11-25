// ============================================================================
// @file   UIManager.cpp
// @brief  UIManager クラスの実装（全2D UI・モーダル・デバッグオーバーレイ描画）
// ============================================================================
#include "UIManager.h"
#include "GameManager.h"
#include "SoundManager.h"
#include "WorkoutCollectionManager.h"
#include "Player3D.h"
#include "Mouse3D.h"
#include "Obstacle3D.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "CsvManager.h"
#include "ModelConfig.h"
#include "Config.h"
#include "Common.h"
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
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
        DrawStringToHandle(minusX + 11, btnY + 4, "< -", hoverMinus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font13);

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
        DrawStringToHandle(plusX + 11, btnY + 4, "+ >", hoverPlus ? GetColor(255, 255, 255) : GetColor(190, 215, 245), font13);
    }

    // タイトル画面で現在再生すべき筋トレモーションを算出（ONのモーションのみを順番にループ）
    Player3D::TitleWorkoutType CalcActiveTitleWorkout(int animTimer)
    {
        std::vector<WorkoutMotionType> enabledList;
        for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
        {
            auto t = static_cast<WorkoutMotionType>(i);
            if (Player3D::IsWorkoutMotionEnabled(t))
            {
                enabledList.push_back(t);
            }
        }
        if (enabledList.empty())
        {
            enabledList.push_back(WorkoutMotionType::Squat);
        }

        // 1種目あたり 270フレーム (200F: 筋トレモーション再生 + 70F: 息抜き・ポージング待機)
        int currentSegment = animTimer / 270;
        int subFrame       = animTimer % 270;
        if (subFrame >= 200)
        {
            return Player3D::TitleWorkoutType::Idle;
        }

        static int s_lastTitleSegment = -1;
        static WorkoutMotionType s_chosenTitleMotion = WorkoutMotionType::Squat;
        if (currentSegment != s_lastTitleSegment ||
            !Player3D::IsWorkoutMotionEnabled(s_chosenTitleMotion))
        {
            s_lastTitleSegment = currentSegment;
            s_chosenTitleMotion = enabledList[GetRand(static_cast<int>(enabledList.size()) - 1)];
        }

        switch (s_chosenTitleMotion)
        {
        case WorkoutMotionType::Squat:           return Player3D::TitleWorkoutType::Squat;
        case WorkoutMotionType::PushUp:          return Player3D::TitleWorkoutType::PushUp;
        case WorkoutMotionType::SitUp:           return Player3D::TitleWorkoutType::SitUp;
        case WorkoutMotionType::PullUp:          return Player3D::TitleWorkoutType::PullUp;
        case WorkoutMotionType::SideRaise:       return Player3D::TitleWorkoutType::SideRaise;
        case WorkoutMotionType::DumbbellCurl:    return Player3D::TitleWorkoutType::DumbbellCurl;
        case WorkoutMotionType::InclineBench:    return Player3D::TitleWorkoutType::InclineBench;
        case WorkoutMotionType::BenchPress:      return Player3D::TitleWorkoutType::BenchPress;
        case WorkoutMotionType::InOverHeadPress: return Player3D::TitleWorkoutType::InOverHeadPress;
        default:                                 return Player3D::TitleWorkoutType::Squat;
        }
    }

    // 筋トレアニメーション選択ボタン（9種・2段グリッド）の座標算出ヘルパー
    void GetWorkoutBtnRect(int cardX, int cardY, int cardW, int cardH, int index, int& outX, int& outY, int& outW, int& outH)
    {
        const int padX = 12;
        const int gapX = 8;
        const int gapY = 6;
        const int headerH = 26;
        const int btnH = 34;

        if (index < 5)
        {
            int col = index;
            int btnW = (cardW - padX * 2 - gapX * 4) / 5;
            outX = cardX + padX + col * (btnW + gapX);
            outY = cardY + headerH;
            outW = btnW;
            outH = btnH;
        }
        else
        {
            int col = index - 5;
            int btnW = (cardW - padX * 2 - gapX * 3) / 4;
            outX = cardX + padX + col * (btnW + gapX);
            outY = cardY + headerH + btnH + gapY;
            outW = btnW;
            outH = btnH;
        }
    }

    // 筋トレコレクションアクションボタン（ガチャ・セーブ・ロード・(Debug削除)）の座標算出ヘルパー
    void GetWorkoutActionBtnRect(int cardX, int cardY, int cardW, int cardH, int actionIdx, int& outX, int& outY, int& outW, int& outH)
    {
        const int padX = 12;
        const int gapX = 8;
        const int actY = cardY + 26 + 34 * 2 + 6 + 10; // cardY + 110
        const int actH = 28;
        outY = actY;
        outH = actH;

#ifdef _DEBUG
        // デバッグビルド時: 4ボタン（ガチャ、セーブ、ロード、デバッグ削除）
        int totalW = cardW - padX * 2 - gapX * 3;
        int w0 = (totalW * 14) / 44; // ガチャ
        int w1 = (totalW * 10) / 44; // セーブ
        int w2 = (totalW * 10) / 44; // ロード
        int w3 = totalW - w0 - w1 - w2; // デバッグ削除

        if (actionIdx == 0)      { outX = cardX + padX; outW = w0; }
        else if (actionIdx == 1) { outX = cardX + padX + w0 + gapX; outW = w1; }
        else if (actionIdx == 2) { outX = cardX + padX + w0 + gapX + w1 + gapX; outW = w2; }
        else                     { outX = cardX + padX + w0 + gapX + w1 + gapX + w2 + gapX; outW = w3; }
#else
        // Releaseビルド時: 3ボタン（ガチャ、セーブ、ロード）のみ！デバッグ削除は非表示
        int totalW = cardW - padX * 2 - gapX * 2;
        int w0 = (totalW * 18) / 38; // ガチャ
        int w1 = (totalW * 10) / 38; // セーブ
        int w2 = totalW - w0 - w1;   // ロード

        if (actionIdx == 0)      { outX = cardX + padX; outW = w0; }
        else if (actionIdx == 1) { outX = cardX + padX + w0 + gapX; outW = w1; }
        else if (actionIdx == 2) { outX = cardX + padX + w0 + gapX + w1 + gapX; outW = w2; }
        else                     { outX = 0; outY = -999; outW = 0; outH = 0; }
#endif
    }

    // 筋トレコレクション＆アニメーション選択カード共通描画ヘルパー
    void DrawWorkoutMotionSelectorCard(
        int x, int y, int w, int h,
        bool isRowSelected,
        int focusedBtnIdx,
        int mx, int my,
        int font16, int font13
    )
    {
        auto& collMgr = WorkoutCollectionManager::GetInstance();

        // 1. 外枠カード
        SoftUI::DrawSoftCard(x, y, w, h, 16,
                             isRowSelected ? GetColor(26, 34, 56) : GetColor(18, 24, 40), 242,
                             isRowSelected ? GetColor(255, 195, 50) : GetColor(55, 75, 110),
                             isRowSelected ? 2 : 1, isRowSelected, GetColor(255, 185, 45));

        // 2. ヘッダー表示（コレクション所持数 ＆ プロテインポイント）
        char colTitleBuf[64];
        snprintf(colTitleBuf, sizeof(colTitleBuf), "★ 筋トレコレクション (%d / %d 所持)",
                 collMgr.GetUnlockedCount(), collMgr.GetTotalMotionCount());
        DrawStringToHandle(x + 12, y + 5, colTitleBuf, isRowSelected ? GetColor(255, 235, 90) : GetColor(220, 240, 255), font13);

        char ptBuf[64];
        snprintf(ptBuf, sizeof(ptBuf), "プロテイン: %d pt", collMgr.GetProteinPoints());
        int ptW = GetDrawStringWidthToHandle(ptBuf, static_cast<int>(strlen(ptBuf)), font13);
        DrawStringToHandle(x + w - ptW - 12, y + 5, ptBuf, GetColor(255, 215, 60), font13);

        // 3. 9つのモーションボタン（上段5個 / 下段4個）
        for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
        {
            auto mType      = static_cast<WorkoutMotionType>(i);
            bool isUnlocked = collMgr.IsMotionUnlocked(i);
            bool isOn       = Player3D::IsWorkoutMotionEnabled(mType);
            std::string aeCode = WorkoutCollectionManager::IdToAeCode(i);

            int bx = 0, by = 0, bw = 0, bh = 0;
            GetWorkoutBtnRect(x, y, w, h, i, bx, by, bw, bh);

            bool isHover = (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);
            bool isFocus = (isRowSelected && focusedBtnIdx == i) || isHover;

            if (isUnlocked)
            {
                unsigned int bgCol     = isOn ? (isFocus ? GetColor(38, 110, 68) : GetColor(24, 82, 50))
                                              : (isFocus ? GetColor(50, 60, 80)  : GetColor(30, 36, 50));
                unsigned int borderCol = isFocus ? GetColor(255, 230, 80)
                                                 : (isOn ? GetColor(85, 230, 130) : GetColor(70, 85, 110));

                SoftUI::DrawSoftCard(bx, by, bw, bh, 8,
                                     bgCol, 245, borderCol, isFocus ? 2 : 1,
                                     isFocus || isOn, isFocus ? GetColor(255, 220, 70) : GetColor(70, 210, 115));

                // モーション名（上段）
                const char* mName = Player3D::GetWorkoutMotionName(mType);
                int nameW = GetDrawStringWidthToHandle(mName, static_cast<int>(strlen(mName)), font13);
                DrawStringToHandle(bx + (bw - nameW) / 2, by + 2, mName, isOn ? GetColor(255, 255, 255) : GetColor(170, 180, 200), font13);

                // A/Eコード ＆ ON/OFFバッジ（下段）
                char badgeBuf[32];
            snprintf(badgeBuf, sizeof(badgeBuf), "%s", isOn ? "【 ON 】" : "【 OFF 】");
                int bdW = GetDrawStringWidthToHandle(badgeBuf, static_cast<int>(strlen(badgeBuf)), font13);
                DrawStringToHandle(bx + (bw - bdW) / 2, by + 17, badgeBuf, isOn ? GetColor(180, 255, 200) : GetColor(140, 150, 170), font13);
            }
            else
            {
                // 未所持（ロック表示）
                unsigned int bgCol     = isHover ? GetColor(34, 38, 48) : GetColor(22, 25, 34);
                unsigned int borderCol = isHover ? GetColor(140, 150, 175) : GetColor(50, 58, 72);

                SoftUI::DrawSoftCard(bx, by, bw, bh, 8, bgCol, 240, borderCol, 1);

            const char* lockTxt = "【未所持】";
                int lW = GetDrawStringWidthToHandle(lockTxt, static_cast<int>(strlen(lockTxt)), font13);
                DrawStringToHandle(bx + (bw - lW) / 2, by + 2, lockTxt, GetColor(130, 140, 155), font13);

                char codeBuf[32];
            snprintf(codeBuf, sizeof(codeBuf), "%s", "ガチャで解放");
                int cW = GetDrawStringWidthToHandle(codeBuf, static_cast<int>(strlen(codeBuf)), font13);
                DrawStringToHandle(bx + (bw - cW) / 2, by + 17, codeBuf, GetColor(100, 110, 130), font13);
            }
        }

        // 4. アクションボタン（ガチャ・セーブ・ロード・(Debug削除)）
#ifdef _DEBUG
        const int numActions = 4;
#else
        const int numActions = 3;
#endif
        for (int act = 0; act < numActions; ++act)
        {
            int ax = 0, ay = 0, aw = 0, ah = 0;
            GetWorkoutActionBtnRect(x, y, w, h, act, ax, ay, aw, ah);
            bool isHover = (mx >= ax && mx <= ax + aw && my >= ay && my <= ay + ah);

            if (act == 0) // ガチャ
            {
                bool isAllUnlocked = collMgr.IsAllUnlocked();
                bool canAfford = (collMgr.GetProteinPoints() >= WorkoutCollectionManager::GACHA_COST);

                unsigned int fillCol = isAllUnlocked ? GetColor(45, 55, 75)
                    : (canAfford ? (isHover ? GetColor(255, 185, 45) : GetColor(220, 150, 30))
                                 : (isHover ? GetColor(65, 50, 35) : GetColor(45, 35, 25)));
                unsigned int borderCol = isAllUnlocked ? GetColor(90, 110, 140)
                    : (canAfford ? (isHover ? GetColor(255, 240, 140) : GetColor(255, 210, 60))
                                 : GetColor(130, 95, 45));

                SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 250, borderCol, isHover ? 2 : 1);

                const char* gachaLabel = isAllUnlocked ? "★ コンプリート！"
                    : (canAfford ? "★ ガチャ (100 pt)" : "ガチャ (100 pt必要)");
                int gW = GetDrawStringWidthToHandle(gachaLabel, static_cast<int>(strlen(gachaLabel)), font13);
                unsigned int gTxtCol = isAllUnlocked ? GetColor(180, 200, 230)
                    : (canAfford ? GetColor(20, 20, 30) : GetColor(210, 170, 120));
                DrawStringToHandle(ax + (aw - gW) / 2, ay + 6, gachaLabel, gTxtCol, font13);
            }
            else if (act == 1) // セーブ
            {
                unsigned int fillCol   = isHover ? GetColor(40, 90, 130) : GetColor(25, 60, 95);
                unsigned int borderCol = isHover ? GetColor(140, 225, 255) : GetColor(65, 150, 210);
                SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

                const char* saveLabel = "セーブ (コピー)";
                int sW = GetDrawStringWidthToHandle(saveLabel, static_cast<int>(strlen(saveLabel)), font13);
                DrawStringToHandle(ax + (aw - sW) / 2, ay + 6, saveLabel, GetColor(225, 245, 255), font13);
            }
            else if (act == 2) // ロード
            {
                unsigned int fillCol   = isHover ? GetColor(40, 90, 130) : GetColor(25, 60, 95);
                unsigned int borderCol = isHover ? GetColor(140, 225, 255) : GetColor(65, 150, 210);
                SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

                const char* loadLabel = "ロード (読込)";
                int lW = GetDrawStringWidthToHandle(loadLabel, static_cast<int>(strlen(loadLabel)), font13);
                DrawStringToHandle(ax + (aw - lW) / 2, ay + 6, loadLabel, GetColor(225, 245, 255), font13);
            }
            else if (act == 3) // デバッグ削除（Debugビルドのみ）
            {
                unsigned int fillCol   = isHover ? GetColor(150, 35, 45) : GetColor(95, 25, 35);
                unsigned int borderCol = isHover ? GetColor(255, 120, 120) : GetColor(180, 50, 60);
                SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

                const char* resetLabel = "【Debug】データ削除";
                int rW = GetDrawStringWidthToHandle(resetLabel, static_cast<int>(strlen(resetLabel)), font13);
                DrawStringToHandle(ax + (aw - rW) / 2, ay + 6, resetLabel, GetColor(255, 225, 225), font13);
            }
        }

        // 5. フッターステータス ＆ セーブコード表示
        int footerY = y + 148;
        if (collMgr.GetStatusMessageTimer() > 0)
        {
            const auto& msg = collMgr.GetStatusMessage();
            DrawStringToHandle(x + 12, footerY, msg.c_str(), GetColor(255, 235, 100), font13);
        }
        else
        {
            std::string codeStr = "※筋トレの所持状況・プロテインは自動保存されます";
            DrawStringToHandle(x + 12, footerY, codeStr.c_str(), GetColor(150, 175, 205), font13);

            const char* hintTxt = "※クリックでON/OFF切替";
            int hW = GetDrawStringWidthToHandle(hintTxt, static_cast<int>(strlen(hintTxt)), font13);
            DrawStringToHandle(x + w - hW - 12, footerY, hintTxt, GetColor(130, 150, 175), font13);
        }
    }
} // namespace
void UIManager::GetWorkoutBtnRect(int cardX, int cardY, int cardW, int cardH, int btnIdx,
                                  int& outX, int& outY, int& outW, int& outH)
{
    ::GetWorkoutBtnRect(cardX, cardY, cardW, cardH, btnIdx, outX, outY, outW, outH);
}

UIManager::~UIManager()
{
    Release();
}

void UIManager::Init()
{
    Release();
    // Resource/UI/ にカスタムPNGが置かれていれば自動ロード（存在しない場合は -1 のまま標準UIを描画）
    m_hudStatusGraph  = LoadGraph(Config::HudUI::STATUS_PNG_PATH);
    m_hudGuideGraph   = LoadGraph(Config::HudUI::GUIDE_PNG_PATH);
    m_hudOverlayGraph = LoadGraph(Config::HudUI::OVERLAY_PNG_PATH);
}

void UIManager::Release()
{
    if (m_hudStatusGraph != -1)  { DeleteGraph(m_hudStatusGraph);  m_hudStatusGraph  = -1; }
    if (m_hudGuideGraph != -1)   { DeleteGraph(m_hudGuideGraph);   m_hudGuideGraph   = -1; }
    if (m_hudOverlayGraph != -1) { DeleteGraph(m_hudOverlayGraph); m_hudOverlayGraph = -1; }
}

bool UIManager::DrawHudStatusBg(int x, int y, int w, int h) const
{
    if (m_hudStatusGraph == -1) return false;
    if (w > 0 && h > 0)
    {
        DrawExtendGraph(x, y, x + w, y + h, m_hudStatusGraph, TRUE);
    }
    else
    {
        DrawGraph(x, y, m_hudStatusGraph, TRUE);
    }
    return true;
}

bool UIManager::DrawHudGuideBg(int x, int y, int w, int h) const
{
    if (m_hudGuideGraph == -1) return false;
    if (w > 0 && h > 0)
    {
        DrawExtendGraph(x, y, x + w, y + h, m_hudGuideGraph, TRUE);
    }
    else
    {
        DrawGraph(x, y, m_hudGuideGraph, TRUE);
    }
    return true;
}

void UIManager::DrawHudOverlay() const
{
    if (m_hudOverlayGraph == -1) return;
    int x = Config::HudUI::OVERLAY_X;
    int y = Config::HudUI::OVERLAY_Y;
    int w = Config::HudUI::OVERLAY_W;
    int h = Config::HudUI::OVERLAY_H;
    if (w > 0 && h > 0)
    {
        DrawExtendGraph(x, y, x + w, y + h, m_hudOverlayGraph, TRUE);
    }
    else
    {
        DrawGraph(x, y, m_hudOverlayGraph, TRUE);
    }
}

void UIManager::GetWorkoutActionBtnRect(int cardX, int cardY, int cardW, int cardH, int actionIdx,
                                        int& outX, int& outY, int& outW, int& outH)
{
    ::GetWorkoutActionBtnRect(cardX, cardY, cardW, cardH, actionIdx, outX, outY, outW, outH);
}

Player3D::TitleWorkoutType UIManager::CalcActiveTitleWorkout(int animTimer)
{
    return ::CalcActiveTitleWorkout(animTimer);
}

void UIManager::DrawGameClearScreen(GameManager& gm)
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

    // カラーパレット定義
    unsigned int colYellow  = GetColor(255, 235, 0);
    unsigned int colOrange  = GetColor(255, 140, 0);
    unsigned int colBlack   = GetColor(0, 0, 0);
    unsigned int colWhite   = GetColor(255, 255, 255);
    unsigned int colGold    = GetColor(255, 215, 0);
    unsigned int colNeonRed = GetColor(255, 0, 68);
    unsigned int colCyan    = GetColor(90, 225, 255);
    unsigned int colGreen   = GetColor(85, 235, 135);

    float animSec = gm.m_clearAnimTimer / 60.0f;

    // =========================================================================
    // 1. 左上：猫の頭上を邪魔しないコンパクトな「STAGE CLEAR!! / TIME UP!!」バナー
    //    （画面左側 X = 0〜540 は背景で筋トレする猫がくっきり見えるよう暗幕を敷かない）
    // =========================================================================
    {
        float headerBounce = std::abs(std::sin(animSec * 4.5f)) * -5.0f;
        int bannerX = 22;
        int bannerY = 18 + static_cast<int>(headerBounce);
        int bannerW = 504;
        int bannerH = 68;

        // 背面の斜めスピードバナー帯（アメコミ風ブラック＆ネオンレッド枠）
        DrawTriangle(bannerX - 8, bannerY + bannerH + 4, bannerX + 14, bannerY + 4, bannerX + bannerW + 12, bannerY + 4, colBlack, TRUE);
        DrawTriangle(bannerX - 8, bannerY + bannerH + 4, bannerX + bannerW + 12, bannerY + 4, bannerX + bannerW - 10, bannerY + bannerH + 4, colBlack, TRUE);

        unsigned int bannerFrameCol = gm.m_isTimeUp ? colNeonRed : colOrange;
        DrawTriangle(bannerX - 4, bannerY + bannerH, bannerX + 16, bannerY, bannerX + bannerW + 8, bannerY, bannerFrameCol, TRUE);
        DrawTriangle(bannerX - 4, bannerY + bannerH, bannerX + bannerW + 8, bannerY, bannerX + bannerW - 12, bannerY + bannerH, bannerFrameCol, TRUE);

        DrawTriangle(bannerX + 2, bannerY + bannerH - 4, bannerX + 20, bannerY + 4, bannerX + bannerW + 2, bannerY + 4, GetColor(20, 24, 38), TRUE);
        DrawTriangle(bannerX + 2, bannerY + bannerH - 4, bannerX + bannerW + 2, bannerY + 4, bannerX + bannerW - 16, bannerY + bannerH - 4, GetColor(20, 24, 38), TRUE);

        const char* clearLogo = gm.m_isTimeUp ? "★ TIME UP!! ★" : "★ STAGE CLEAR!! ★";
        int logoW = GetDrawStringWidthToHandle(clearLogo, static_cast<int>(strlen(clearLogo)), font36);
        int logoX = bannerX + (bannerW - logoW) / 2;
        int logoY = bannerY + 15;

        for (int dy = -3; dy <= 3; ++dy)
        {
            for (int dx = -3; dx <= 3; ++dx)
            {
                if (dx != 0 || dy != 0)
                {
                    DrawStringToHandle(logoX + dx, logoY + dy, clearLogo, colBlack, font36);
                }
            }
        }
        DrawStringToHandle(logoX + 3, logoY + 3, clearLogo, colBlack, font36);
        DrawStringToHandle(logoX, logoY + 1, clearLogo, gm.m_isTimeUp ? colNeonRed : colOrange, font36);
        DrawStringToHandle(logoX, logoY - 1, clearLogo, colYellow, font36);
    }

    // 左下：背景で猫が実践中の筋トレモーション名を表示するライブバッジ
    {
        Player3D::TitleWorkoutType clearWorkout = CalcActiveTitleWorkout(gm.m_clearAnimTimer);
        if (clearWorkout == Player3D::TitleWorkoutType::Idle)
        {
            clearWorkout = CalcActiveTitleWorkout((gm.m_clearAnimTimer / 270) * 270);
        }
        const char* motionName = "スクワット";
        if (clearWorkout == Player3D::TitleWorkoutType::PushUp) motionName = "腕立て伏せ";
        else if (clearWorkout == Player3D::TitleWorkoutType::SitUp) motionName = "腹筋";
        else if (clearWorkout == Player3D::TitleWorkoutType::PullUp) motionName = "懸垂";
        else if (clearWorkout == Player3D::TitleWorkoutType::SideRaise) motionName = "サイドレイズ";
        else if (clearWorkout == Player3D::TitleWorkoutType::DumbbellCurl) motionName = "ダンベルカール";
        else if (clearWorkout == Player3D::TitleWorkoutType::InclineBench) motionName = "インクラインベンチ";
        else if (clearWorkout == Player3D::TitleWorkoutType::BenchPress) motionName = "ベンチプレス";
        else if (clearWorkout == Player3D::TitleWorkoutType::InOverHeadPress) motionName = "インクラインOHP";

        char showcaseBuf[96];
        snprintf(showcaseBuf, sizeof(showcaseBuf), "WORKOUT SHOWCASE : %s", motionName);
        int swW = GetDrawStringWidthToHandle(showcaseBuf, static_cast<int>(strlen(showcaseBuf)), font16);
        int badgeX = 24;
        int badgeY = Config::SCREEN_HEIGHT - 56;
        SoftUI::DrawSoftCard(badgeX, badgeY, swW + 32, 36, 16, GetColor(18, 22, 36), 210, colGold, 1);
        DrawStringToHandle(badgeX + 16, badgeY + 9, showcaseBuf, colYellow, font16);
    }

    // ズームイン演出中（開始〜GameManager::CLEAR_ZOOM_DURATION フレーム）はまず左側の猫の筋トレ姿へフォーカス
    if (gm.m_clearAnimTimer <= GameManager::CLEAR_ZOOM_DURATION)
    {
        return;
    }

    int uiTimer = gm.m_clearAnimTimer - GameManager::CLEAR_ZOOM_DURATION;

    // =========================================================================
    // 2. スコア計算（残り時間・最大コンボ数・筋トレ成功回数・最大レベルの4項目を参照）
    //    合計スコアに応じて S / A / B / C の4段階にランク分け
    // =========================================================================
    auto player = gm.m_objManager.GetPlayer();
    float remainSec      = gm.m_remainingTimeSeconds;
    int   maxCombo       = gm.m_objManager.GetMaxComboCount();
    int   workoutSuccess = player ? player->GetWorkoutSuccessCount() : 0;
    int   maxLevel       = player ? player->GetMaxRepCount() : 0;

    int   caughtMice     = gm.m_objManager.GetCaughtCount();
    int   remainMice     = gm.m_objManager.GetRemainingMouseCount();
    int   totalMice      = caughtMice + remainMice;
    if (totalMice <= 0)
    {
        totalMice = Config::NORMAL_MOUSE_COUNT + Config::FAST_MOUSE_COUNT;
    }
    if (!gm.m_isTimeUp && remainMice == 0)
    {
        caughtMice = totalMice;
    }
    float catchRate      = (totalMice > 0) ? (static_cast<float>(caughtMice) / static_cast<float>(totalMice)) : 1.0f;
    catchRate            = MathHelper::Clamp(catchRate, 0.0f, 1.0f);
    int   caughtPct      = static_cast<int>(catchRate * 100.0f + 0.5f);

    // 4項目の個別スコア算出
    int timeScore     = static_cast<int>(remainSec * 50.0f + 0.5f); // 残り時間: 1秒につき 50 PTS (最大 6,000 PTS)
    int comboScore    = maxCombo * 500;                             // 最大コンボ数: 1コンボにつき 500 PTS
    int workoutScore  = workoutSuccess * 100;                       // ★ 筋トレ成功回数: 1回につき 100 PTS
    int levelScore    = maxLevel * 400;                             // 最大レベル: 1Lvにつき 400 PTS
    int rawTotalScore = timeScore + comboScore + workoutScore + levelScore;

    // ★ ネズミ全匹捕獲時が 100%、1匹少ないごとに総数に応じた割合 (1/totalMice) が引かれる！
    int totalScore    = static_cast<int>(static_cast<float>(rawTotalScore) * catchRate + 0.5f);

    // ランク閾値定義 (S: 15000+ / A: 11000+ / B: 7000+ / C: ~6999)
    constexpr int SCORE_RANK_S = 15000;
    constexpr int SCORE_RANK_A = 11000;
    constexpr int SCORE_RANK_B = 7000;

    const char*  rankLetter  = "C";
    const char*  rankTitle   = gm.m_isTimeUp ? "TIME UP..." : "ROOKIE CAT";
    unsigned int rankMainCol = GetColor(100, 215, 255);
    unsigned int rankSubCol  = GetColor(30, 130, 200);

    if (totalScore >= SCORE_RANK_S)
    {
        rankLetter  = "S";
        rankTitle   = "GOD MUSCLE CAT!!";
        rankMainCol = colGold;
        rankSubCol  = colOrange;
    }
    else if (totalScore >= SCORE_RANK_A)
    {
        rankLetter  = "A";
        rankTitle   = "GREAT MUSCLE!";
        rankMainCol = colOrange;
        rankSubCol  = colNeonRed;
    }
    else if (totalScore >= SCORE_RANK_B)
    {
        rankLetter  = "B";
        rankTitle   = "NICE PUMP!";
        rankMainCol = colYellow;
        rankSubCol  = GetColor(180, 140, 0);
    }

    // スコアのカウントアップ演出（UI表示開始から30フレームで0からtotalScoreまで上昇）
    float countUpRatio = MathHelper::Clamp(static_cast<float>(uiTimer) / 30.0f, 0.0f, 1.0f);
    int   dispTotalScore = static_cast<int>(totalScore * countUpRatio + 0.5f);

    // =========================================================================
    // 3. 右側メインパネル（X = 548 〜 1260）：スコア・ランク・内訳・ボタンを集約
    // =========================================================================
    int panelX = 548;
    int panelY = 18;
    int panelW = 712;
    int panelH = 684;

    // パネル外枠カード（角丸＋ソフトシャドウ）
    SoftUI::DrawSoftCard(panelX, panelY, panelW, panelH, 22, GetColor(18, 22, 36), 232, colYellow, 2);

    // パネル最上部ヘッダーバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 235);
    DrawRoundRect(panelX + 3, panelY + 3, panelX + panelW - 3, panelY + 46, 18, 18, colOrange, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(panelX + 16, panelY + 46, panelX + panelW - 16, panelY + 46, colBlack);

    DrawStringToHandle(panelX + 20, panelY + 11, "★ RESULT & SCORE RANKING ★", colBlack, font24);
    DrawStringToHandle(panelX + 18, panelY + 9,  "★ RESULT & SCORE RANKING ★", colWhite, font24);

    // ヘッダー右端：捕獲数バッジ
    char caughtHeaderBuf[64];
    snprintf(caughtHeaderBuf, sizeof(caughtHeaderBuf), "捕獲: %d / %d (%d%%)", caughtMice, totalMice, caughtPct);
    int chW = GetDrawStringWidthToHandle(caughtHeaderBuf, static_cast<int>(strlen(caughtHeaderBuf)), font16);
    int chX = panelX + panelW - chW - 24;
    SoftUI::DrawSoftCard(chX - 10, panelY + 8, chW + 20, 28, 10, GetColor(16, 20, 32), 230, colYellow, 1);
    DrawStringToHandle(chX, panelY + 13, caughtHeaderBuf, colYellow, font16);

    // =========================================================================
    // 4. メイン表示（通常: ランク＋特大TOTAL SCOREのみスッキリ表示 / 詳細ON時: 内訳4項目表示）
    // =========================================================================
    int earnedPoints = workoutSuccess * WorkoutCollectionManager::POINTS_PER_WORKOUT;
    int totalPoints  = WorkoutCollectionManager::GetInstance().GetProteinPoints();

    char scoreSubLabel[96];
    if (caughtMice >= totalMice)
    {
        snprintf(scoreSubLabel, sizeof(scoreSubLabel), "全匹捕獲: 100%% (%d/%d匹)", caughtMice, totalMice);
    }
    else
    {
        int missed = totalMice - caughtMice;
        int penaltyPct = 100 - caughtPct;
        snprintf(scoreSubLabel, sizeof(scoreSubLabel), "捕獲率 %d%% (%d/%d匹 : %d匹未捕獲で-%d%%)", caughtPct, caughtMice, totalMice, missed, penaltyPct);
    }

    char totalScoreStr[48];
    snprintf(totalScoreStr, sizeof(totalScoreStr), "%d PTS", dispTotalScore);

    float stampProgress = MathHelper::Clamp(uiTimer / 14.0f, 0.0f, 1.0f);
    float stampBounce = (uiTimer >= 14 && uiTimer < 26)
        ? std::sin((uiTimer - 14) * 0.5f) * 0.08f
        : 0.0f;
    float burstScale = stampProgress + stampBounce;

    if (!gm.m_showClearDetails)
    {
        // =====================================================================
        // 【シンプル表示モード】特大ランクスタンプ ＆ 超特大 TOTAL SCORE を中央に堂々配置
        // =====================================================================
        int heroY = panelY + 62;
        int heroH = 368;
        int boxX  = panelX + 20;
        int boxW  = panelW - 40;

        SoftUI::DrawSoftCard(boxX, heroY, boxW, heroH, 20, GetColor(12, 15, 26), 242, rankMainCol, 2, true, rankMainCol);

        // 上段：左にランクスタンプ、右に「TOTAL SCORE」＋超特大スコア数字
        int rankCX = boxX + 128;
        int rankCY = heroY + 118;

        constexpr int NUM_VERTICES = 24;
        for (int layer = 0; layer < 3; ++layer)
        {
            float rOuterBase = (layer == 0 ? 78.0f : (layer == 1 ? 70.0f : 56.0f)) * burstScale;
            float rInnerBase = (layer == 0 ? 54.0f : (layer == 1 ? 46.0f : 36.0f)) * burstScale;
            unsigned int layerCol = (layer == 0) ? colBlack : (layer == 1 ? ((rankLetter[0] == 'S') ? colNeonRed : colOrange) : colYellow);
            float angleOffset = (layer == 2) ? (-animSec * 0.30f + 0.2f) : (animSec * 0.22f);

            int vx[NUM_VERTICES];
            int vy[NUM_VERTICES];
            int ox = (layer == 0) ? 4 : 0;
            int oy = (layer == 0) ? 4 : 0;
            for (int i = 0; i < NUM_VERTICES; ++i)
            {
                float angle = angleOffset + (MathHelper::PI * 2.0f * i) / NUM_VERTICES;
                float r = (i % 2 == 0) ? rOuterBase : rInnerBase;
                vx[i] = rankCX + ox + static_cast<int>(std::cos(angle) * r);
                vy[i] = rankCY + oy + static_cast<int>(std::sin(angle) * r);
            }
            for (int i = 0; i < NUM_VERTICES; ++i)
            {
                int next = (i + 1) % NUM_VERTICES;
                DrawTriangle(rankCX + ox, rankCY + oy, vx[i], vy[i], vx[next], vy[next], layerCol, TRUE);
            }
        }

        int rLetterW = GetDrawStringWidthToHandle(rankLetter, static_cast<int>(strlen(rankLetter)), font96);
        int rLetterX = rankCX - rLetterW / 2;
        int rLetterY = rankCY - 48;
        for (int d = 3; d <= 6; ++d) DrawStringToHandle(rLetterX + d, rLetterY + d, rankLetter, colBlack, font96);
        for (int dy = -3; dy <= 3; ++dy)
            for (int dx = -3; dx <= 3; ++dx)
                if (dx != 0 || dy != 0) DrawStringToHandle(rLetterX + dx, rLetterY + dy, rankLetter, colBlack, font96);
        DrawStringToHandle(rLetterX, rLetterY + 2, rankLetter, rankSubCol, font96);
        DrawStringToHandle(rLetterX, rLetterY,     rankLetter, rankMainCol, font96);
        DrawStringToHandle(rLetterX, rLetterY - 2, rankLetter, colWhite, font96);

        // ランク称号リボン
        int ribbonW = 216;
        int ribbonH = 30;
        int ribbonX = rankCX - ribbonW / 2;
        int ribbonY = heroY + 206;
        DrawRoundRect(ribbonX, ribbonY, ribbonX + ribbonW, ribbonY + ribbonH, 8, 8, rankMainCol, TRUE);
        DrawRoundRect(ribbonX, ribbonY, ribbonX + ribbonW, ribbonY + ribbonH, 8, 8, colBlack, FALSE);
        int rTitleW = GetDrawStringWidthToHandle(rankTitle, static_cast<int>(strlen(rankTitle)), font16);
        DrawStringToHandle(rankCX - rTitleW / 2, ribbonY + 6, rankTitle, colBlack, font16);

        // 右側：超特大 TOTAL SCORE（font60 で大きく強調表示）
        int rightAreaX = boxX + 252;
        int rightAreaW = boxW - 270;

        DrawStringToHandle(rightAreaX, heroY + 34, "TOTAL SCORE", colGold, font24);
        DrawStringToHandle(rightAreaX, heroY + 66, scoreSubLabel, (caughtMice >= totalMice) ? GetColor(100, 245, 140) : GetColor(255, 130, 120), font16);

        int tsW = GetDrawStringWidthToHandle(totalScoreStr, static_cast<int>(strlen(totalScoreStr)), font60);
        int tsX = rightAreaX + (rightAreaW - tsW) / 2;
        int tsY = heroY + 104;
        DrawStringToHandle(tsX + 4, tsY + 4, totalScoreStr, colBlack, font60);
        DrawStringToHandle(tsX,     tsY,     totalScoreStr, colYellow, font60);

        // ランク到達プログレスバー（0 〜 17500 PTS）
        int barX = boxX + 28;
        int barY = heroY + 266;
        int barW = boxW - 56;
        int barH = 20;
        float maxBarScore = 17500.0f;
        float barRatio = MathHelper::Clamp(static_cast<float>(dispTotalScore) / maxBarScore, 0.0f, 1.0f);

        DrawRoundRect(barX, barY, barX + barW, barY + barH, 8, 8, GetColor(24, 30, 46), TRUE);
        int fillW = static_cast<int>(barW * barRatio);
        if (fillW > 4) DrawRoundRect(barX + 1, barY + 1, barX + fillW, barY + barH - 1, 7, 7, rankMainCol, TRUE);
        DrawRoundRect(barX, barY, barX + barW, barY + barH, 8, 8, GetColor(90, 110, 145), FALSE);

        int tickB = barX + static_cast<int>(barW * (SCORE_RANK_B / maxBarScore));
        int tickA = barX + static_cast<int>(barW * (SCORE_RANK_A / maxBarScore));
        int tickS = barX + static_cast<int>(barW * (SCORE_RANK_S / maxBarScore));
        DrawLine(tickB, barY - 4, tickB, barY + barH + 4, colYellow);
        DrawLine(tickA, barY - 4, tickA, barY + barH + 4, colOrange);
        DrawLine(tickS, barY - 4, tickS, barY + barH + 4, colGold);

        char critBuf[96];
        snprintf(critBuf, sizeof(critBuf), "S >= %d   |   A >= %d   |   B >= %d   |   C < %d",
                 SCORE_RANK_S, SCORE_RANK_A, SCORE_RANK_B, SCORE_RANK_B);
        int cW = GetDrawStringWidthToHandle(critBuf, static_cast<int>(strlen(critBuf)), font16);
        DrawStringToHandle(boxX + (boxW - cW) / 2, barY + 30, critBuf, GetColor(220, 230, 245), font16);

        // プロテインポイント獲得バナー（シンプル表示下部にコンパクト配置）
        int ptCardY = heroY + heroH + 14;
        int ptCardH = 52;
        SoftUI::DrawSoftCard(boxX, ptCardY, boxW, ptCardH, 12, GetColor(16, 22, 38), 245, colGold, 1);

        int pBadgeW = 96;
        int pBadgeH = 30;
        int pBadgeX = boxX + 14;
        int pBadgeY = ptCardY + (ptCardH - pBadgeH) / 2;
        SoftUI::DrawSoftCard(pBadgeX, pBadgeY, pBadgeW, pBadgeH, 8, colOrange, 255, colGold, 1);
        const char* pBadgeText = "PROTEIN";
        int pbW = GetDrawStringWidthToHandle(pBadgeText, static_cast<int>(strlen(pBadgeText)), font16);
        DrawStringToHandle(pBadgeX + (pBadgeW - pbW) / 2, pBadgeY + 7, pBadgeText, colBlack, font16);

        char earnedText[64];
        snprintf(earnedText, sizeof(earnedText), "獲得: +%d pt", earnedPoints);
        DrawStringToHandle(pBadgeX + pBadgeW + 18, ptCardY + 17, earnedText, colYellow, font18);

        char totalText[64];
        snprintf(totalText, sizeof(totalText), "総所持: %d pt", totalPoints);
        int ttW = GetDrawStringWidthToHandle(totalText, static_cast<int>(strlen(totalText)), font18);
        DrawStringToHandle(boxX + boxW - ttW - 18, ptCardY + 17, totalText, colGold, font18);
    }
    else
    {
        // =====================================================================
        // 【詳細表示モード】上段コンパクト要約 ＋ スコア内訳4項目 ＋ プロテイン情報
        // =====================================================================
        int rowX = panelX + 18;
        int rowW = panelW - 36;

        // 上段：コンパクトな総合スコアサマリーバー
        int sumY = panelY + 56;
        int sumH = 74;
        SoftUI::DrawSoftCard(rowX, sumY, rowW, sumH, 14, GetColor(12, 15, 26), 242, colGold, 1);
        DrawFormatStringToHandle(rowX + 18, sumY + 14, rankMainCol, font48, "%s", rankLetter);
        DrawStringToHandle(rowX + 76, sumY + 12, "TOTAL SCORE", colGold, font16);
        DrawStringToHandle(rowX + 76, sumY + 38, scoreSubLabel, (caughtMice >= totalMice) ? GetColor(100, 245, 140) : GetColor(255, 130, 120), font13);

        int tsW = GetDrawStringWidthToHandle(totalScoreStr, static_cast<int>(strlen(totalScoreStr)), font36);
        DrawStringToHandle(rowX + rowW - tsW - 18, sumY + 18, totalScoreStr, colYellow, font36);

        // スコア内訳4項目カード
        int rowH      = 64;
        int rowGap    = 8;
        int startRowY = sumY + sumH + 12;

        auto DrawScoreBreakdownRow = [&](int rowIndex, const char* title, const char* subFormula,
                                         const char* valueStr, int scorePts, unsigned int accentCol)
        {
            int ry = startRowY + rowIndex * (rowH + rowGap);

            SoftUI::DrawSoftCard(rowX, ry, rowW, rowH, 12, GetColor(12, 15, 26), 238, accentCol, 1);
            DrawRoundRect(rowX + 3, ry + 4, rowX + 9, ry + rowH - 4, 3, 3, accentCol, TRUE);

            DrawStringToHandle(rowX + 18, ry + 10, title, accentCol, font16);
            DrawStringToHandle(rowX + 18, ry + 36, subFormula, GetColor(170, 190, 215), font13);

            int pillW = 160;
            int pillH = 40;
            int pillX = rowX + rowW - pillW - 12;
            int pillY = ry + (rowH - pillH) / 2;
            SoftUI::DrawSoftCard(pillX, pillY, pillW, pillH, 10, GetColor(24, 30, 48), 245, accentCol, 1);

            int dispPts = static_cast<int>(scorePts * countUpRatio + 0.5f);
            char ptsBuf[32];
            snprintf(ptsBuf, sizeof(ptsBuf), "+%d PTS", dispPts);
            int pW = GetDrawStringWidthToHandle(ptsBuf, static_cast<int>(strlen(ptsBuf)), font24);
            int pX = pillX + (pillW - pW) / 2;
            int pY = pillY + (pillH - 24) / 2;
            DrawStringToHandle(pX + 2, pY + 2, ptsBuf, colBlack, font24);
            DrawStringToHandle(pX,     pY,     ptsBuf, colYellow, font24);

            int valW = GetDrawStringWidthToHandle(valueStr, static_cast<int>(strlen(valueStr)), font24);
            int valX = pillX - 16 - valW;
            int valY = ry + (rowH - 24) / 2;
            DrawStringToHandle(valX + 2, valY + 2, valueStr, colBlack, font24);
            DrawStringToHandle(valX,     valY,     valueStr, colWhite, font24);
        };

        int totalCentisec = static_cast<int>(remainSec * 100.0f + 0.5f);
        int minutes  = totalCentisec / 6000;
        int seconds  = (totalCentisec / 100) % 60;
        int centisec = totalCentisec % 100;
        char timeValStr[32];
        snprintf(timeValStr, sizeof(timeValStr), "%02d:%02d.%02d", minutes, seconds, centisec);
        char timeSubStr[96];
        snprintf(timeSubStr, sizeof(timeSubStr), "クリアタイム: %.2f秒 (残り1秒 × 50 PTS)", gm.m_clearTimeSeconds);
        DrawScoreBreakdownRow(0, "【TIME】 残り時間 (TIME LEFT)", timeSubStr, timeValStr, timeScore, colYellow);

        char comboValStr[32];
        snprintf(comboValStr, sizeof(comboValStr), "%d COMBO", maxCombo);
        DrawScoreBreakdownRow(1, "【COMBO】 最大コンボ数 (MAX COMBO)", "ネズミ連続捕獲数 (1コンボ × 500 PTS)", comboValStr, comboScore, colCyan);

        char workoutValStr[32];
        snprintf(workoutValStr, sizeof(workoutValStr), "%d 回", workoutSuccess);
        DrawScoreBreakdownRow(2, "【PUMP】 筋トレ成功回数 (WORKOUT SUCCESS)", "筋トレQTE成功 (1回 × 100 PTS)", workoutValStr, workoutScore, colGreen);

        char lvValStr[32];
        snprintf(lvValStr, sizeof(lvValStr), "Lv. %d", maxLevel);
        DrawScoreBreakdownRow(3, "【LEVEL】 最大レベル (MAX LEVEL)", "最高到達マッスルレベル (1Lv × 400 PTS)", lvValStr, levelScore, colOrange);

        // プロテイン獲得サマリー
        int ptY = startRowY + 4 * (rowH + rowGap) + 6;
        char ptSummary[96];
        snprintf(ptSummary, sizeof(ptSummary), "★ 獲得プロテイン: +%d pt  /  総所持: %d pt", earnedPoints, totalPoints);
        int ptsW = GetDrawStringWidthToHandle(ptSummary, static_cast<int>(strlen(ptSummary)), font16);
        DrawStringToHandle(rowX + (rowW - ptsW) / 2, ptY + 4, ptSummary, colGold, font16);
    }

    // =========================================================================
    // 5. 詳細表示トグルボタン（RETRY / MAIN MENU の直上に配置）
    // =========================================================================
    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    int detBtnX = 568;
    int detBtnY = 524;
    int detBtnW = 672;
    int detBtnH = 44;
    bool hoverDetail = (mx >= detBtnX && mx <= detBtnX + detBtnW && my >= detBtnY && my <= detBtnY + detBtnH);

    SoftUI::DrawSoftCard(detBtnX, detBtnY, detBtnW, detBtnH, 14,
                         hoverDetail ? GetColor(48, 68, 108) : GetColor(26, 36, 58), 245,
                         hoverDetail ? colYellow : colCyan, hoverDetail ? 2 : 1,
                         hoverDetail, colCyan);

    const char* detLabel = gm.m_showClearDetails
        ? "▲ スコア詳細を閉じる  (クリック / [I] キー)"
        : "▼ スコア詳細を表示  (クリック / [I] キー)";
    int dW = GetDrawStringWidthToHandle(detLabel, static_cast<int>(strlen(detLabel)), font18);
    DrawStringToHandle(detBtnX + (detBtnW - dW) / 2, detBtnY + 13, detLabel, hoverDetail ? colYellow : colWhite, font18);

    // =========================================================================
    // 6. アクションボタン（右パネル最下部：RETRY / MAIN MENU）
    // =========================================================================
    int btnW   = 322;
    int btnH   = 58;
    int btnGap = 28;
    int btn1X  = 568;
    int btn2X  = btn1X + btnW + btnGap;
    int btnY   = 584;

    auto DrawArcadeButton = [&](int bx, int by, bool isSelected, const char* label, const char* keyBadge, unsigned int activeMainCol)
    {
        float floatY = isSelected ? (std::sin(animSec * 6.0f) * 3.0f) : 0.0f;
        int curBy = by + static_cast<int>(floatY);

        unsigned int fillCol   = isSelected ? activeMainCol : GetColor(40, 48, 70);
        unsigned int borderCol = isSelected ? colGold : GetColor(70, 82, 115);

        SoftUI::DrawSoftCard(bx, curBy, btnW, btnH, 24, fillCol, 255, borderCol, 2, isSelected, borderCol);

        if (isSelected)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(bx + 6, curBy + 3, bx + btnW - 6, curBy + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 左側：入力対応キーアイコンバッジ
        int keyW = 68;
        int keyH = 36;
        int keyX = bx + 14;
        int keyY = curBy + (btnH - keyH) / 2;
        SoftUI::DrawSoftCard(keyX, keyY, keyW, keyH, 10, colNeonRed, 255, colWhite, 1);

        int kbTextW = GetDrawStringWidthToHandle(keyBadge, static_cast<int>(strlen(keyBadge)), font18);
        DrawStringToHandle(keyX + (keyW - kbTextW) / 2, keyY + 9, keyBadge, colWhite, font18);

        // 右側：ボタンメインラベル
        int labelAreaX = keyX + keyW + 10;
        int labelAreaW = (bx + btnW - 10) - labelAreaX;
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
        DrawStringToHandle(lbX,     lbY,     label, isSelected ? colWhite : GetColor(215, 225, 245), font24);
    };

    DrawArcadeButton(btn1X, btnY, (gm.m_clearSelectedBtn == 0), "RETRY !!",  "[R]", colOrange);
    DrawArcadeButton(btn2X, btnY, (gm.m_clearSelectedBtn == 1), "MAIN MENU", "[M]", colYellow);
}

void UIManager::DrawTitleScreen(GameManager& gm)
{
    auto& collMgr = WorkoutCollectionManager::GetInstance();
    // ★ ガチャ演出中はタイトルの通常ボタン群（STARTやSTAGE等）を隠し、実演する3D猫と獲得カードのみを最高に際立たせる！
    if (gm.m_showWorkoutCollection && collMgr.IsGachaAnimationActive())
    {
        DrawWorkoutCollectionModal(gm);
        return;
    }

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
        float aL = -0.72f + std::sin(gm.m_titleAnimTimer * 0.016f) * 0.32f;
        int lx1 = 0, ly1 = Config::SCREEN_HEIGHT;
        int lx2 = static_cast<int>(std::cos(aL - 0.12f) * 1300.0f);
        int ly2 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aL - 0.12f) * 1300.0f);
        int lx3 = static_cast<int>(std::cos(aL + 0.12f) * 1300.0f);
        int ly3 = Config::SCREEN_HEIGHT + static_cast<int>(std::sin(aL + 0.12f) * 1300.0f);
        DrawTriangle(lx1, ly1, lx2, ly2, lx3, ly3, GetColor(60, 200, 255), TRUE);

        // 右サーチライト（ゴールデンアンバー）
        float aR = -2.42f + std::sin(gm.m_titleAnimTimer * 0.019f + 1.2f) * 0.32f;
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
    float sunAngleOuter = gm.m_titleAnimTimer * 0.005f;
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
    float sunAngleInner = -gm.m_titleAnimTimer * 0.008f;
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
        float pX = std::fmod(static_cast<float>(i * 57 + 35) + std::sin(gm.m_titleAnimTimer * 0.02f + static_cast<float>(i)) * 22.0f, static_cast<float>(Config::SCREEN_WIDTH));
        float pY = static_cast<float>(Config::SCREEN_HEIGHT) - std::fmod(static_cast<float>(i * 42) + gm.m_titleAnimTimer * 1.25f, static_cast<float>(Config::SCREEN_HEIGHT + 30));

        const char* pGlyph = (i % 5 == 0) ? "★" : (i % 5 == 1 ? "☆" : (i % 5 == 2 ? "▲" : (i % 5 == 3 ? "◆" : "●")));
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

    const char* helpStr = "[ ↑ / ↓ ] 選択   [ ← / → ] ステージ切替   [ ENTER / SPACE ] 決定   [ H / TAB ] あそびかた   [ O ] 設定   [ C ] 筋トレ";
    int helpW = GetDrawStringWidthToHandle(helpStr, static_cast<int>(strlen(helpStr)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - helpW) / 2, Config::SCREEN_HEIGHT - 32, helpStr, GetColor(235, 245, 255), font16);
    DrawStringToHandle(Config::SCREEN_WIDTH - 85, Config::SCREEN_HEIGHT - 30, "VER 1.0", GetColor(100, 220, 255), font13);

    // ------------------------------------------------------------------------
    // 5. タイトルロゴ背面の半透明コミックプレート（コンパクトに収めて背景の視界を確保）
    // ------------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
    DrawBox(0, 68, Config::SCREEN_WIDTH, 178, GetColor(15, 18, 30), TRUE);
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
    int glintCycle = gm.m_titleAnimTimer % 180;
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
        float badgePulse = std::sin(gm.m_titleAnimTimer * 0.08f) * 4.0f;

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
        // 3. 腹筋（シットアップ）中
        static const char* sitUpShouts[] = {
            "腹筋が6LDK！板チョコを超えた！",
            "シックスパックがダイヤモンド！",
            "腹直筋でクルミが割れる！",
            "体幹が鉄筋コンクリート！",
            "ニャイスシットアップ！！",
            "お腹のキレがカミソリ級！",
            "腹筋運動で竜巻が起きるぞ！",
            "究極のシックスパックにゃんこ！"
        };
        // 4. 懸垂（プルアップ）中
        static const char* pullUpShouts[] = {
            "広背筋に翼が生えてるぞ！",
            "背中で語れるマッスル猫！",
            "空気を掴んで懸垂してる！？",
            "背中に鬼神が宿ってる！",
            "ニャイスプルアップ！！",
            "広背筋がムササビ級！",
            "重力すら引き寄せる背中！",
            "逆三角形の黄金シルエット！"
        };
        // 5. インターバル・ポーズ中
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

        Player3D::TitleWorkoutType curWorkout = CalcActiveTitleWorkout(gm.m_titleAnimTimer);
        const char** activePool = poseShouts;
        int activeCount = sizeof(poseShouts) / sizeof(poseShouts[0]);

        if (curWorkout == Player3D::TitleWorkoutType::Squat)
        {
            activePool = squatShouts;
            activeCount = sizeof(squatShouts) / sizeof(squatShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::PushUp)
        {
            activePool = pushUpShouts;
            activeCount = sizeof(pushUpShouts) / sizeof(pushUpShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::SitUp)
        {
            activePool = sitUpShouts;
            activeCount = sizeof(sitUpShouts) / sizeof(sitUpShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::PullUp)
        {
            activePool = pullUpShouts;
            activeCount = sizeof(pullUpShouts) / sizeof(pullUpShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::SideRaise)
        {
            static const char* sideShouts[] = {
                "肩メロン！！巨大メロン！",
                "三角筋が羽ばたいてる！",
                "サイドレイズで翼を授ける！",
                "横への広がりが凄まじい！"
            };
            activePool = sideShouts;
            activeCount = sizeof(sideShouts) / sizeof(sideShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::DumbbellCurl)
        {
            static const char* curlShouts[] = {
                "上腕二頭筋がチョモランマ！",
                "二頭筋が山脈！",
                "力こぶチョモランマ！",
                "腕が太くて毛づくろい不可！"
            };
            activePool = curlShouts;
            activeCount = sizeof(curlShouts) / sizeof(curlShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::InclineBench)
        {
            static const char* incShouts[] = {
                "大胸筋上部が爆発！",
                "インクラインで盛り上がる！",
                "鎖骨の下が分厚い！",
                "角度がいい！最高だ！"
            };
            activePool = incShouts;
            activeCount = sizeof(incShouts) / sizeof(incShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::BenchPress)
        {
            static const char* bpShouts[] = {
                "大胸筋がまな板！",
                "胸が分厚すぎて前が見えねえ！",
                "ベンチプレス最高！",
                "バーベルが軽すぎる！"
            };
            activePool = bpShouts;
            activeCount = sizeof(bpShouts) / sizeof(bpShouts[0]);
        }
        else if (curWorkout == Player3D::TitleWorkoutType::InOverHeadPress)
        {
            static const char* ohpShouts[] = {
                "肩にキャノン砲！",
                "天を突く肩幅！",
                "天井に届くプレス！",
                "ダンベルが空を飛ぶ！"
            };
            activePool = ohpShouts;
            activeCount = sizeof(ohpShouts) / sizeof(ohpShouts[0]);
        }

        // --- 掛け声吹き出し（左右各1箇所に厳選し、画面のごちゃつきを解消してテンポよく入れ替え） ---
        const int SHOUT_SPAN = 180; // 約3秒スパン (60fps * 3s)

        struct BubbleSlot
        {
            bool isRight;
            int baseY;
            int timerOffset;
            unsigned int bgCol;
            unsigned int textCol;
        };

        BubbleSlot slots[2] = {
            // 左側（パステルイエロー・バター）
            { false, 310,   0, GetColor(255, 240, 140), GetColor(35, 30, 20) },
            // 右側（サクラピンク）
            { true,  310,  90, GetColor(255, 185, 215), GetColor(45, 20, 35) }
        };

        for (int i = 0; i < 2; ++i)
        {
            const auto& slot = slots[i];
            int timer = gm.m_titleAnimTimer + slot.timerOffset;
            int step = timer / SHOUT_SPAN;
            int frame = timer % SHOUT_SPAN;

            // アニメーション：3秒ごとの切り替え時に弾むポップバウンス
            float bounce = (frame < 18) ? (std::sin((static_cast<float>(frame) / 18.0f) * MathHelper::PI) * -8.0f) : 0.0f;
            // ふわふわ浮遊アニメーション
            float floatY = std::sin(gm.m_titleAnimTimer * 0.045f + i * 1.05f) * 3.5f;

            int idx = (step * 5 + i * 3) % activeCount;
            const char* shoutText = activePool[idx];

            int textW = GetDrawStringWidthToHandle(shoutText, static_cast<int>(strlen(shoutText)), font16);
            int sW = textW + 30;
            int sH = 38;
            int sX = slot.isRight ? (Config::SCREEN_WIDTH - sW - 36) : 36;
            int sY = slot.baseY + static_cast<int>(floatY + bounce);

            // ふわふわの雲・マシュマロ吹き出しを描画
            SoftUI::DrawFluffySpeechBubble(sX, sY, sW, sH, 18, slot.bgCol, slot.textCol, slot.isRight, shoutText, font16);
        }
    }

    // ------------------------------------------------------------------------
    // 8. ふわふわカプセル・メニューボタン（5項目：START, STAGE, 説明, 音量, 筋トレ）
    // ------------------------------------------------------------------------
    int btnW = 380;
    int btnH = 38;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;
    int startBtnY = 360;
    int stageBtnY = 406;
    int howBtnY   = 452;
    int soundBtnY = 498;
    int collBtnY  = 544;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: GAME START ---
    bool isStartSelected = (gm.m_selectedMenuItem == 0);
    {
        float floatY = isStartSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = startBtnY + static_cast<int>(floatY);

        if (isStartSelected)
        {
            unsigned int neonColor = ((gm.m_titleAnimTimer / 15) % 2 == 0) ? GetColor(255, 220, 60) : GetColor(255, 140, 40);
            unsigned int bodyColor = GetColor(255, 220 + static_cast<int>(30 * btnPulse), 35);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            // 上部ハイライトピル
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 元気な矢印アイコン
            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 13, ">", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 13, "<", GetColor(25, 25, 30), font24);

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
    bool isStageSelected = (gm.m_selectedMenuItem == 1);
    const char* stageNameStr = (gm.m_selectedMap == ModelConfig::MapType::House)
        ? "STAGE : MAP 1 (HOUSE)"
        : "STAGE : MAP 2 (NEW MAP)";
    {
        float floatY = isStageSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = stageBtnY + static_cast<int>(floatY);

        if (isStageSelected)
        {
            unsigned int neonColor = ((gm.m_titleAnimTimer / 15) % 2 == 0) ? GetColor(255, 170, 50) : GetColor(255, 70, 50);
            unsigned int bodyColor = GetColor(255, 140 + static_cast<int>(30 * btnPulse), 45);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int arrowPulse = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 4.0f);
            DrawStringToHandle(btnX + 22 - arrowPulse, curY + 13, "<", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 + arrowPulse, curY + 13, ">", GetColor(25, 25, 30), font24);

            int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, stageNameStr, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 13, stageNameStr, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            DrawStringToHandle(btnX + 22, curY + 13, "<", GetColor(140, 170, 210), font24);
            DrawStringToHandle(btnX + btnW - 34, curY + 13, ">", GetColor(140, 170, 210), font24);

            int tW = GetDrawStringWidthToHandle(stageNameStr, static_cast<int>(strlen(stageNameStr)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 14, stageNameStr, GetColor(210, 230, 255), font24);
        }
    }

    // --- ボタン3: ゲーム説明 ---
    bool isHowSelected = (gm.m_selectedMenuItem == 2);
    {
        float floatY = isHowSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = howBtnY + static_cast<int>(floatY);

        if (isHowSelected)
        {
            unsigned int neonColor = ((gm.m_titleAnimTimer / 15) % 2 == 0) ? GetColor(80, 220, 255) : GetColor(60, 150, 255);
            unsigned int bodyColor = GetColor(80, 220 + static_cast<int>(30 * btnPulse), 255);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 27, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 13, ">", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 13, "<", GetColor(25, 25, 30), font24);

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

    // --- ボタン4: 設定 (BGM/SE音量・マウス感度) ---
    bool isSoundSelected = (gm.m_selectedMenuItem == 3);
    {
        float floatY = isSoundSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = soundBtnY + static_cast<int>(floatY);

        if (isSoundSelected)
        {
            unsigned int neonColor = ((gm.m_titleAnimTimer / 15) % 2 == 0) ? GetColor(160, 255, 120) : GetColor(80, 220, 100);
            unsigned int bodyColor = GetColor(100, 225 + static_cast<int>(30 * btnPulse), 110);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 11, ">", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 11, "<", GetColor(25, 25, 30), font24);

            const char* soundTxt = "設定 (O) !!";
            int tW = GetDrawStringWidthToHandle(soundTxt, static_cast<int>(strlen(soundTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, soundTxt, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 11, soundTxt, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 22, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            const char* soundTxt = "設定 (O)";
            int tW = GetDrawStringWidthToHandle(soundTxt, static_cast<int>(strlen(soundTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, soundTxt, GetColor(210, 230, 255), font24);
        }
    }

    // --- ボタン5: 筋トレコレクション (ガチャ・セーブ・コード) ---
    bool isCollSelected = (gm.m_selectedMenuItem == 4);
    {
        float floatY = isCollSelected ? (std::sin(GetNowCount() * 0.0055f) * 3.0f) : 0.0f;
        int curY = collBtnY + static_cast<int>(floatY);

        if (isCollSelected)
        {
            unsigned int neonColor = ((gm.m_titleAnimTimer / 15) % 2 == 0) ? GetColor(255, 120, 220) : GetColor(200, 70, 255);
            unsigned int bodyColor = GetColor(190, 60 + static_cast<int>(30 * btnPulse), 220);
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 24, bodyColor, 255, neonColor, 2, true, neonColor);

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
            DrawRoundRect(btnX + 6, curY + 3, btnX + btnW - 6, curY + 16, 12, 12, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            int bounceX = static_cast<int>(std::sin(GetNowCount() * 0.015f) * 5.0f);
            DrawStringToHandle(btnX + 22 + bounceX, curY + 11, ">", GetColor(25, 25, 30), font24);
            DrawStringToHandle(btnX + btnW - 42 - bounceX, curY + 11, "<", GetColor(25, 25, 30), font24);

            const char* collTxt = "筋トレコレクション (C) !!";
            int tW = GetDrawStringWidthToHandle(collTxt, static_cast<int>(strlen(collTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, collTxt, GetColor(20, 20, 30), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 11, collTxt, GetColor(0, 0, 0), font24);
        }
        else
        {
            SoftUI::DrawSoftCard(btnX, curY, btnW, btnH, 22, GetColor(30, 40, 62), 225, GetColor(60, 80, 120), 1);

            const char* collTxt = "筋トレコレクション (C)";
            int tW = GetDrawStringWidthToHandle(collTxt, static_cast<int>(strlen(collTxt)), font24);
            DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, curY + 12, collTxt, GetColor(210, 230, 255), font24);
        }
    }

    // ------------------------------------------------------------------------
    // 10. 遊び方モーダルが開いていれば描画
    // ------------------------------------------------------------------------
    if (gm.m_showHowToPlay)
    {
        DrawHowToPlayModal(gm);
    }

    // サウンド設定モーダルが開いていれば描画
    if (gm.m_showSoundSettings)
    {
        DrawSoundSettingsModal(gm);
    }

    // 筋トレコレクションモーダルが開いていれば描画
    if (gm.m_showWorkoutCollection)
    {
        DrawWorkoutCollectionModal(gm);
    }

    // ------------------------------------------------------------------------
    // 11. ゲーム開始ホワイトトランジション
    // ------------------------------------------------------------------------
    if (gm.m_startTransitionTimer > 0)
    {
        float alphaRatio = (22.0f - gm.m_startTransitionTimer) / 22.0f;
        int alphaVal = static_cast<int>(alphaRatio * 255.0f);
        if (alphaVal > 255) alphaVal = 255;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaVal);
        DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 筋トレコレクション確認ダイアログ（最前面描画）
    WorkoutCollectionManager::GetInstance().DrawConfirmDialogModal(font24, font16);
}

void UIManager::DrawHowToPlayModal(GameManager& gm)
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    float animSec = GetNowCount() / 1000.0f;

    auto DrawCenteredStr = [&](int boxX, int boxW, int y, const char* text, unsigned int col, int fontHandle)
    {
        int tw = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), fontHandle);
        DrawStringToHandle(boxX + (boxW - tw) / 2, y, text, col, fontHandle);
    };

    // ------------------------------------------------------------------------
    // 視覚的3Dキーボードキー（キーキャップ）描画ヘルパーラムダ
    // ------------------------------------------------------------------------
    auto DrawKeycap = [&](int x, int y, int w, int h, const char* label, bool isHighlighted, unsigned int highlightCol, int fontHandle)
    {
        int pressOffset = isHighlighted ? 3 : 0;

        // 1. キー底面の立体ベベル（厚み）
        unsigned int bottomCol = isHighlighted ? GetColor(20, 45, 35) : GetColor(10, 14, 24);
        DrawRoundRect(x, y + 4, x + w, y + h + 4, 9, 9, bottomCol, TRUE);

        // 2. ハイライト時の外周発光グロー
        if (isHighlighted)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 95);
            DrawRoundRect(x - 3, y + pressOffset - 3, x + w + 3, y + h + pressOffset + 3, 11, 11, highlightCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 3. キートップ表面
        unsigned int topBg     = isHighlighted ? highlightCol : GetColor(38, 48, 72);
        unsigned int topBorder = isHighlighted ? GetColor(255, 255, 255) : GetColor(110, 135, 180);
        DrawRoundRect(x, y + pressOffset, x + w, y + h + pressOffset, 8, 8, topBg, TRUE);
        DrawRoundRect(x, y + pressOffset, x + w, y + h + pressOffset, 8, 8, topBorder, FALSE);

        // キートップ内側のツヤ線
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, isHighlighted ? 90 : 45);
        DrawRoundRect(x + 3, y + pressOffset + 2, x + w - 3, y + pressOffset + 10, 5, 5, GetColor(255, 255, 255), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 4. キー文字ラベル（中央揃え）
        int tw = GetDrawStringWidthToHandle(label, static_cast<int>(strlen(label)), fontHandle);
        int tx = x + (w - tw) / 2;
        int fontH = (fontHandle == font24) ? 24 : (fontHandle == font18 ? 18 : 14);
        int ty = y + pressOffset + (h - fontH) / 2;
        unsigned int txtCol = isHighlighted ? GetColor(15, 20, 28) : GetColor(240, 245, 255);
        DrawStringToHandle(tx, ty, label, txtCol, fontHandle);
    };

    // ------------------------------------------------------------------------
    // 視覚的マウスアイコン（左クリック・右クリック・マウス移動）描画ヘルパーラムダ
    // ------------------------------------------------------------------------
    auto DrawMouseDiagram = [&](int cx, int cy, bool activeLeft, bool activeRight, bool activeMove)
    {
        int mw = 60;
        int mh = 80;
        int mx1 = cx - mw / 2;
        int my1 = cy - mh / 2;
        int mx2 = cx + mw / 2;
        int my2 = cy + mh / 2;

        // マウス移動時の左右スイング矢印
        if (activeMove)
        {
            int swing = static_cast<int>(std::sin(animSec * 6.0f) * 4.0f);
            DrawStringToHandle(mx1 - 24 - swing, cy - 9, "<", GetColor(100, 230, 255), font18);
            DrawStringToHandle(mx2 + 6 + swing,  cy - 9, ">", GetColor(100, 230, 255), font18);
        }

        // マウス本体ベース
        DrawRoundRect(mx1 - 2, my1 - 2, mx2 + 2, my2 + 2, 22, 22, GetColor(110, 135, 180), TRUE);
        DrawRoundRect(mx1, my1, mx2, my2, 20, 20, GetColor(34, 42, 64), TRUE);

        // 左クリックボタン領域
        unsigned int lCol = activeLeft ? GetColor(255, 85, 85) : GetColor(52, 64, 92);
        DrawRoundRect(mx1 + 3, my1 + 3, cx - 2, cy - 4, 12, 12, lCol, TRUE);
        if (activeLeft)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
            DrawCircle(mx1 + 15, my1 + 16, 16, GetColor(255, 90, 90), FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 右クリックボタン領域
        unsigned int rCol = activeRight ? GetColor(255, 155, 40) : GetColor(52, 64, 92);
        DrawRoundRect(cx + 2, my1 + 3, mx2 - 3, cy - 4, 12, 12, rCol, TRUE);
        if (activeRight)
        {
            SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
            DrawCircle(mx2 - 15, my1 + 16, 16, GetColor(255, 165, 50), FALSE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 中央分割線＆ホイール
        DrawLine(cx, my1 + 3, cx, cy - 2, GetColor(20, 24, 38), 2);
        DrawLine(mx1 + 3, cy - 3, mx2 - 3, cy - 3, GetColor(20, 24, 38), 2);
        DrawRoundRect(cx - 4, my1 + 9, cx + 4, my1 + 25, 4, 4, GetColor(210, 225, 245), TRUE);
    };

    // 1. 全画面暗転レイヤー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 215);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダルウィンドウ本体 (1040x620)
    int modalW = 1040;
    int modalH = 620;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 22, GetColor(16, 20, 32), 245, GetColor(255, 140, 45), 2);

    // ヘッダーバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 50, 20, 20, GetColor(30, 40, 66), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 20, modalY + 50, modalX + modalW - 20, modalY + 50, GetColor(255, 140, 45));

    const char* pageTitles[GameManager::HOW_TO_PLAY_PAGE_COUNT] = {
        "ゲーム説明 [1/5] : ゲームの目的・基本移動・カメラ操作",
        "ゲーム説明 [2/5] : 筋トレで猫をパワーアップ！ (SPACEキー)",
        "ゲーム説明 [3/5] : Lv.4で解放！ 2つの必殺アクション",
        "ゲーム説明 [4/5] : 筋トレコレクション ＆ ガチャで新種目解放！",
        "ゲーム説明 [5/5] : ネズミの種類と高ランク(S)攻略のコツ"
    };
    DrawCenteredStr(modalX, modalW, modalY + 13, pageTitles[gm.m_howToPlayPage], GetColor(255, 230, 75), font24);

    // 右上「×」閉じるボタン
    int topCloseX = modalX + modalW - 48;
    int topCloseY = modalY + 8;
    bool hoverTopClose = (mx >= topCloseX && mx <= topCloseX + 36 && my >= topCloseY && my <= topCloseY + 36);
    SoftUI::DrawSoftCard(topCloseX, topCloseY, 36, 36, 10,
                         hoverTopClose ? GetColor(245, 70, 70) : GetColor(60, 40, 52), 240,
                         hoverTopClose ? GetColor(255, 200, 200) : GetColor(180, 90, 100), 1);
    DrawCenteredStr(topCloseX, 36, topCloseY + 6, "×", GetColor(255, 255, 255), font24);

    // ------------------------------------------------------------------------
    // 3. 上部ステップタブバー（クリックで直接ページ移動可能）
    // ------------------------------------------------------------------------
    const char* tabLabels[GameManager::HOW_TO_PLAY_PAGE_COUNT] = {
        "① 目的・移動・視点",
        "② 筋トレで強化",
        "③ 必殺アクション",
        "④ 筋トレコレクション",
        "⑤ ネズミ・攻略"
    };
    unsigned int tabAccentCols[GameManager::HOW_TO_PLAY_PAGE_COUNT] = {
        GetColor(80, 220, 255),
        GetColor(100, 245, 140),
        GetColor(255, 150, 45),
        GetColor(255, 120, 240),
        GetColor(255, 220, 60)
    };

    int tabY      = modalY + 58;
    int tabH      = 38;
    int tabW      = 192;
    int tabGap    = 8;
    int tabStartX = modalX + 24;

    for (int i = 0; i < GameManager::HOW_TO_PLAY_PAGE_COUNT; ++i)
    {
        int tx = tabStartX + i * (tabW + tabGap);
        bool isCurTab   = (gm.m_howToPlayPage == i);
        bool isHoverTab = (mx >= tx && mx <= tx + tabW && my >= tabY && my <= tabY + tabH);

        unsigned int bgCol = isCurTab ? GetColor(38, 52, 84) : (isHoverTab ? GetColor(28, 36, 58) : GetColor(20, 25, 40));
        unsigned int bdCol = isCurTab ? tabAccentCols[i] : GetColor(65, 80, 115);
        SoftUI::DrawSoftCard(tx, tabY, tabW, tabH, 12, bgCol, 245, bdCol, isCurTab ? 2 : 1, isCurTab, tabAccentCols[i]);

        DrawCenteredStr(tx, tabW, tabY + 11, tabLabels[i],
                        isCurTab ? tabAccentCols[i] : GetColor(185, 205, 235), font16);
    }

    // ------------------------------------------------------------------------
    // 4. メインコンテンツ領域（文字とキー/マウス図解のみでゆったり配置）
    // ------------------------------------------------------------------------
    int contentX = modalX + 24;
    int contentY = modalY + 108;
    int contentW = modalW - 48; // 992px

    if (gm.m_howToPlayPage == 0)
    {
        // --- 上段：ゲームの目的カード（簡潔化） ---
        SoftUI::DrawSoftCard(contentX, contentY, contentW, 74, 14, GetColor(22, 28, 44), 240, GetColor(80, 220, 255), 1);
        DrawCenteredStr(contentX, contentW, contentY + 14,
                        "■ 目的 ： 120秒以内にネズミに体当たりして全員捕まえろ！",
                        GetColor(255, 235, 80), font18);
        DrawCenteredStr(contentX, contentW, contentY + 44,
                        "最初はネズミの方が速いので、[SPACE] の筋トレでスピードUPしよう！",
                        GetColor(100, 245, 140), font16);

        // --- 下段左：WASDキー移動図解カード ---
        int boxY = contentY + 88;
        int boxW = 486;
        int boxH = 352;
        SoftUI::DrawSoftCard(contentX, boxY, boxW, boxH, 14, GetColor(22, 28, 44), 240, GetColor(80, 220, 255), 1);
        DrawStringToHandle(contentX + 20, boxY + 16, "■ 移動 ： [W] [A] [S] [D] キー", GetColor(80, 220, 255), font18);

        int activeKey = (GetNowCount() / 650) % 4; // 0:W, 1:D, 2:S, 3:A
        int kw = 66, kh = 56, kgap = 12;
        int kBaseX = contentX + (boxW - (kw * 3 + kgap * 2)) / 2;
        int kBaseY = boxY + 90;

        DrawCenteredStr(kBaseX + kw + kgap, kw, kBaseY - 22, "▲前進", GetColor(180, 235, 255), font13);
        DrawKeycap(kBaseX + kw + kgap,       kBaseY,             kw, kh, "W", (activeKey == 0), GetColor(80, 235, 255), font24);
        DrawKeycap(kBaseX,                   kBaseY + kh + kgap, kw, kh, "A", (activeKey == 3), GetColor(80, 235, 255), font24);
        DrawKeycap(kBaseX + kw + kgap,       kBaseY + kh + kgap, kw, kh, "S", (activeKey == 2), GetColor(80, 235, 255), font24);
        DrawKeycap(kBaseX + (kw + kgap) * 2, kBaseY + kh + kgap, kw, kh, "D", (activeKey == 1), GetColor(80, 235, 255), font24);

        int bottomLabelY = kBaseY + kh * 2 + kgap + 14;
        DrawCenteredStr(kBaseX,                   kw, bottomLabelY, "←左",   GetColor(180, 235, 255), font13);
        DrawCenteredStr(kBaseX + kw + kgap,       kw, bottomLabelY, "▼後退", GetColor(180, 235, 255), font13);
        DrawCenteredStr(kBaseX + (kw + kgap) * 2, kw, bottomLabelY, "右→",   GetColor(180, 235, 255), font13);

        SoftUI::DrawSoftCard(contentX + 24, boxY + 268, boxW - 48, 56, 10, GetColor(16, 22, 36), 235, GetColor(60, 130, 180), 1);
        DrawCenteredStr(contentX + 24, boxW - 48, boxY + 288, "カメラが向いている方向へ走るぞ！", GetColor(100, 245, 140), font18);

        // --- 下段右：カメラ視点操作カード ---
        int rightBoxX = contentX + 506;
        SoftUI::DrawSoftCard(rightBoxX, boxY, boxW, boxH, 14, GetColor(22, 28, 44), 240, GetColor(100, 220, 255), 1);
        DrawStringToHandle(rightBoxX + 20, boxY + 16, "■ 視点 ： マウス移動 ＆ [F] リセット", GetColor(100, 220, 255), font18);

        int mouseCx = rightBoxX + 110;
        DrawMouseDiagram(mouseCx, boxY + 136, false, false, true);
        DrawCenteredStr(rightBoxX + 20, 180, boxY + 196, "マウス / 矢印で見回す", GetColor(180, 230, 255), font13);

        int camX = rightBoxX + 220;
        bool fHighlight = ((GetNowCount() / 900) % 2 == 0);
        DrawKeycap(camX, boxY + 90, 52, 44, "F", fHighlight, GetColor(255, 220, 70), font18);
        DrawStringToHandle(camX + 66, boxY + 102, "正面へ視点リセット", GetColor(255, 230, 90), font16);

        DrawKeycap(camX,      boxY + 162, 44, 38, "-", false, GetColor(100, 220, 255), font18);
        DrawKeycap(camX + 52, boxY + 162, 44, 38, "+", false, GetColor(100, 220, 255), font18);
        DrawStringToHandle(camX + 108, boxY + 172, "マウス感度変更", GetColor(255, 255, 255), font16);

        SoftUI::DrawSoftCard(rightBoxX + 24, boxY + 268, boxW - 48, 56, 10, GetColor(16, 22, 36), 235, GetColor(60, 130, 180), 1);
        DrawCenteredStr(rightBoxX + 24, boxW - 48, boxY + 288, "見失ったら [F] キーで視点を戻そう！", GetColor(255, 230, 90), font18);
    }
    else if (gm.m_howToPlayPage == 1)
    {
        // --- 上段：SPACEキー長押し＆緑・金ゾーン解放の図解 ---
        int topH = 220;
        SoftUI::DrawSoftCard(contentX, contentY, contentW, topH, 14, GetColor(22, 28, 44), 240, GetColor(100, 245, 140), 1);
        DrawStringToHandle(contentX + 20, contentY + 14, "■ 筋トレ ： [SPACE] 長押し → 緑・金ゾーンで離す！", GetColor(100, 245, 140), font18);

        float qteCycle    = std::fmod(animSec, 3.2f);
        bool  isHolding   = (qteCycle < 1.9f);
        float needleRatio = (qteCycle >= 2.0f) ? 0.70f : (qteCycle / 2.0f) * 0.70f;

        int spX = contentX + 44;
        int spY = contentY + 52;
        int spW = 290;
        int spH = 48;
        DrawKeycap(spX, spY, spW, spH, "SPACE (長押し)", isHolding, GetColor(80, 245, 130), font18);

        int stepBoxX = spX + spW + 24;
        int stepBoxW = contentW - (stepBoxX - contentX) - 44;
        if (isHolding)
        {
            SoftUI::DrawSoftCard(stepBoxX, spY + 2, stepBoxW, 46, 12, GetColor(25, 70, 45), 245, GetColor(100, 255, 150), 1);
            DrawCenteredStr(stepBoxX, stepBoxW, spY + 15, "① [SPACE] 長押しで針を進める！", GetColor(120, 255, 160), font18);
        }
        else
        {
            SoftUI::DrawSoftCard(stepBoxX, spY + 2, stepBoxW, 46, 12, GetColor(95, 70, 15), 245, GetColor(255, 230, 50), 2);
            DrawCenteredStr(stepBoxX, stepBoxW, spY + 15, "② 金ゾーンで離せば一気に +2 レベルUP！！", GetColor(255, 240, 60), font18);
        }

        // 連動する拡大ゲージ図解（緑ゾーン＋中央の金ゾーンを描画！）
        int gBarX = contentX + 96;
        int gBarY = contentY + 142;
        int gBarW = contentW - 192; // 800px
        int gBarH = 30;
        DrawRoundRect(gBarX - 2, gBarY - 2, gBarX + gBarW + 2, gBarY + gBarH + 2, 14, 14, GetColor(255, 140, 45), TRUE);
        DrawRoundRect(gBarX, gBarY, gBarX + gBarW, gBarY + gBarH, 12, 12, GetColor(28, 34, 52), TRUE);

        // 緑の成功ゾーン (0.58 〜 0.82)
        int gz1 = gBarX + static_cast<int>(gBarW * 0.58f);
        int gz2 = gBarX + static_cast<int>(gBarW * 0.82f);
        DrawRoundRect(gz1, gBarY + 2, gz2, gBarY + gBarH - 2, 6, 6, GetColor(45, 225, 115), TRUE);

        // ★ 中央の金ゾーン (0.66 〜 0.74)
        int pz1 = gBarX + static_cast<int>(gBarW * 0.66f);
        int pz2 = gBarX + static_cast<int>(gBarW * 0.74f);
        DrawRoundRect(pz1 - 1, gBarY + 1, pz2 + 1, gBarY + gBarH - 1, 5, 5, GetColor(255, 215, 20), TRUE);
        DrawRoundRect(pz1, gBarY + 1, pz2, gBarY + gBarH - 1, 5, 5, GetColor(255, 255, 210), FALSE);

        DrawCenteredStr(gz1 - 60, (gz2 - gz1) + 120, gBarY - 22, "▼ 緑：Lv.+1  ／  ★ 中央の金：Lv.+2 (PERFECT!) ★", GetColor(255, 235, 50), font16);

        int gNeedleX = gBarX + static_cast<int>(gBarW * needleRatio);
        DrawRoundRect(gNeedleX - 4, gBarY - 6, gNeedleX + 4, gBarY + gBarH + 6, 4, 4,
                      (!isHolding) ? GetColor(255, 235, 40) : GetColor(255, 255, 255), TRUE);

        DrawCenteredStr(contentX, contentW, gBarY + 40,
                        "緑ゾーンで Lv.+1 ＆ +100pt！ ど真ん中の【金ゾーン】なら一気に Lv.+2 ＆ +200pt！！",
                        GetColor(255, 235, 80), font16);

        // --- 下段：成功・失敗・放置の3ルールカード（簡潔化） ---
        int botY = contentY + 234;
        int botH = 206;
        SoftUI::DrawSoftCard(contentX, botY, contentW, botH, 14, GetColor(22, 28, 44), 240, GetColor(255, 180, 60), 1);
        DrawStringToHandle(contentX + 20, botY + 14, "■ 筋トレの3つのルール （まずは Lv.4 を目指そう！）", GetColor(255, 220, 70), font18);

        int colW   = 306;
        int colH   = 140;
        int colGap = 17;
        int colY   = botY + 48;

        // 1. 成功（緑＆金）
        int c1X = contentX + 20;
        SoftUI::DrawSoftCard(c1X, colY, colW, colH, 12, GetColor(20, 48, 36), 235, GetColor(90, 240, 130), 1);
        DrawCenteredStr(c1X, colW, colY + 14, "○ 成功 ： スピードUP！", GetColor(100, 255, 145), font18);
        DrawCenteredStr(c1X, colW, colY + 54, "・緑＝Lv.+1 / 金＝Lv.+2！", GetColor(255, 235, 60), font16);
        DrawCenteredStr(c1X, colW, colY + 92, "・Lv.4で必殺技が使える！", GetColor(255, 255, 255), font16);

        // 2. 失敗
        int c2X = c1X + colW + colGap;
        SoftUI::DrawSoftCard(c2X, colY, colW, colH, 12, GetColor(52, 24, 30), 235, GetColor(255, 95, 95), 1);
        DrawCenteredStr(c2X, colW, colY + 14, "× 失敗 ： 5秒間「筋肉痛」", GetColor(255, 110, 110), font18);
        DrawCenteredStr(c2X, colW, colY + 54, "・ゾーン外で離すと失敗", GetColor(245, 225, 225), font16);
        DrawCenteredStr(c2X, colW, colY + 92, "・5秒間動けなくなる！", GetColor(255, 160, 160), font16);

        // 3. 放置減衰
        int c3X = c2X + colW + colGap;
        SoftUI::DrawSoftCard(c3X, colY, colW, colH, 12, GetColor(52, 36, 20), 235, GetColor(255, 165, 60), 1);
        DrawCenteredStr(c3X, colW, colY + 14, "▲ 放置 ： 15秒で「Lv.-3」", GetColor(255, 185, 70), font18);
        DrawCenteredStr(c3X, colW, colY + 54, "・15秒放置でレベル減少", GetColor(245, 235, 215), font16);
        DrawCenteredStr(c3X, colW, colY + 92, "・こまめに鍛えて維持！", GetColor(255, 220, 90), font16);
    }
    else if (gm.m_howToPlayPage == 2)
    {
        // --- 上段：Lv.4必殺アクション概要バナー（簡潔化） ---
        SoftUI::DrawSoftCard(contentX, contentY, contentW, 64, 14, GetColor(22, 28, 44), 240, GetColor(255, 160, 50), 1);
        DrawCenteredStr(contentX, contentW, contentY + 22,
                        "■ Lv.4 以上になると、2つの必殺アクションが解放！（高Lvほど強化）",
                        GetColor(255, 230, 80), font18);

        float skillCycle = std::fmod(animSec, 6.0f);
        bool  isStunDemo = (skillCycle >= 3.0f);

        int boxY = contentY + 80;
        int boxW = 486;
        int boxH = 360;

        // --- 下段左：必殺技① 飛びつき捕獲 ---
        SoftUI::DrawSoftCard(contentX, boxY, boxW, boxH, 14, GetColor(26, 24, 40), 240,
                             !isStunDemo ? GetColor(255, 90, 90) : GetColor(120, 70, 80), !isStunDemo ? 2 : 1);
        DrawStringToHandle(contentX + 20, boxY + 16, "■ 必殺技① ： 飛びつき捕獲 （Lv.4〜）", GetColor(255, 105, 105), font18);

        int m1Cx = contentX + 114;
        DrawMouseDiagram(m1Cx, boxY + 120, true, false, false);
        DrawCenteredStr(m1Cx - 60, 120, boxY + 174, "左クリック", GetColor(255, 115, 115), font16);

        DrawCenteredStr(contentX + 188, 70, boxY + 114, "または", GetColor(190, 205, 230), font16);

        int sKeyX = contentX + 276;
        DrawKeycap(sKeyX, boxY + 96, 120, 48, "SHIFT", !isStunDemo, GetColor(255, 95, 95), font18);
        DrawCenteredStr(sKeyX, 120, boxY + 174, "SHIFT キー", GetColor(210, 225, 245), font16);

        SoftUI::DrawSoftCard(contentX + 20, boxY + 222, boxW - 40, 114, 12, GetColor(18, 20, 34), 240, GetColor(180, 75, 85), 1);
        DrawCenteredStr(contentX + 20, boxW - 40, boxY + 244, "前方へ大ジャンプして一気に捕獲！", GetColor(255, 230, 90), font18);
        DrawCenteredStr(contentX + 20, boxW - 40, boxY + 286, "逃げるネズミとの距離を一瞬で詰めるぞ！", GetColor(255, 255, 255), font16);

        // --- 下段右：必殺技② スタン突進攻撃 ---
        int rightBoxX = contentX + 506;
        SoftUI::DrawSoftCard(rightBoxX, boxY, boxW, boxH, 14, GetColor(26, 24, 40), 240,
                             isStunDemo ? GetColor(255, 160, 45) : GetColor(120, 90, 60), isStunDemo ? 2 : 1);
        DrawStringToHandle(rightBoxX + 20, boxY + 16, "■ 必殺技② ： スタン突進 （Lv.4〜）", GetColor(255, 170, 55), font18);

        int m2Cx = rightBoxX + 114;
        DrawMouseDiagram(m2Cx, boxY + 120, false, true, false);
        DrawCenteredStr(m2Cx - 60, 120, boxY + 174, "右クリック", GetColor(255, 175, 60), font16);

        DrawCenteredStr(rightBoxX + 188, 70, boxY + 114, "または", GetColor(190, 205, 230), font16);

        int eKeyX = rightBoxX + 292;
        DrawKeycap(eKeyX, boxY + 96, 84, 48, "E", isStunDemo, GetColor(255, 165, 45), font24);
        DrawCenteredStr(eKeyX - 18, 120, boxY + 174, "E キー", GetColor(210, 225, 245), font16);

        SoftUI::DrawSoftCard(rightBoxX + 20, boxY + 222, boxW - 40, 114, 12, GetColor(18, 20, 34), 240, GetColor(180, 120, 55), 1);
        DrawCenteredStr(rightBoxX + 20, boxW - 40, boxY + 244, "高速タックルで周囲のネズミを気絶！", GetColor(255, 230, 90), font18);
        DrawCenteredStr(rightBoxX + 20, boxW - 40, boxY + 286, "※壁に激突すると猫が気絶するので注意！", GetColor(255, 130, 130), font16);
    }
    else if (gm.m_howToPlayPage == 3)
    {
        // --------------------------------------------------------------------
        // [4/5] 筋トレコレクション ＆ ガチャ解放システム（簡潔化）
        // --------------------------------------------------------------------
        int bannerH = 64;
        SoftUI::DrawSoftCard(contentX, contentY, contentW, bannerH, 14, GetColor(36, 18, 38), 245, GetColor(255, 120, 240), 1);
        DrawCenteredStr(contentX, contentW, contentY + 22,
                        "■ 筋トレ成功で「プロテインpt」獲得！ 100ptで【★ガチャ】を回して新種目解放！",
                        GetColor(255, 230, 90), font18);

        // --- 中段左：システム解説カード（シンプル3項目） ---
        int midY = contentY + 78;
        int leftCardW = 430;
        int midH = 276;
        SoftUI::DrawSoftCard(contentX, midY, leftCardW, midH, 14, GetColor(24, 20, 36), 240, GetColor(255, 130, 230), 1);
        DrawStringToHandle(contentX + 18, midY + 16, "★ コレクションの使い方", GetColor(255, 140, 235), font18);

        int item1Y = midY + 54;
        SoftUI::DrawSoftCard(contentX + 16, item1Y, leftCardW - 32, 62, 10, GetColor(34, 26, 48), 235, GetColor(210, 100, 230), 1);
        DrawStringToHandle(contentX + 28, item1Y + 12, "● タイトル ＆ ポーズ画面 [ESC] で開く", GetColor(255, 235, 90), font16);
        DrawStringToHandle(contentX + 28, item1Y + 36, "いつでもガチャや設定が可能！", GetColor(220, 215, 240), font13);

        int item2Y = item1Y + 72;
        SoftUI::DrawSoftCard(contentX + 16, item2Y, leftCardW - 32, 62, 10, GetColor(34, 26, 48), 235, GetColor(210, 100, 230), 1);
        DrawStringToHandle(contentX + 28, item2Y + 12, "● 筋トレの【ON / OFF】を自由切替", GetColor(100, 250, 180), font16);
        DrawStringToHandle(contentX + 28, item2Y + 36, "ONにした種目がゲーム中に再生される！", GetColor(220, 215, 240), font13);

        int item3Y = item2Y + 72;
        SoftUI::DrawSoftCard(contentX + 16, item3Y, leftCardW - 32, 62, 10, GetColor(34, 26, 48), 235, GetColor(210, 100, 230), 1);
        DrawStringToHandle(contentX + 28, item3Y + 12, "● 自動セーブ対応で安心", GetColor(100, 220, 255), font16);
        DrawStringToHandle(contentX + 28, item3Y + 36, "集めた種目とptは自動でPCに保存！", GetColor(220, 215, 240), font13);

        // --- 中段右：全9種目リスト（3×3グリッド） ---
        int rightCardX = contentX + leftCardW + 14;
        int rightCardW = contentW - leftCardW - 14; // 548px
        SoftUI::DrawSoftCard(rightCardX, midY, rightCardW, midH, 14, GetColor(22, 24, 40), 240, GetColor(255, 160, 60), 1);
        DrawStringToHandle(rightCardX + 18, midY + 16, "■ 全9種類の筋トレ種目", GetColor(255, 185, 75), font18);

        struct MotionGuideItem {
            const char* name;
            const char* tag;
            unsigned int tagCol;
        };
        const MotionGuideItem motions[9] = {
            { "腕立て伏せ",      "★初期所持", GetColor(100, 255, 150) },
            { "腹筋 (シット)",   "ガチャ解放", GetColor(255, 190, 80) },
            { "スクワット",      "ガチャ解放", GetColor(255, 190, 80) },
            { "懸垂 (プル)",     "ガチャ解放", GetColor(255, 190, 80) },
            { "サイドレイズ",    "ガチャ解放", GetColor(255, 190, 80) },
            { "ダンベルカール",  "ガチャ解放", GetColor(255, 190, 80) },
            { "インクライン",    "ガチャ解放", GetColor(255, 190, 80) },
            { "ベンチプレス",    "ガチャ解放", GetColor(255, 190, 80) },
            { "インクラインOHP", "ガチャ解放", GetColor(255, 190, 80) }
        };

        int gridCols = 3;
        int itemGapX = 8;
        int itemGapY = 10;
        int gridStartX = rightCardX + 14;
        int gridStartY = midY + 52;
        int itemW = (rightCardW - 28 - itemGapX * 2) / gridCols; // 168px
        int itemH = 62;

        for (int i = 0; i < 9; ++i)
        {
            int r = i / gridCols;
            int c = i % gridCols;
            int ix = gridStartX + c * (itemW + itemGapX);
            int iy = gridStartY + r * (itemH + itemGapY);

            bool isInitial = (i == 0);
            unsigned int cardBg = isInitial ? GetColor(24, 46, 36) : GetColor(28, 30, 48);
            unsigned int cardBd = isInitial ? GetColor(80, 220, 130) : GetColor(70, 80, 115);
            SoftUI::DrawSoftCard(ix, iy, itemW, itemH, 8, cardBg, 240, cardBd, 1);

            DrawCenteredStr(ix, itemW, iy + 12, motions[i].name, isInitial ? GetColor(120, 255, 170) : GetColor(255, 255, 255), font16);
            DrawCenteredStr(ix, itemW, iy + 38, motions[i].tag, motions[i].tagCol, font13);
        }

        // --- 下段：ワンポイントカード ---
        int botY = midY + midH + 12;
        int botH = 68;
        SoftUI::DrawSoftCard(contentX, botY, contentW, botH, 14, GetColor(22, 28, 44), 240, GetColor(100, 245, 140), 1);
        DrawCenteredStr(contentX, contentW, botY + 24,
                        "★ お気に入りの筋トレだけを【ON】にしてプレイすることもできるぞ！",
                        GetColor(100, 255, 160), font18);
    }
    else
    {
        // --- 上段：ネズミの種類と特徴（簡潔化） ---
        int topH = 204;
        SoftUI::DrawSoftCard(contentX, contentY, contentW, topH, 14, GetColor(22, 28, 44), 240, GetColor(255, 220, 60), 1);
        DrawStringToHandle(contentX + 20, contentY + 14, "■ ネズミの特徴", GetColor(255, 220, 60), font18);

        int colW   = 306;
        int colH   = 136;
        int colGap = 17;
        int colY   = contentY + 50;

        int c1X = contentX + 20;
        SoftUI::DrawSoftCard(c1X, colY, colW, colH, 12, GetColor(20, 36, 54), 235, GetColor(100, 210, 255), 1);
        DrawCenteredStr(c1X, colW, colY + 16, "● 通常ネズミ", GetColor(110, 225, 255), font18);
        DrawCenteredStr(c1X, colW, colY + 58, "・近づくと反対方向へ逃げる", GetColor(255, 255, 255), font16);
        DrawCenteredStr(c1X, colW, colY + 94, "・まずはこいつから捕獲！", GetColor(180, 235, 255), font16);

        int c2X = c1X + colW + colGap;
        SoftUI::DrawSoftCard(c2X, colY, colW, colH, 12, GetColor(52, 24, 30), 235, GetColor(255, 100, 100), 1);
        DrawCenteredStr(c2X, colW, colY + 16, "★ 高速ネズミ", GetColor(255, 115, 115), font18);
        DrawCenteredStr(c2X, colW, colY + 58, "・逃げ足がとても速い！", GetColor(255, 255, 255), font16);
        DrawCenteredStr(c2X, colW, colY + 94, "・筋トレでLv.4以上に強化！", GetColor(255, 230, 80), font16);

        int c3X = c2X + colW + colGap;
        SoftUI::DrawSoftCard(c3X, colY, colW, colH, 12, GetColor(48, 38, 18), 235, GetColor(255, 190, 50), 1);
        DrawCenteredStr(c3X, colW, colY + 16, "▲ 終盤のスピードUP", GetColor(255, 215, 70), font18);
        DrawCenteredStr(c3X, colW, colY + 58, "・残り匹数が減るとさらに加速", GetColor(255, 255, 255), font16);
        DrawCenteredStr(c3X, colW, colY + 94, "・終盤はスタン突進で足止め！", GetColor(100, 245, 140), font16);

        // --- 下段：Sランク（15,000pt〜）攻略3ステップ ＆ 便利キー ---
        int botY = contentY + 218;
        int botH = 222;
        SoftUI::DrawSoftCard(contentX, botY, contentW, botH, 14, GetColor(22, 28, 44), 240, GetColor(100, 245, 140), 1);
        DrawStringToHandle(contentX + 20, botY + 14, "■ Sランク（15,000 PTS〜）攻略の3ステップ ＆ 便利キー", GetColor(100, 245, 140), font18);

        int stepW = 284, stepH = 74, stepY = botY + 50;
        int s1X = contentX + 20;
        int s2X = contentX + 354;
        int s3X = contentX + 688;

        SoftUI::DrawSoftCard(s1X, stepY, stepW, stepH, 10, GetColor(24, 48, 38), 240, GetColor(100, 245, 140), 1);
        DrawCenteredStr(s1X, stepW, stepY + 14, "STEP 1 ： 金ゾーン筋トレ", GetColor(100, 255, 145), font16);
        DrawCenteredStr(s1X, stepW, stepY + 44, "一気に Lv.4 以上へ強化！", GetColor(255, 255, 255), font16);

        DrawCenteredStr(s1X + stepW, s2X - (s1X + stepW), stepY + 26, "→", GetColor(255, 220, 60), font24);

        SoftUI::DrawSoftCard(s2X, stepY, stepW, stepH, 10, GetColor(48, 34, 22), 240, GetColor(255, 160, 50), 1);
        DrawCenteredStr(s2X, stepW, stepY + 14, "STEP 2 ： 連続コンボ捕獲", GetColor(255, 175, 60), font16);
        DrawCenteredStr(s2X, stepW, stepY + 44, "素早く連続で捕まえて高得点！", GetColor(255, 255, 255), font16);

        DrawCenteredStr(s2X + stepW, s3X - (s2X + stepW), stepY + 26, "→", GetColor(255, 220, 60), font24);

        SoftUI::DrawSoftCard(s3X, stepY, stepW, stepH, 10, GetColor(24, 38, 58), 240, GetColor(100, 220, 255), 1);
        DrawCenteredStr(s3X, stepW, stepY + 14, "STEP 3 ： 高レベル維持", GetColor(100, 230, 255), font16);
        DrawCenteredStr(s3X, stepW, stepY + 44, "高Lv＆残り時間で15,000pt突破！", GetColor(255, 235, 80), font16);

        // 下段：便利ショートカットキーキャップ一覧
        int ky = botY + 148;
        DrawKeycap(contentX + 40, ky, 76, 44, "ESC", false, GetColor(255, 140, 50), font18);
        DrawStringToHandle(contentX + 134, ky + 14, "ポーズ・設定画面を開く", GetColor(255, 255, 255), font16);

        DrawKeycap(contentX + 540, ky, 56, 44, "H", false, GetColor(100, 220, 255), font18);
        DrawStringToHandle(contentX + 614, ky + 14, "このゲーム説明を開閉", GetColor(255, 255, 255), font16);
    }

    // ------------------------------------------------------------------------
    // 5. 下部中央：左右矢印ページ切り替えバー ＆ 右下：閉じるボタン
    // ------------------------------------------------------------------------
    int centerX  = Config::SCREEN_WIDTH / 2;
    int navY     = modalY + modalH - 54;
    int prevBtnW = 156;
    int prevBtnH = 42;
    int prevBtnX = centerX - 246;
    int prevBtnY = navY;

    int nextBtnW = 156;
    int nextBtnH = 42;
    int nextBtnX = centerX + 90;
    int nextBtnY = navY;

    bool hoverPrev = (mx >= prevBtnX && mx <= prevBtnX + prevBtnW && my >= prevBtnY && my <= prevBtnY + prevBtnH);
    bool hoverNext = (mx >= nextBtnX && mx <= nextBtnX + nextBtnW && my >= nextBtnY && my <= nextBtnY + nextBtnH);

    int arrowPulse = static_cast<int>(std::sin(animSec * 8.0f) * 3.0f);

    // 左矢印ボタン（前のページ）
    unsigned int prevBg = hoverPrev ? GetColor(60, 165, 245) : GetColor(32, 48, 78);
    unsigned int prevBd = hoverPrev ? GetColor(180, 240, 255) : GetColor(90, 150, 220);
    SoftUI::DrawSoftCard(prevBtnX, prevBtnY, prevBtnW, prevBtnH, 21, prevBg, 250, prevBd, 2, hoverPrev, GetColor(80, 200, 255));
    DrawStringToHandle(prevBtnX + 16 - arrowPulse, prevBtnY + 12, "<", GetColor(255, 235, 80), font18);
    DrawCenteredStr(prevBtnX + 42, prevBtnW - 48, prevBtnY + 13, "前のページ", GetColor(255, 255, 255), font16);

    // 中央ページ番号 ＆ ページインジケータードット
    char pageNumBuf[32];
    snprintf(pageNumBuf, sizeof(pageNumBuf), "%d / %d ページ", gm.m_howToPlayPage + 1, GameManager::HOW_TO_PLAY_PAGE_COUNT);
    DrawCenteredStr(centerX - 80, 160, navY + 2, pageNumBuf, GetColor(255, 235, 90), font18);

    int dotStartX = centerX - ((GameManager::HOW_TO_PLAY_PAGE_COUNT - 1) * 26) / 2;
    int dotY      = navY + 32;
    for (int i = 0; i < GameManager::HOW_TO_PLAY_PAGE_COUNT; ++i)
    {
        int dx = dotStartX + i * 26;
        if (i == gm.m_howToPlayPage)
        {
            DrawRoundRect(dx - 10, dotY - 5, dx + 10, dotY + 5, 5, 5, tabAccentCols[i], TRUE);
        }
        else
        {
            DrawCircle(dx, dotY, 5, GetColor(90, 110, 150), TRUE);
        }
    }

    // 右矢印ボタン（次のページ）
    unsigned int nextBg = hoverNext ? GetColor(60, 165, 245) : GetColor(32, 48, 78);
    unsigned int nextBd = hoverNext ? GetColor(180, 240, 255) : GetColor(90, 150, 220);
    SoftUI::DrawSoftCard(nextBtnX, nextBtnY, nextBtnW, nextBtnH, 21, nextBg, 250, nextBd, 2, hoverNext, GetColor(80, 200, 255));
    DrawCenteredStr(nextBtnX + 6, nextBtnW - 48, nextBtnY + 13, "次のページ", GetColor(255, 255, 255), font16);
    DrawStringToHandle(nextBtnX + nextBtnW - 34 + arrowPulse, nextBtnY + 12, ">", GetColor(255, 235, 80), font18);

    // 左下ヒントラベル
    DrawStringToHandle(modalX + 24, navY + 14, "[←/→] [A/D] キーでもページ切替", GetColor(165, 190, 225), font13);

    // 右下：閉じるボタン
    int closeBtnW = 154;
    int closeBtnH = 42;
    int closeBtnX = modalX + modalW - closeBtnW - 24;
    int closeBtnY = navY;

    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW &&
                       my >= closeBtnY && my <= closeBtnY + closeBtnH);

    unsigned int closeBg     = hoverClose ? GetColor(255, 75, 75) : GetColor(210, 45, 45);
    unsigned int closeBorder = hoverClose ? GetColor(255, 200, 200) : GetColor(240, 100, 100);
    SoftUI::DrawSoftCard(closeBtnX, closeBtnY, closeBtnW, closeBtnH, 21, closeBg, 245, closeBorder, 1, hoverClose, GetColor(255, 80, 80));
    DrawCenteredStr(closeBtnX, closeBtnW, closeBtnY + 13, "閉じる (ESC)", GetColor(255, 255, 255), font16);
}
// ============================================================================
// ポーズ中の更新処理（ESC / リトライ / タイトルに戻る / マウス感度調整）

void UIManager::DrawPauseModal(GameManager& gm)
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

    // 2. モーダル本体（文字はみ出し・ボタン密集を完全解消したゆったり寸法）
    int modalW = 640;
    int modalH = 594;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 22, GetColor(20, 24, 38), 240, GetColor(255, 125, 35), 2);

    // ヘッダーバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 44, 20, 20, GetColor(32, 42, 68), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 20, modalY + 44, modalX + modalW - 20, modalY + 44, GetColor(255, 125, 35));

    const char* pauseHeader = "PAUSE / 設定";
    int hW = GetDrawStringWidthToHandle(pauseHeader, static_cast<int>(strlen(pauseHeader)), font24);
    DrawStringToHandle((Config::SCREEN_WIDTH - hW) / 2, modalY + 10, pauseHeader, GetColor(255, 220, 60), font24);

    int btnW = 460;
    int btnX = (Config::SCREEN_WIDTH - btnW) / 2;

    int retryBtnY    = modalY + 50;
    int retryBtnH    = 34;
    int titleBtnY    = modalY + 90;
    int titleBtnH    = 34;
    int sensRowY     = modalY + 130;
    int sensRowH     = 52;
    int bgmRowY      = modalY + 188;
    int bgmRowH      = 52;
    int seRowY       = modalY + 246;
    int seRowH       = 52;
    int workoutCardW = 604;
    int workoutCardX = (Config::SCREEN_WIDTH - workoutCardW) / 2;
    int workoutCardY = modalY + 306;
    int workoutCardH = 172;

    float btnPulse = (std::sin(GetNowCount() * 0.01f) + 1.0f) * 0.5f;

    // --- ボタン1: リトライ ---
    bool isRetrySelected = (gm.m_pauseMenuItem == 0);
    if (isRetrySelected)
    {
        unsigned int col = GetColor(100, 230 + static_cast<int>(25 * btnPulse), 120);
        SoftUI::DrawSoftCard(btnX, retryBtnY, btnW, retryBtnH, 22, col, 255, GetColor(160, 255, 180), 2, true, GetColor(100, 240, 130));
        DrawStringToHandle(btnX + 22, retryBtnY + 6, ">", GetColor(20, 20, 20), font24);
        const char* retryTxt = "リトライ (R)";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 7, retryTxt, GetColor(15, 25, 20), font24);
    }
    else
    {
        SoftUI::DrawSoftCard(btnX, retryBtnY, btnW, retryBtnH, 22, GetColor(32, 42, 65), 235, GetColor(60, 75, 110), 1);
        DrawStringToHandle(btnX + 22, retryBtnY + 6, ">", GetColor(140, 170, 210), font24);
        const char* retryTxt = "リトライ";
        int tW = GetDrawStringWidthToHandle(retryTxt, static_cast<int>(strlen(retryTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, retryBtnY + 7, retryTxt, GetColor(210, 230, 255), font24);
    }

    // --- ボタン2: タイトルに戻る ---
    bool isTitleSelected = (gm.m_pauseMenuItem == 1);
    if (isTitleSelected)
    {
        unsigned int col = GetColor(70, 190 + static_cast<int>(35 * btnPulse), 255);
        SoftUI::DrawSoftCard(btnX, titleBtnY, btnW, titleBtnH, 22, col, 255, GetColor(160, 230, 255), 2, true, GetColor(80, 200, 255));
        DrawStringToHandle(btnX + 22, titleBtnY + 6, ">", GetColor(20, 20, 20), font24);
        const char* titleTxt = "タイトルに戻る (T)";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 7, titleTxt, GetColor(15, 20, 30), font24);
    }
    else
    {
        SoftUI::DrawSoftCard(btnX, titleBtnY, btnW, titleBtnH, 22, GetColor(32, 42, 65), 235, GetColor(60, 75, 110), 1);
        DrawStringToHandle(btnX + 22, titleBtnY + 6, ">", GetColor(140, 170, 210), font24);
        const char* titleTxt = "タイトルに戻る";
        int tW = GetDrawStringWidthToHandle(titleTxt, static_cast<int>(strlen(titleTxt)), font24);
        DrawStringToHandle((Config::SCREEN_WIDTH - tW) / 2, titleBtnY + 7, titleTxt, GetColor(210, 230, 255), font24);
    }

    int mx, my;
    GetMousePoint(&mx, &my);
    int minusBtnX  = btnX + 10;
    int minusBtnW  = 46;
    int minusBtnH  = 22;
    int plusBtnX   = btnX + btnW - 56;
    int plusBtnW   = 46;
    int plusBtnH   = 22;

    // --- コントロール3: マウス感度 ---
    bool isSensSelected = (gm.m_pauseMenuItem == 2);
    float currentSens = gm.m_camera.GetMouseSensitivity();
    char sensValStr[32];
    snprintf(sensValStr, sizeof(sensValStr), "%.1fx", currentSens);
    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    int sensMinusY = sensRowY + 24;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, sensRowY, btnW, sensRowH,
        isSensSelected, "マウス感度", sensValStr, sensRatio,
        (currentSens >= 2.0f) ? GetColor(255, 120, 60) : ((currentSens >= 1.0f) ? GetColor(100, 220, 255) : GetColor(70, 180, 240)),
        hoverSensMinus, hoverSensPlus, font16, font13
    );

    // --- コントロール4: BGM音量 ---
    bool isBgmSelected = (gm.m_pauseMenuItem == 3);
    int bgmPercent = SoundManager::GetInstance().GetBGMVolumePercent();
    char bgmValStr[32];
    snprintf(bgmValStr, sizeof(bgmValStr), "%d%%", bgmPercent);
    float bgmRatio = SoundManager::GetInstance().GetBGMVolume();
    int bgmMinusY = bgmRowY + 24;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, bgmRowY, btnW, bgmRowH,
        isBgmSelected, "♪ BGM音量", bgmValStr, bgmRatio,
        GetColor(255, 170, 50),
        hoverBgmMinus, hoverBgmPlus, font16, font13
    );

    // --- コントロール5: SE音量 ---
    bool isSeSelected = (gm.m_pauseMenuItem == 4);
    int sePercent = SoundManager::GetInstance().GetSEVolumePercent();
    char seValStr[32];
    snprintf(seValStr, sizeof(seValStr), "%d%%", sePercent);
    float seRatio = SoundManager::GetInstance().GetSEVolume();
    int seMinusY = seRowY + 24;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, seRowY, btnW, seRowH,
        isSeSelected, "SE音量 (効果音)", seValStr, seRatio,
        GetColor(80, 220, 120),
        hoverSeMinus, hoverSePlus, font16, font13
    );

    // --- コントロール6: 筋トレ時アニメーション選択 (9つのモーションON/OFFボタン) ---
    bool isWorkoutRowSelected = (gm.m_pauseMenuItem == 5);
    DrawWorkoutMotionSelectorCard(
        workoutCardX, workoutCardY, workoutCardW, workoutCardH,
        isWorkoutRowSelected, gm.m_pauseSelectedWorkoutBtn,
        mx, my, font16, font13
    );

    // --- 下部: ゲームに戻る (ESC) ボタン ---
    int resumeBtnW = 280;
    int resumeBtnH = 34;
    int resumeBtnX = (Config::SCREEN_WIDTH - resumeBtnW) / 2;
    int resumeBtnY = modalY + 488;
    bool hoverResume = (mx >= resumeBtnX && mx <= resumeBtnX + resumeBtnW && my >= resumeBtnY && my <= resumeBtnY + resumeBtnH);

    SoftUI::DrawSoftCard(resumeBtnX, resumeBtnY, resumeBtnW, resumeBtnH, 18,
                         hoverResume ? GetColor(45, 60, 90) : GetColor(24, 30, 46), 235,
                         hoverResume ? GetColor(100, 220, 255) : GetColor(60, 75, 110), 1,
                         hoverResume, GetColor(80, 200, 255));

    const char* resumeTxt = "→ ゲームに戻る (ESC)";
    int rW = GetDrawStringWidthToHandle(resumeTxt, static_cast<int>(strlen(resumeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - rW) / 2, resumeBtnY + 8, resumeTxt, hoverResume ? GetColor(255, 255, 255) : GetColor(180, 205, 235), font16);

    // 操作説明ガイド（モーダル枠内下部に綺麗に配置）
    const char* guideTxt1 = "選択: [↑/↓]  感度・音量・モーション選択: [←/→] または クリック";
    int gW1 = GetDrawStringWidthToHandle(guideTxt1, static_cast<int>(strlen(guideTxt1)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW1) / 2, modalY + 534, guideTxt1, GetColor(160, 185, 220), font13);

    const char* guideTxt2 = "決定/ON・OFF切替: [ENTER] / [SPACE]  |  ゲーム説明: [H / TAB]  |  閉じる: [ESC]";
    int gW2 = GetDrawStringWidthToHandle(guideTxt2, static_cast<int>(strlen(guideTxt2)), font13);
    DrawStringToHandle((Config::SCREEN_WIDTH - gW2) / 2, modalY + 556, guideTxt2, GetColor(130, 150, 180), font13);

    // 筋トレコレクション確認ダイアログ（最前面描画）
    WorkoutCollectionManager::GetInstance().DrawConfirmDialogModal(font24, font16);
}

void UIManager::DrawSoundSettingsModal(GameManager& gm)
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

    const char* headerTxt = "設定";
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
    bool isBgmSelected = (gm.m_titleSoundSettingItem == 0);
    int bgmPercent = SoundManager::GetInstance().GetBGMVolumePercent();
    char bgmValStr[32];
    snprintf(bgmValStr, sizeof(bgmValStr), "%d%%", bgmPercent);
    int bgmMinusY = bgmRowY + 33;
    bool hoverBgmMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= bgmMinusY && my <= bgmMinusY + minusBtnH);
    bool hoverBgmPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= bgmMinusY && my <= bgmMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, bgmRowY, btnW, bgmRowH,
        isBgmSelected, "♪ BGM音量", bgmValStr, SoundManager::GetInstance().GetBGMVolume(),
        GetColor(255, 170, 50),
        hoverBgmMinus, hoverBgmPlus, font16, font13
    );

    // --- 2. SE音量 ---
    bool isSeSelected = (gm.m_titleSoundSettingItem == 1);
    int sePercent = SoundManager::GetInstance().GetSEVolumePercent();
    char seValStr[32];
    snprintf(seValStr, sizeof(seValStr), "%d%%", sePercent);
    int seMinusY = seRowY + 33;
    bool hoverSeMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= seMinusY && my <= seMinusY + minusBtnH);
    bool hoverSePlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= seMinusY && my <= seMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, seRowY, btnW, seRowH,
        isSeSelected, "SE音量 (効果音)", seValStr, SoundManager::GetInstance().GetSEVolume(),
        GetColor(80, 220, 120),
        hoverSeMinus, hoverSePlus, font16, font13
    );

    // --- 3. マウス感度 ---
    bool isSensSelected = (gm.m_titleSoundSettingItem == 2);
    float currentSens = gm.m_camera.GetMouseSensitivity();
    char sensValStr[32];
    snprintf(sensValStr, sizeof(sensValStr), "%.1fx", currentSens);
    float sensRatio = (currentSens - 0.2f) / (3.0f - 0.2f);
    int sensMinusY = sensRowY + 33;
    bool hoverSensMinus = (mx >= minusBtnX && mx <= minusBtnX + minusBtnW && my >= sensMinusY && my <= sensMinusY + minusBtnH);
    bool hoverSensPlus  = (mx >= plusBtnX && mx <= plusBtnX + plusBtnW && my >= sensMinusY && my <= sensMinusY + plusBtnH);

    DrawSettingSliderCard(
        btnX, sensRowY, btnW, sensRowH,
        isSensSelected, "マウス感度", sensValStr, sensRatio,
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

void UIManager::DrawDebug2DOverlay(GameManager& gm)
{
    auto player = gm.m_objManager.GetPlayer();
    if (!player)
    {
        return;
    }

    const auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();

    const auto& stage = gm.m_objManager.GetStage();
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
    if (gm.m_debugCubeMode == 1)
    {
        float yaw = gm.m_camera.GetAngleH();
        cubeCenter = VGet(pPos.x + std::sin(yaw) * halfCube, pPos.y, pPos.z + std::cos(yaw) * halfCube);
    }
    else if (gm.m_debugCubeMode == 2)
    {
        cubeCenter = VGet(0.0f, pPos.y, 0.0f);
    }

    if (gm.m_debugCubeMode != 3)
    {
        float baseY = cubeCenter.y + 2.2f;
        float topY  = baseY + cubeSize;
        float midY  = baseY + halfCube;

        unsigned int cyanCol   = GetColor(80, 235, 255);
        unsigned int yellowCol = GetColor(255, 220, 80);

        // 底面4辺の中央に「10m (580.5u)」ラベルを表示
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z - halfCube), "< 10m辺 (580.5u) >", cyanCol);
        drawWorldLabel(VGet(cubeCenter.x, baseY, cubeCenter.z + halfCube), "< 10m辺 (580.5u) >", cyanCol);
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
    std::vector<VECTOR> csvPositions = CsvManager::GetInstance().GetMousePositionsForMap(gm.m_selectedMap);
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
    for (const auto& obj : gm.m_objManager.GetObjects())
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
    snprintf(modeBuf, sizeof(modeBuf), "・[7] ■配置: %s", cubeModeNames[gm.m_debugCubeMode % 4]);
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
    const char* mapName = (gm.m_selectedMap == ModelConfig::MapType::NewMap) ? "Map2(NewMap)" : "Map1(House)";

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
    for (const auto& obj : gm.m_objManager.GetObjects())
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
    if (gm.m_debugCubeMode != 3)
    {
        int cx0, cy0, cx1, cy1;
        worldToMinimap(cubeCenter.x - halfCube, cubeCenter.z + halfCube, cx0, cy0);
        worldToMinimap(cubeCenter.x + halfCube, cubeCenter.z - halfCube, cx1, cy1);
        DrawBox(cx0, cy0, cx1, cy1, GetColor(0, 255, 220), FALSE);
    }

    // 生存ネズミのミニマップ位置
    for (const auto& obj : gm.m_objManager.GetObjects())
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
                             "[4] ネズミ時間停止: %s", gm.m_objManager.IsTimeFrozen() ? "ON" : "OFF");
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
                             "[8] 10m全体目測・広角カメラ: %s", gm.m_debugWideCamera ? "ON (俯瞰)" : "OFF (通常)");
    cy += 18;
    DrawStringToHandle(cheatX + 12, cy, "[F1] デバッグオーバーレイ ON/OFF", GetColor(180, 195, 215), font13);
}



// ============================================================================
// 筋トレコレクションモーダルの入力更新処理

void UIManager::DrawWorkoutCollectionModal(GameManager& gm)
{
    auto& fm = FontManager::GetInstance();
    int font13 = fm.GetFont13();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();
    auto& collMgr = WorkoutCollectionManager::GetInstance();

    // ★ ガチャ演出中であれば、通常モーダル枠やボタンは隠し、ガチャ演出（3D猫実演＋カード）を最優先描画！
    if (collMgr.IsGachaAnimationActive())
    {
        collMgr.DrawGachaAnimationModal(font24, font18, font16, font13);
        return;
    }

    // 1. 全画面暗転レイヤー（背景の3D筋トレ猫をほんのり見せつつダイアログを際立たせる）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
    DrawBox(0, 0, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, GetColor(8, 10, 18), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 2. モーダル本体外枠
    int modalW = 760;
    int modalH = 376;
    int modalX = (Config::SCREEN_WIDTH - modalW) / 2;
    int modalY = (Config::SCREEN_HEIGHT - modalH) / 2;

    unsigned int borderGlow = collMgr.IsDebugMode() ? GetColor(255, 215, 60) : GetColor(220, 90, 240);
    SoftUI::DrawSoftCard(modalX, modalY, modalW, modalH, 20, GetColor(20, 24, 38), 250, borderGlow, collMgr.IsDebugMode() ? 3 : 2);

    // ヘッダーバー背景
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
    DrawRoundRect(modalX + 3, modalY + 3, modalX + modalW - 3, modalY + 48, 18, 18, GetColor(38, 30, 60), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(modalX + 16, modalY + 48, modalX + modalW - 16, modalY + 48, borderGlow);

    // ヘッダー左側: ★ 筋トレコレクション
    DrawStringToHandle(modalX + 20, modalY + 14, "★ 筋トレコレクション", GetColor(255, 220, 90), font18);

    // ヘッダー: (所持: X / 9)
    char countBuf[32];
    snprintf(countBuf, sizeof(countBuf), "(所持: %d / 9)", collMgr.GetUnlockedCount());
    DrawStringToHandle(modalX + 245, modalY + 16, countBuf, GetColor(200, 220, 255), font16);

    // ヘッダー中央右: プロテイン: X pt
    char ptBuf[64];
    snprintf(ptBuf, sizeof(ptBuf), "プロテイン: %d pt", collMgr.GetProteinPoints());
    DrawStringToHandle(modalX + 380, modalY + 16, ptBuf, GetColor(255, 230, 90), font16);

    int mx = 0, my = 0;
    GetMousePoint(&mx, &my);

#ifdef _DEBUG
    // ヘッダー右側：[【Debug】OFF/ON (D)] ボタン（Debugビルドのみ）
    int dbgBtnW = 130;
    int dbgBtnH = 28;
    int dbgBtnX = modalX + modalW - 178;
    int dbgBtnY = modalY + 11;
    bool hoverDbg = (mx >= dbgBtnX && mx <= dbgBtnX + dbgBtnW && my >= dbgBtnY && my <= dbgBtnY + dbgBtnH);
    unsigned int dbgBg = collMgr.IsDebugMode()
        ? (hoverDbg ? GetColor(240, 170, 30) : GetColor(200, 140, 20))
        : (hoverDbg ? GetColor(60, 75, 105)  : GetColor(38, 48, 68));
    unsigned int dbgBorder = collMgr.IsDebugMode() ? GetColor(255, 255, 140) : GetColor(90, 115, 150);

    SoftUI::DrawSoftCard(dbgBtnX, dbgBtnY, dbgBtnW, dbgBtnH, 8, dbgBg, 250, dbgBorder, hoverDbg ? 2 : 1);
    const char* dbgLabel = collMgr.IsDebugMode() ? "★ DEBUG: ON (D)" : "【Debug】OFF (D)";
    int dlW = GetDrawStringWidthToHandle(dbgLabel, static_cast<int>(strlen(dbgLabel)), font13);
    DrawStringToHandle(dbgBtnX + (dbgBtnW - dlW) / 2, dbgBtnY + 6, dbgLabel, collMgr.IsDebugMode() ? GetColor(20, 20, 30) : GetColor(210, 225, 245), font13);
#endif

    // 右上 [×] ボタン
    int xBtnX = modalX + modalW - 38;
    int xBtnY = modalY + 11;
    int xBtnSize = 28;
    bool hoverX = (mx >= xBtnX && mx <= xBtnX + xBtnSize && my >= xBtnY && my <= xBtnY + xBtnSize);
    DrawRoundRect(xBtnX, xBtnY, xBtnX + xBtnSize, xBtnY + xBtnSize, 6, 6, hoverX ? GetColor(190, 40, 60) : GetColor(50, 40, 65), TRUE);
    DrawStringToHandle(xBtnX + 7, xBtnY + 5, "×", hoverX ? GetColor(255, 255, 255) : GetColor(200, 180, 220), font16);

    // 3. 9つの筋トレモーションボタン（上段5個 / 下段4個）
    int topRowY = modalY + 56;
    int botRowY = modalY + 110;
    int btnW    = 124;
    int btnH    = 46;

    for (int i = 0; i < static_cast<int>(WorkoutMotionType::Count); ++i)
    {
        auto mType      = static_cast<WorkoutMotionType>(i);
        bool isUnlocked = collMgr.IsMotionUnlocked(i);
        bool isOn       = Player3D::IsWorkoutMotionEnabled(mType);
        std::string aeCode = WorkoutCollectionManager::IdToAeCode(i);

        int bx = (i < 5) ? (modalX + 30 + i * 140) : (modalX + 100 + (i - 5) * 140);
        int by = (i < 5) ? topRowY : botRowY;

        bool isHover = (mx >= bx && mx <= bx + btnW && my >= by && my <= by + btnH);
        bool isFocus = (gm.m_selectedWorkoutBtn == i) || isHover;

        if (isUnlocked)
        {
            unsigned int bgCol     = isOn ? (isFocus ? GetColor(38, 120, 72) : GetColor(24, 86, 52))
                                          : (isFocus ? GetColor(50, 60, 80)  : GetColor(30, 36, 50));
            unsigned int borderCol = isFocus ? GetColor(255, 230, 80)
                                             : (isOn ? GetColor(85, 230, 130) : GetColor(70, 85, 110));

            SoftUI::DrawSoftCard(bx, by, btnW, btnH, 8,
                                 bgCol, 245, borderCol, isFocus ? 2 : 1,
                                 isFocus || isOn, isFocus ? GetColor(255, 220, 70) : GetColor(70, 210, 115));

            // モーション名（上段）
            const char* mName = Player3D::GetWorkoutMotionName(mType);
            int nameW = GetDrawStringWidthToHandle(mName, static_cast<int>(strlen(mName)), font13);
            DrawStringToHandle(bx + (btnW - nameW) / 2, by + 4, mName, isOn ? GetColor(255, 255, 255) : GetColor(170, 180, 200), font13);

            // A/Eコード ＆ ON/OFFバッジ（下段）
            char badgeBuf[32];
            snprintf(badgeBuf, sizeof(badgeBuf), "%s", isOn ? "【 ON 】" : "【 OFF 】");
            int bdW = GetDrawStringWidthToHandle(badgeBuf, static_cast<int>(strlen(badgeBuf)), font13);
            DrawStringToHandle(bx + (btnW - bdW) / 2, by + 23, badgeBuf, isOn ? GetColor(180, 255, 200) : GetColor(140, 150, 170), font13);
        }
        else
        {
            // 未所持（ロック表示）
            unsigned int bgCol     = isHover ? GetColor(34, 38, 48) : GetColor(22, 25, 34);
            unsigned int borderCol = isHover ? GetColor(140, 150, 175) : GetColor(50, 58, 72);

            SoftUI::DrawSoftCard(bx, by, btnW, btnH, 8, bgCol, 240, borderCol, 1);

            const char* lockTxt = "【未所持】";
            int lW = GetDrawStringWidthToHandle(lockTxt, static_cast<int>(strlen(lockTxt)), font13);
            DrawStringToHandle(bx + (btnW - lW) / 2, by + 4, lockTxt, GetColor(130, 140, 155), font13);

            char codeBuf[32];
            snprintf(codeBuf, sizeof(codeBuf), "%s", "ガチャで解放");
            int cW = GetDrawStringWidthToHandle(codeBuf, static_cast<int>(strlen(codeBuf)), font13);
            DrawStringToHandle(bx + (btnW - cW) / 2, by + 23, codeBuf, GetColor(100, 110, 130), font13);
        }
    }

    // 4. アクションボタン（ガチャ・セーブ・ロード・(Debug削除)）
    int actBtnY = modalY + 168;
    int actBtnH = 36;
#ifdef _DEBUG
    const int numTitleActs = 4;
    int actX[4] = { modalX + 30, modalX + 220, modalX + 380, modalX + 540 };
    int actW[4] = { 175, 145, 145, 185 };
#else
    const int numTitleActs = 3;
    int actX[3] = { modalX + 30, modalX + 280, modalX + 510 };
    int actW[3] = { 230, 210, 220 };
#endif

    for (int act = 0; act < numTitleActs; ++act)
    {
        int ax = actX[act];
        int ay = actBtnY;
        int aw = actW[act];
        int ah = actBtnH;
        bool isHover = (mx >= ax && mx <= ax + aw && my >= ay && my <= ay + ah);

        if (act == 0) // ガチャ
        {
            bool isAllUnlocked = collMgr.IsAllUnlocked();
            bool canAfford = (collMgr.GetProteinPoints() >= WorkoutCollectionManager::GACHA_COST);

            unsigned int fillCol = isAllUnlocked ? GetColor(45, 55, 75)
                : (canAfford ? (isHover ? GetColor(255, 185, 45) : GetColor(220, 150, 30))
                             : (isHover ? GetColor(65, 50, 35) : GetColor(45, 35, 25)));
            unsigned int borderCol = isAllUnlocked ? GetColor(90, 110, 140)
                : (canAfford ? (isHover ? GetColor(255, 240, 140) : GetColor(255, 210, 60))
                             : GetColor(130, 95, 45));

            SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 250, borderCol, isHover ? 2 : 1);

            const char* gachaLabel = isAllUnlocked ? "★ コンプリート！"
                : (canAfford ? "★ ガチャ (100 pt)" : "ガチャ (100 pt必要)");
            int gW = GetDrawStringWidthToHandle(gachaLabel, static_cast<int>(strlen(gachaLabel)), font16);
            unsigned int gTxtCol = isAllUnlocked ? GetColor(180, 200, 230)
                : (canAfford ? GetColor(20, 20, 30) : GetColor(210, 170, 120));
            DrawStringToHandle(ax + (aw - gW) / 2, ay + 8, gachaLabel, gTxtCol, font16);
        }
        else if (act == 1) // セーブ
        {
            unsigned int fillCol   = isHover ? GetColor(40, 95, 140) : GetColor(25, 60, 95);
            unsigned int borderCol = isHover ? GetColor(140, 225, 255) : GetColor(65, 150, 210);
            SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

            const char* saveLabel = "セーブ (コピー)";
            int sW = GetDrawStringWidthToHandle(saveLabel, static_cast<int>(strlen(saveLabel)), font16);
            DrawStringToHandle(ax + (aw - sW) / 2, ay + 8, saveLabel, GetColor(225, 245, 255), font16);
        }
        else if (act == 2) // ロード
        {
            unsigned int fillCol   = isHover ? GetColor(40, 95, 140) : GetColor(25, 60, 95);
            unsigned int borderCol = isHover ? GetColor(140, 225, 255) : GetColor(65, 150, 210);
            SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

            const char* loadLabel = "ロード (読込)";
            int lW = GetDrawStringWidthToHandle(loadLabel, static_cast<int>(strlen(loadLabel)), font16);
            DrawStringToHandle(ax + (aw - lW) / 2, ay + 8, loadLabel, GetColor(225, 245, 255), font16);
        }
        else if (act == 3) // デバッグ削除
        {
            unsigned int fillCol   = isHover ? GetColor(160, 35, 45) : GetColor(100, 25, 35);
            unsigned int borderCol = isHover ? GetColor(255, 120, 120) : GetColor(180, 50, 60);
            SoftUI::DrawSoftCard(ax, ay, aw, ah, 8, fillCol, 245, borderCol, isHover ? 2 : 1);

            const char* resetLabel = "【Debug】データ削除";
            int rW = GetDrawStringWidthToHandle(resetLabel, static_cast<int>(strlen(resetLabel)), font16);
            DrawStringToHandle(ax + (aw - rW) / 2, ay + 8, resetLabel, GetColor(255, 225, 225), font16);
        }
    }

#ifdef _DEBUG
    // 5. ★ デバッグツールバー（Debugビルド時のみ）
    if (collMgr.IsDebugMode())
    {
        int dbgRowY = modalY + 214;
        int dbgRowH = 32;

        int dBtnX[5] = { modalX + 30, modalX + 176, modalX + 322, modalX + 448, modalX + 574 };
        int dBtnW[5] = { 136, 136, 116, 116, 156 };
        const char* dLabels[5] = { "【1: 全解放】", "【2: +1000pt】", "【3: 全ON】", "【4: 全OFF】", "【5: データ削除】" };
        unsigned int dColors[5] = {
            GetColor(255, 180, 40),
            GetColor(255, 220, 50),
            GetColor(80, 220, 120),
            GetColor(80, 180, 240),
            GetColor(255, 90, 90)
        };

        for (int d = 0; d < 5; ++d)
        {
            int dx = dBtnX[d];
            int dy = dbgRowY;
            int dw = dBtnW[d];
            int dh = dbgRowH;
            bool isDHover = (mx >= dx && mx <= dx + dw && my >= dy && my <= dy + dh);

            unsigned int bg = isDHover ? GetColor(50, 45, 60) : GetColor(26, 26, 36);
            SoftUI::DrawSoftCard(dx, dy, dw, dh, 6, bg, 255, dColors[d], isDHover ? 2 : 1);
            int txtW = GetDrawStringWidthToHandle(dLabels[d], static_cast<int>(strlen(dLabels[d])), font13);
            DrawStringToHandle(dx + (dw - txtW) / 2, dy + 7, dLabels[d], isDHover ? GetColor(255, 255, 255) : dColors[d], font13);
        }
    }
    else
    {
        // デバッグOFF時のショートカット案内ヒント
        int hintY = modalY + 222;
        const char* dHint = "※ [D] または [F1] キーで【筋トレデバッグモード】を切り替え可能";
        DrawStringToHandle(modalX + 30, hintY, dHint, GetColor(130, 150, 180), font13);
    }
#endif

    // 6. ステータスメッセージ / セーブコード（行1）
    int statusY = modalY + 258;
    if (collMgr.GetStatusMessageTimer() > 0)
    {
        const auto& msg = collMgr.GetStatusMessage();
        DrawStringToHandle(modalX + 30, statusY, msg.c_str(), GetColor(255, 235, 100), font13);
    }
    else
    {
        std::string codeStr = "※筋トレの所持状況・プロテインは自動保存されます";
        DrawStringToHandle(modalX + 30, statusY, codeStr.c_str(), GetColor(150, 180, 215), font13);
    }

    // 操作案内（行2：ステータスと重複しない独立行）
    int guideY = modalY + 282;
    const char* hintTxt = "※クリックまたは[ENTER / SPACE]でON/OFF切替";
    DrawStringToHandle(modalX + 30, guideY, hintTxt, GetColor(140, 160, 185), font13);

    // 7. 下部中央 [ × 閉じる (ESC / C) ] ボタン
    int closeBtnW = 220;
    int closeBtnH = 36;
    int closeBtnX = (Config::SCREEN_WIDTH - closeBtnW) / 2;
    int closeBtnY = modalY + modalH - 46;

    bool hoverClose = (mx >= closeBtnX && mx <= closeBtnX + closeBtnW && my >= closeBtnY && my <= closeBtnY + closeBtnH);
    SoftUI::DrawSoftCard(
        closeBtnX, closeBtnY, closeBtnW, closeBtnH, 16,
        hoverClose ? GetColor(150, 60, 170) : GetColor(45, 35, 60),
        250,
        hoverClose ? GetColor(255, 180, 255) : GetColor(130, 95, 160),
        hoverClose ? 2 : 1
    );
    const char* closeTxt = "[ × 閉じる (ESC / C) ]";
    int cW = GetDrawStringWidthToHandle(closeTxt, static_cast<int>(strlen(closeTxt)), font16);
    DrawStringToHandle((Config::SCREEN_WIDTH - cW) / 2, closeBtnY + 8, closeTxt, hoverClose ? GetColor(255, 255, 255) : GetColor(220, 200, 240), font16);

    // ★ ガチャ演出中であれば最前面に熱血アニメーションを描画！
    if (collMgr.IsGachaAnimationActive())
    {
        collMgr.DrawGachaAnimationModal(font24, font18, font16, font13);
    }
}
