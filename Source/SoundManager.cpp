// ============================================================================
// @file   SoundManager.cpp
// @brief  ゲーム内の全BGM・SE（効果音）の読み込み・再生・音量制御シングルトン実装
// ============================================================================
#include "SoundManager.h"
#include "DxLib.h"
#include <algorithm>
#include <cstdio>

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
    Release();
}

int SoundManager::CalcBgmVolumeDxLib() const
{
    float v = m_masterVolume * m_bgmVolume;
    v = (std::max)(0.0f, (std::min)(1.0f, v));
    return static_cast<int>(v * 255.0f);
}

int SoundManager::CalcSeVolumeDxLib(float scale) const
{
    float v = m_masterVolume * m_seVolume * scale;
    v = (std::max)(0.0f, (std::min)(1.0f, v));
    return static_cast<int>(v * 255.0f);
}

void SoundManager::Init()
{
    if (m_isInitialized)
    {
        return;
    }

    // 保存されている音量設定を読み込み
    LoadSettings();

    // ------------------------------------------------------------------------
    // 1. BGMリソースの読み込み
    // ------------------------------------------------------------------------
    const struct { BgmType type; const char* path; } bgmFiles[] = {
        { BgmType::Title,  "Resource/Sounds/bgm_title.wav" },
        { BgmType::Stage,  "Resource/Sounds/bgm_stage.wav" },
        { BgmType::Clear,  "Resource/Sounds/bgm_clear.wav" },
        { BgmType::TimeUp, "Resource/Sounds/bgm_timeup.wav" }
    };

    for (const auto& item : bgmFiles)
    {
        int handle = LoadSoundMem(item.path);
        if (handle != -1)
        {
            m_bgmHandles[item.type] = handle;
            ChangeVolumeSoundMem(CalcBgmVolumeDxLib(), handle);
        }
    }

    // ------------------------------------------------------------------------
    // 2. SEリソースの読み込み（多重再生用にバッファ数 3〜6 を確保）
    // ------------------------------------------------------------------------
    const struct { SeType type; const char* path; int bufferNum; } seFiles[] = {
        { SeType::Cursor,       "Resource/Sounds/se_cursor.wav",       4 },
        { SeType::Decide,       "Resource/Sounds/se_decide.wav",       4 },
        { SeType::Cancel,       "Resource/Sounds/se_cancel.wav",       3 },
        { SeType::Pause,        "Resource/Sounds/se_pause.wav",        2 },
        { SeType::SquatStart,   "Resource/Sounds/se_squat_start.wav",  3 },
        { SeType::SquatSuccess, "Resource/Sounds/se_squat_success.wav", 4 },
        { SeType::SquatFail,    "Resource/Sounds/se_squat_fail.wav",   3 },
        { SeType::Pounce,       "Resource/Sounds/se_pounce.wav",       4 },
        { SeType::Tackle,       "Resource/Sounds/se_tackle.wav",       4 },
        { SeType::Catch,        "Resource/Sounds/se_catch.wav",        6 },
        { SeType::RatSqueak,    "Resource/Sounds/se_rat_squeak.wav",   4 },
        { SeType::PumpDecay,    "Resource/Sounds/se_pump_decay.wav",   3 },
        { SeType::Stamp,        "Resource/Sounds/se_stamp.wav",        3 },
        { SeType::Countdown,    "Resource/Sounds/se_countdown.wav",    3 },
        { SeType::Footstep,     "Resource/Sounds/se_footstep.wav",     4 }
    };

    for (const auto& item : seFiles)
    {
        int handle = LoadSoundMem(item.path, item.bufferNum);
        if (handle != -1)
        {
            m_seHandles[item.type] = handle;
            ChangeVolumeSoundMem(CalcSeVolumeDxLib(1.0f), handle);
        }
    }

    m_isInitialized = true;
}

void SoundManager::Release()
{
    if (!m_isInitialized)
    {
        return;
    }

    SaveSettings();
    StopAll();

    for (auto& pair : m_bgmHandles)
    {
        if (pair.second != -1)
        {
            DeleteSoundMem(pair.second);
        }
    }
    m_bgmHandles.clear();

    for (auto& pair : m_seHandles)
    {
        if (pair.second != -1)
        {
            DeleteSoundMem(pair.second);
        }
    }
    m_seHandles.clear();

    m_currentBgm    = BgmType::None;
    m_isBgmPaused   = false;
    m_isInitialized = false;
}

void SoundManager::PlayBGM(BgmType type, bool loop)
{
    if (type == BgmType::None)
    {
        StopBGM();
        return;
    }

    // 既に同じBGMが再生中であればスキップ（一時停止中なら再開）
    if (m_currentBgm == type)
    {
        if (m_isBgmPaused)
        {
            ResumeBGM();
        }
        return;
    }

    // 現在のBGMを停止
    StopBGM();

    auto it = m_bgmHandles.find(type);
    if (it != m_bgmHandles.end() && it->second != -1)
    {
        m_currentBgm = type;
        m_isBgmPaused = false;
        ChangeVolumeSoundMem(CalcBgmVolumeDxLib(), it->second);
        int playType = loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK;
        PlaySoundMem(it->second, playType, TRUE);
    }
}

void SoundManager::StopBGM()
{
    if (m_currentBgm != BgmType::None)
    {
        auto it = m_bgmHandles.find(m_currentBgm);
        if (it != m_bgmHandles.end() && it->second != -1)
        {
            StopSoundMem(it->second);
        }
        m_currentBgm = BgmType::None;
        m_isBgmPaused = false;
    }
}

void SoundManager::PauseBGM()
{
    if (m_currentBgm != BgmType::None && !m_isBgmPaused)
    {
        auto it = m_bgmHandles.find(m_currentBgm);
        if (it != m_bgmHandles.end() && it->second != -1)
        {
            StopSoundMem(it->second);
            m_isBgmPaused = true;
        }
    }
}

void SoundManager::ResumeBGM()
{
    if (m_currentBgm != BgmType::None && m_isBgmPaused)
    {
        auto it = m_bgmHandles.find(m_currentBgm);
        if (it != m_bgmHandles.end() && it->second != -1)
        {
            ChangeVolumeSoundMem(CalcBgmVolumeDxLib(), it->second);
            PlaySoundMem(it->second, DX_PLAYTYPE_LOOP, FALSE);
            m_isBgmPaused = false;
        }
    }
}

bool SoundManager::IsBGMPlaying() const
{
    if (m_currentBgm == BgmType::None || m_isBgmPaused)
    {
        return false;
    }
    auto it = m_bgmHandles.find(m_currentBgm);
    if (it != m_bgmHandles.end() && it->second != -1)
    {
        return CheckSoundMem(it->second) != 0;
    }
    return false;
}

void SoundManager::PlaySE(SeType type, float volumeScale)
{
    auto it = m_seHandles.find(type);
    if (it != m_seHandles.end() && it->second != -1)
    {
        ChangeVolumeSoundMem(CalcSeVolumeDxLib(volumeScale), it->second);
        PlaySoundMem(it->second, DX_PLAYTYPE_BACK, TRUE);
    }
}

void SoundManager::StopSE(SeType type)
{
    auto it = m_seHandles.find(type);
    if (it != m_seHandles.end() && it->second != -1)
    {
        StopSoundMem(it->second);
    }
}

void SoundManager::StopAllSE()
{
    for (auto& pair : m_seHandles)
    {
        if (pair.second != -1)
        {
            StopSoundMem(pair.second);
        }
    }
}

void SoundManager::SetBGMVolume(float volume)
{
    m_bgmVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    if (m_currentBgm != BgmType::None)
    {
        auto it = m_bgmHandles.find(m_currentBgm);
        if (it != m_bgmHandles.end() && it->second != -1)
        {
            ChangeVolumeSoundMem(CalcBgmVolumeDxLib(), it->second);
        }
    }
}

void SoundManager::SetSEVolume(float volume)
{
    m_seVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
}

void SoundManager::SetMasterVolume(float volume)
{
    m_masterVolume = (std::max)(0.0f, (std::min)(1.0f, volume));
    SetBGMVolume(m_bgmVolume);
}

void SoundManager::AdjustBGMVolume(float delta)
{
    SetBGMVolume(m_bgmVolume + delta);
    SaveSettings();
}

void SoundManager::AdjustSEVolume(float delta)
{
    SetSEVolume(m_seVolume + delta);
    SaveSettings();
}

void SoundManager::SaveSettings()
{
    FILE* fp = nullptr;
    fopen_s(&fp, "Resource/SoundConfig.txt", "w");
    if (fp)
    {
        fprintf(fp, "BGM_VOLUME=%.2f\n", m_bgmVolume);
        fprintf(fp, "SE_VOLUME=%.2f\n", m_seVolume);
        fclose(fp);
    }
}

void SoundManager::LoadSettings()
{
    FILE* fp = nullptr;
    fopen_s(&fp, "Resource/SoundConfig.txt", "r");
    if (fp)
    {
        char line[128];
        while (fgets(line, sizeof(line), fp))
        {
            float val = 0.0f;
            if (sscanf_s(line, "BGM_VOLUME=%f", &val) == 1)
            {
                m_bgmVolume = (std::max)(0.0f, (std::min)(1.0f, val));
            }
            else if (sscanf_s(line, "SE_VOLUME=%f", &val) == 1)
            {
                m_seVolume = (std::max)(0.0f, (std::min)(1.0f, val));
            }
        }
        fclose(fp);
    }
}

void SoundManager::StopAll()
{
    StopBGM();
    StopAllSE();
}
