// ============================================================================
// @file   FontManager.h
// @brief  ゲーム全体で使用するフォントリソースの一元管理クラス（シングルトン）
// ============================================================================
#pragma once
#include "DxLib.h"

// ============================================================================
// FontManager クラス（シングルトン）
// 毎フレームのフォント再生成コストを防ぐため、起動時にサイズ別フォントハンドルを
// 一括生成・保持し、各UI描画処理へ提供する
// ============================================================================
class FontManager
{
private:
    int m_font13 = -1; // 13pt: 補足説明・デバッグ数値・操作ガイド用
    int m_font16 = -1; // 16pt: 標準UIラベル・吹き出しテキスト用
    int m_font18 = -1; // 18pt: 小見出し・ボタン内キーバッジ用
    int m_font24 = -1; // 24pt: メニューボタン・カード見出し・通知ポップアップ用
    int m_font36 = -1; // 36pt: 大見出し・スコア数値・コンボ表示用
    int m_font48 = -1; // 48pt: クリアタイム等の特大数値用
    int m_font60 = -1; // 60pt: タイトルロゴ「マッスルねこ」・ステージクリアロゴ用
    int m_font96 = -1; // 96pt: リザルト画面の特大ランク文字（S/A/B/C）スタンプ用

    // シングルトンのためコンストラクタ・デストラクタは非公開
    FontManager() = default;
    ~FontManager() = default;

public:
    // 唯一のインスタンスを取得する
    static FontManager& GetInstance()
    {
        static FontManager instance;
        return instance;
    }

    // コピー・代入を禁止
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // 全サイズのフォントハンドルを生成する
    void Init();

    // 生成済みの全フォントハンドルを解放する
    void Release();

    // ------------------------------------------------------------------------
    // サイズ別フォントハンドル取得ゲッター
    // ------------------------------------------------------------------------
    int GetFont13() const { return m_font13; }
    int GetFont16() const { return m_font16; }
    int GetFont18() const { return m_font18; }
    int GetFont24() const { return m_font24; }
    int GetFont36() const { return m_font36; }
    int GetFont48() const { return m_font48; }
    int GetFont60() const { return m_font60; }
    int GetFont96() const { return m_font96; }
};
