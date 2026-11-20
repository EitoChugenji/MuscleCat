// ============================================================================
// @file   UIManager.h
// @brief  全画面の2D UI・モーダル・デバッグオーバーレイ描画を統括するクラス定義
// ============================================================================
#pragma once
#include "Player3D.h"

class GameManager;

/// <summary>
/// タイトル画面・リザルト画面・各種モーダル（ゲーム説明・設定・ポーズ・筋トレコレクション）
/// およびデバッグ用2Dオーバーレイの描画を専門に担当するクラス
/// </summary>
class UIManager
{
public:
    UIManager() = default;
    ~UIManager() = default;

    /// <summary>タイトル画面の2D UI（ロゴ・吹き出し・メニューボタン・各種モーダル）を描画する</summary>
    void DrawTitleScreen(GameManager& gm);

    /// <summary>リザルト画面（ランクスタンプ・スコア内訳・アクションボタン）を描画する</summary>
    void DrawGameClearScreen(GameManager& gm);

    /// <summary>ゲーム説明モーダルダイアログ（全5ページ）を描画する</summary>
    void DrawHowToPlayModal(GameManager& gm);

    /// <summary>タイトル画面の各種設定モーダル（BGM/SE音量・マウス感度）を描画する</summary>
    void DrawSoundSettingsModal(GameManager& gm);

    /// <summary>筋トレコレクション＆ガチャモーダルを描画する</summary>
    void DrawWorkoutCollectionModal(GameManager& gm);

    /// <summary>プレイ中のポーズ／設定モーダルを描画する</summary>
    void DrawPauseModal(GameManager& gm);

    /// <summary>F1デバッグモード時の2D情報パネル＆ミニマップを描画する</summary>
    void DrawDebug2DOverlay(GameManager& gm);

    /// <summary>筋トレモーション選択カード内の個別ボタン矩形を算出する（描画・当たり判定共通）</summary>
    static void GetWorkoutBtnRect(int cardX, int cardY, int cardW, int cardH, int btnIdx,
                                  int& outX, int& outY, int& outW, int& outH);

    /// <summary>筋トレコレクションカード下部のアクションボタン矩形を算出する（描画・当たり判定共通）</summary>
    static void GetWorkoutActionBtnRect(int cardX, int cardY, int cardW, int cardH, int actionIdx,
                                        int& outX, int& outY, int& outW, int& outH);

    /// <summary>タイトル画面・リザルト画面の背景猫が再生する筋トレモーションを算出する</summary>
    static Player3D::TitleWorkoutType CalcActiveTitleWorkout(int animTimer);
};
