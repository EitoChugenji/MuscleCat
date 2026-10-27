// ============================================================================
// @file   FontManager.cpp
// @brief  FontManager クラスの実装（サイズ別フォントの生成・解放）
// ============================================================================
#include "FontManager.h"

// ============================================================================
// フォントの初期化処理
// 視認性とポップな力強さを両立するため、太字の「ＭＳ ゴシック」をベースに生成する。
// 中小サイズ（13〜48pt）はクッキリとしたドット感・可読性を重視して NORMAL、
// 特大サイズ（60・96pt）はジャギーを抑えるため 8x8 アンチエイリアスを適用する。
// ============================================================================
void FontManager::Init()
{
    m_font13 = CreateFontToHandle("ＭＳ ゴシック", 13, 5, DX_FONTTYPE_NORMAL);
    m_font16 = CreateFontToHandle("ＭＳ ゴシック", 16, 6, DX_FONTTYPE_NORMAL);
    m_font18 = CreateFontToHandle("ＭＳ ゴシック", 18, 6, DX_FONTTYPE_NORMAL);
    m_font24 = CreateFontToHandle("ＭＳ ゴシック", 24, 7, DX_FONTTYPE_NORMAL);
    m_font36 = CreateFontToHandle("ＭＳ ゴシック", 36, 8, DX_FONTTYPE_NORMAL);
    m_font48 = CreateFontToHandle("ＭＳ ゴシック", 48, 8, DX_FONTTYPE_NORMAL);
    m_font60 = CreateFontToHandle("ＭＳ ゴシック", 60, 9, DX_FONTTYPE_ANTIALIASING_8X8);
    m_font96 = CreateFontToHandle("ＭＳ ゴシック", 96, 9, DX_FONTTYPE_ANTIALIASING_8X8);
}

// ============================================================================
// フォントリソースの解放処理
// ============================================================================
void FontManager::Release()
{
    if (m_font13 != -1) { DeleteFontToHandle(m_font13); m_font13 = -1; }
    if (m_font16 != -1) { DeleteFontToHandle(m_font16); m_font16 = -1; }
    if (m_font18 != -1) { DeleteFontToHandle(m_font18); m_font18 = -1; }
    if (m_font24 != -1) { DeleteFontToHandle(m_font24); m_font24 = -1; }
    if (m_font36 != -1) { DeleteFontToHandle(m_font36); m_font36 = -1; }
    if (m_font48 != -1) { DeleteFontToHandle(m_font48); m_font48 = -1; }
    if (m_font60 != -1) { DeleteFontToHandle(m_font60); m_font60 = -1; }
    if (m_font96 != -1) { DeleteFontToHandle(m_font96); m_font96 = -1; }
}
