// ============================================================================
// @file   SoundManager.h
// @brief  ゲーム内の全BGM・SE（効果音）の読み込み・再生・音量制御シングルトンクラス
// ============================================================================
#pragma once
#include <unordered_map>
#include <string>

// ============================================================================
// BGM種類列挙型
// ============================================================================
enum class BgmType
{
    None = -1,
    Title,      // タイトル画面BGM（明るくポップな筋トレマーチ）
    Stage,      // ゲームプレイBGM（疾走感・スピード感あふれるアクションBGM）
    Clear,      // クリアファンファーレ（勝利の讃歌ジングル）
    TimeUp,     // タイムアップジングル（コミカル脱力終了音）
    Count
};

// ============================================================================
// SE（効果音）種類列挙型
// ============================================================================
enum class SeType
{
    Cursor,         // UIカーソル移動（ピピッ）
    Decide,         // UI決定・ゲーム開始（ピロリン！）
    Cancel,         // キャンセル・戻る（ポスッ）
    Pause,          // ポーズ（ピロリッ）
    SquatStart,     // 筋トレQTE開始（シュイーン！）
    SquatSuccess,   // 筋トレQTE成功・レベルアップ（シャキーン！）
    SquatFail,      // 筋トレQTE失敗・筋肉痛（ブブーッ！）
    Pounce,         // 飛びつき攻撃・跳躍（シュバッ！）
    Tackle,         // 電撃タックル・スタン攻撃（ズガーン！）
    Catch,          // ネズミ捕獲チャイム（ピコーン！）
    RatSqueak,      // ネズミの鳴き声（チュー！）
    PumpDecay,      // 筋肉減衰（ヒューン…）
    Stamp,          // ランクスタンプ押印（ドスン！）
    Countdown,      // 残り時間警告ビープ（ピッ！）
    Footstep,       // 足音（トコッ、ペタッ）
    Count
};

// ============================================================================
// SoundManager クラス（シングルトン）
// DXライブラリのサウンド機能（LoadSoundMem, PlaySoundMem等）を統合管理し、
// BGMのシームレス再生/停止/一時停止、SEの多重再生、音量バランス制御を提供する
// ============================================================================
class SoundManager
{
public:
    static SoundManager& GetInstance()
    {
        static SoundManager instance;
        return instance;
    }

    // 初期化（全音源ファイルのロード）および解放
    void Init();
    void Release();

    // BGM制御
    void PlayBGM(BgmType type, bool loop = true);
    void StopBGM();
    void PauseBGM();
    void ResumeBGM();
    bool IsBGMPlaying() const;
    BgmType GetCurrentBgm() const { return m_currentBgm; }

    // SE制御
    void PlaySE(SeType type, float volumeScale = 1.0f);
    void StopSE(SeType type);
    void StopAllSE();

    // 音量制御 (0.0f 〜 1.0f)
    void  SetBGMVolume(float volume);
    float GetBGMVolume() const { return m_bgmVolume; }

    void  SetSEVolume(float volume);
    float GetSEVolume() const { return m_seVolume; }

    void  SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }

    // 音量増減ヘルパー (delta: +0.05f / -0.05f 等)
    void AdjustBGMVolume(float delta);
    void AdjustSEVolume(float delta);

    // パーセント表示用 (0 〜 100)
    int GetBGMVolumePercent() const { return static_cast<int>(m_bgmVolume * 100.0f + 0.5f); }
    int GetSEVolumePercent() const  { return static_cast<int>(m_seVolume * 100.0f + 0.5f); }

    // 設定ファイル保存・読み込み
    void SaveSettings();
    void LoadSettings();

    // 全サウンドの一括停止
    void StopAll();

private:
    SoundManager();
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // 音量換算ヘルパー（0.0f〜1.0f を DxLibの 0〜255 に変換）
    int CalcBgmVolumeDxLib() const;
    int CalcSeVolumeDxLib(float scale) const;

    // サウンドハンドル保持テーブル
    std::unordered_map<BgmType, int> m_bgmHandles;
    std::unordered_map<SeType, int>  m_seHandles;

    BgmType m_currentBgm       = BgmType::None;
    bool    m_isBgmPaused      = false;
    float   m_masterVolume     = 1.0f;
    float   m_bgmVolume        = 0.70f;
    float   m_seVolume         = 0.85f;
    bool    m_isInitialized    = false;
};
