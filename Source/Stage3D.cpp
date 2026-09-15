#include "Stage3D.h"
#include "Config.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "Common.h"

Stage3D::Stage3D()
    : m_halfWidth(Config::STAGE_HALF_WIDTH)
    , m_halfDepth(Config::STAGE_HALF_DEPTH)
    , m_wallHeight(Config::WALL_HEIGHT)
    , m_modelPath(ModelConfig::STAGE_MODEL_PATH) {
}

void Stage3D::Init() {
    // 画面クリア背景色（明るい開放的なスカイブルー）
    SetBackgroundColor(205, 228, 250);

    // 3Dライティング設定（明るく影を薄くして視認性を大幅向上）
    SetUseLighting(TRUE);
    SetGlobalAmbientLight(GetColorF(0.72f, 0.72f, 0.76f, 1.0f));
    ChangeLightTypeDir(VGet(0.4f, -0.9f, 0.3f));
    SetLightDifColor(GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
    SetLightSpcColor(GetColorF(0.5f, 0.5f, 0.5f, 1.0f));

    // フォグ設定（広大で明るい空気感）
    SetFogEnable(TRUE);
    SetFogColor(205, 228, 250);
    SetFogStartEnd(1400.0f, 3500.0f);
}

void Stage3D::Draw3D() {
    if (!ModelManager::GetInstance().DrawModelIfLoaded(m_modelPath, VGet(0.0f, 0.0f, 0.0f), 0.0f, ModelConfig::STAGE_MODEL_SCALE)) {
        ModelManager::GetInstance().DrawFallbackStage(m_halfWidth, m_halfDepth, m_wallHeight);
    }
}

bool Stage3D::ClampToBounds(VECTOR& outPos, float radius) const {
    bool collided = false;
    float minX = -m_halfWidth + radius;
    float maxX =  m_halfWidth - radius;
    float minZ = -m_halfDepth + radius;
    float maxZ =  m_halfDepth - radius;

    if (outPos.x < minX) { outPos.x = minX; collided = true; }
    if (outPos.x > maxX) { outPos.x = maxX; collided = true; }
    if (outPos.z < minZ) { outPos.z = minZ; collided = true; }
    if (outPos.z > maxZ) { outPos.z = maxZ; collided = true; }

    return collided;
}
