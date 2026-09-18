#pragma once
#include <string>

// FontManager クラス (シングルトン)
// 毎フレームのSetFontSize負荷を抑えるためのフォント事前キャッシュ管理
class FontManager
{
private:
    int m_font13 = -1;
    int m_font16 = -1;
    int m_font18 = -1;
    int m_font24 = -1;
    int m_font36 = -1;
    int m_font48 = -1;

    FontManager() = default;
    ~FontManager() = default;

public:
    static FontManager& GetInstance()
    {
        static FontManager instance;
        return instance;
    }

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    void Init();
    void Release();

    int GetFont13() const
    {
        return m_font13;
    }

    int GetFont16() const
    {
        return m_font16;
    }

    int GetFont18() const
    {
        return m_font18;
    }

    int GetFont24() const
    {
        return m_font24;
    }

    int GetFont36() const
    {
        return m_font36;
    }

    int GetFont48() const
    {
        return m_font48;
    }
};
