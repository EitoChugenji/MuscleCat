#pragma once
#include "DxLib.h"
#include <string>
#include <unordered_map>

// ModelManager クラス (シングルトン)
// 3Dモデル読み込み・管理・プロシージャルフォールバック描画
class ModelManager
{
private:
    std::unordered_map<std::string, int> m_modelHandles;

    ModelManager() = default;
    ~ModelManager() = default;

public:
    static ModelManager& GetInstance()
    {
        static ModelManager instance;
        return instance;
    }

    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void Init();
    void Release();

    // モデル読み込み（未ロードならロードし、ハンドルを返す。存在しない場合は -1）
    int LoadModelHandle(const std::string& path);

    // モデル描画ヘルパー（ファイルが存在すればモデル描画して true を返す。存在しなければ false を返す）
    bool DrawModelIfLoaded(
        const std::string& path,
        const VECTOR& pos,
        float rotY,
        float scale,
        float rotX = 0.0f,
        float rotZ = 0.0f
    );

    // プレイヤー（猫）のフォールバック描画
    void DrawFallbackCat(
        const VECTOR& pos,
        float rotY,
        int repCount,
        bool isSoreness,
        bool isPouncing,
        float animTime
    );

    // ネズミ（通常）のフォールバック描画
    void DrawFallbackNormalMouse(const VECTOR& pos, float rotY, float animTime);

    // ネズミ（高速）のフォールバック描画
    void DrawFallbackFastMouse(const VECTOR& pos, float rotY, float animTime);

    // 障害物のフォールバック描画
    void DrawFallbackObstacle(
        const VECTOR& pos,
        float width,
        float height,
        float depth,
        unsigned int mainColor,
        unsigned int frameColor,
        const std::string& name
    );

    // ステージ（床・壁）のフォールバック描画
    void DrawFallbackStage(float halfW, float halfD, float wallH);
};

