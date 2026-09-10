#include "FontManager.h"
#include "DxLib.h"

void FontManager::Init() {
    Release();

    m_font13 = CreateFontToHandle("MS Gothic", 13, 1, DX_FONTTYPE_NORMAL);
    m_font16 = CreateFontToHandle("MS Gothic", 16, 2, DX_FONTTYPE_NORMAL);
    m_font18 = CreateFontToHandle("MS Gothic", 18, 2, DX_FONTTYPE_NORMAL);
    m_font24 = CreateFontToHandle("MS Gothic", 24, 3, DX_FONTTYPE_NORMAL);
    m_font36 = CreateFontToHandle("MS Gothic", 36, 4, DX_FONTTYPE_NORMAL);
    m_font48 = CreateFontToHandle("MS Gothic", 48, 5, DX_FONTTYPE_NORMAL);
}

void FontManager::Release() {
    if (m_font13 != -1) { DeleteFontToHandle(m_font13); m_font13 = -1; }
    if (m_font16 != -1) { DeleteFontToHandle(m_font16); m_font16 = -1; }
    if (m_font18 != -1) { DeleteFontToHandle(m_font18); m_font18 = -1; }
    if (m_font24 != -1) { DeleteFontToHandle(m_font24); m_font24 = -1; }
    if (m_font36 != -1) { DeleteFontToHandle(m_font36); m_font36 = -1; }
    if (m_font48 != -1) { DeleteFontToHandle(m_font48); m_font48 = -1; }
}
