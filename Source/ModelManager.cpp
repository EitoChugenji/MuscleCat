#include "ModelManager.h"
#include "ModelConfig.h"
#include "Common.h"
#include <cmath>

void ModelManager::Init() {
    Release();
}

void ModelManager::Release() {
    for (auto& pair : m_modelHandles) {
        if (pair.second != -1) {
            MV1DeleteModel(pair.second);
        }
    }
    m_modelHandles.clear();
}

int ModelManager::LoadModelHandle(const std::string& path) {
    if (path.empty()) return -1;

    auto it = m_modelHandles.find(path);
    if (it != m_modelHandles.end()) {
        return it->second;
    }

    // モデルファイル読み込み試行
    int handle = MV1LoadModel(path.c_str());
    m_modelHandles[path] = handle;
    return handle;
}

bool ModelManager::DrawModelIfLoaded(const std::string& path, const VECTOR& pos, float rotY, float scale, float rotX, float rotZ) {
    int handle = LoadModelHandle(path);
    if (handle == -1) {
        return false; // モデル未ロード/存在しないためフォールバック描画へ
    }

    // 3Dモデルの位置・回転・拡大率を設定して描画
    MV1SetPosition(handle, pos);
    MV1SetRotationXYZ(handle, VGet(rotX, rotY, rotZ));
    MV1SetScale(handle, VGet(scale, scale, scale));
    MV1DrawModel(handle);
    return true;
}

// ----------------------------------------------------------------------------
// プロシージャル3Dフォールバック描画
// ----------------------------------------------------------------------------

void ModelManager::DrawFallbackCat(const VECTOR& pos, float rotY, int repCount, bool isSoreness, bool isPouncing, float animTime) {
    // 猫の基本カラー設定（Rep数・筋肉痛・飛びつきで変化）
    unsigned int bodyColor;
    unsigned int earColor = GetColor(255, 180, 190);
    unsigned int eyeColor = GetColor(20, 20, 20);
    unsigned int dumbbellColor = GetColor(50, 50, 60);

    if (isSoreness) {
        bodyColor = GetColor(100, 160, 230); // 筋肉痛: 青ざめた色
    } else if (isPouncing) {
        bodyColor = GetColor(255, 90, 40);   // 飛びつき: 赤熱オレンジ
    } else if (repCount >= 4) {
        bodyColor = GetColor(255, 170, 40);  // 4 Rep以上: 黄金マッチョ
    } else if (repCount > 0) {
        bodyColor = GetColor(250, 195, 120); // 1-3 Rep: パンプアップ色
    } else {
        bodyColor = GetColor(240, 200, 150); // 0 Rep: 通常の茶白猫色
    }

    // マッチョ度に応じたスケール増分
    float muscleBonus = static_cast<float>(repCount) * 0.6f;
    if (muscleBonus > 6.0f) muscleBonus = 6.0f;

    // 前方向ベクトル・右方向ベクトル計算
    float sinR = std::sin(rotY);
    float cosR = std::cos(rotY);
    VECTOR forward = VGet(sinR, 0.0f, cosR);
    VECTOR right   = VGet(cosR, 0.0f, -sinR);

    // 1. 体（マッチョな胴体）
    VECTOR bodyPos = VGet(pos.x, pos.y + 18.0f + muscleBonus * 0.5f, pos.z);
    float bodyRadius = 14.0f + muscleBonus;
    DrawSphere3D(bodyPos, bodyRadius, 16, bodyColor, bodyColor, TRUE);

    // 2. 頭部
    VECTOR headPos = VAdd(bodyPos, VAdd(VScale(forward, 9.0f), VGet(0.0f, 12.0f, 0.0f)));
    DrawSphere3D(headPos, 11.5f, 16, bodyColor, bodyColor, TRUE);

    // 3. 猫耳（左右の円錐）
    VECTOR leftEarPos  = VAdd(headPos, VAdd(VScale(right, -6.0f), VGet(0.0f, 8.5f, 0.0f)));
    VECTOR rightEarPos = VAdd(headPos, VAdd(VScale(right, 6.0f), VGet(0.0f, 8.5f, 0.0f)));
    DrawCone3D(leftEarPos, VAdd(leftEarPos, VGet(0.0f, 7.0f, 0.0f)), 4.5f, 8, earColor, earColor, TRUE);
    DrawCone3D(rightEarPos, VAdd(rightEarPos, VGet(0.0f, 7.0f, 0.0f)), 4.5f, 8, earColor, earColor, TRUE);

    // 4. 目
    VECTOR leftEyePos  = VAdd(headPos, VAdd(VScale(forward, 10.0f), VAdd(VScale(right, -4.0f), VGet(0.0f, 2.2f, 0.0f))));
    VECTOR rightEyePos = VAdd(headPos, VAdd(VScale(forward, 10.0f), VAdd(VScale(right, 4.0f), VGet(0.0f, 2.2f, 0.0f))));
    DrawSphere3D(leftEyePos, 2.2f, 8, eyeColor, eyeColor, TRUE);
    DrawSphere3D(rightEyePos, 2.2f, 8, eyeColor, eyeColor, TRUE);

    // 5. マッチョな腕（両脇の力こぶ）
    float armArmOffset = 14.0f + muscleBonus * 1.5f;
    float armAngleSwing = std::sin(animTime * 12.0f) * 0.2f;
    VECTOR leftArmPos = VAdd(bodyPos, VAdd(VScale(right, -armArmOffset), VGet(0.0f, 3.0f, 0.0f)));
    VECTOR rightArmPos = VAdd(bodyPos, VAdd(VScale(right, armArmOffset), VGet(0.0f, 3.0f, 0.0f)));
    DrawSphere3D(leftArmPos, 7.0f + muscleBonus * 0.9f, 12, bodyColor, bodyColor, TRUE);
    DrawSphere3D(rightArmPos, 7.0f + muscleBonus * 0.9f, 12, bodyColor, bodyColor, TRUE);

    // ダンベル所持（マッチョ時）
    if (repCount > 0) {
        DrawCapsule3D(VAdd(leftArmPos, VGet(0.0f, -8.0f, 0.0f)), VAdd(leftArmPos, VGet(0.0f, 8.0f, 0.0f)), 4.2f, 8, dumbbellColor, dumbbellColor, TRUE);
        DrawCapsule3D(VAdd(rightArmPos, VGet(0.0f, -8.0f, 0.0f)), VAdd(rightArmPos, VGet(0.0f, 8.0f, 0.0f)), 4.2f, 8, dumbbellColor, dumbbellColor, TRUE);
    }

    // 6. 尻尾
    VECTOR tailBase = VSub(bodyPos, VScale(forward, 11.0f));
    VECTOR tailTip  = VAdd(tailBase, VAdd(VScale(forward, -9.0f), VGet(0.0f, 12.0f + std::sin(animTime * 8.0f) * 3.0f, 0.0f)));
    DrawCapsule3D(tailBase, tailTip, 3.5f, 8, bodyColor, bodyColor, TRUE);

    // 飛びつき時オーラエフェクト
    if (isPouncing) {
        DrawSphere3D(bodyPos, bodyRadius * 1.6f, 12, GetColor(255, 120, 0), GetColor(255, 200, 0), FALSE);
    }
}

void ModelManager::DrawFallbackNormalMouse(const VECTOR& pos, float rotY, float animTime) {
    unsigned int bodyColor = GetColor(160, 160, 170); // 灰色
    unsigned int earColor  = GetColor(255, 180, 190); // ピンク
    unsigned int eyeColor  = GetColor(20, 20, 20);

    float sinR = std::sin(rotY);
    float cosR = std::cos(rotY);
    VECTOR forward = VGet(sinR, 0.0f, cosR);
    VECTOR right   = VGet(cosR, 0.0f, -sinR);

    // 1. 体
    VECTOR bodyPos = VGet(pos.x, pos.y + 7.0f, pos.z);
    DrawSphere3D(bodyPos, 7.5f, 12, bodyColor, bodyColor, TRUE);

    // 2. 頭部
    VECTOR headPos = VAdd(bodyPos, VScale(forward, 5.5f));
    DrawSphere3D(headPos, 5.5f, 12, bodyColor, bodyColor, TRUE);

    // 3. 大きな耳
    VECTOR leftEarPos  = VAdd(headPos, VAdd(VScale(right, -4.0f), VGet(0.0f, 4.8f, 0.0f)));
    VECTOR rightEarPos = VAdd(headPos, VAdd(VScale(right, 4.0f), VGet(0.0f, 4.8f, 0.0f)));
    DrawSphere3D(leftEarPos, 3.2f, 8, earColor, earColor, TRUE);
    DrawSphere3D(rightEarPos, 3.2f, 8, earColor, earColor, TRUE);

    // 4. 目
    VECTOR leftEyePos  = VAdd(headPos, VAdd(VScale(forward, 4.2f), VAdd(VScale(right, -2.4f), VGet(0.0f, 1.5f, 0.0f))));
    VECTOR rightEyePos = VAdd(headPos, VAdd(VScale(forward, 4.2f), VAdd(VScale(right, 2.4f), VGet(0.0f, 1.5f, 0.0f))));
    DrawSphere3D(leftEyePos, 1.3f, 6, eyeColor, eyeColor, TRUE);
    DrawSphere3D(rightEyePos, 1.3f, 6, eyeColor, eyeColor, TRUE);

    // 5. 尻尾
    VECTOR tailBase = VSub(bodyPos, VScale(forward, 6.5f));
    VECTOR tailTip  = VAdd(tailBase, VAdd(VScale(forward, -7.0f), VGet(0.0f, 3.0f + std::sin(animTime * 15.0f) * 2.0f, 0.0f)));
    DrawCapsule3D(tailBase, tailTip, 1.6f, 6, earColor, earColor, TRUE);
}

void ModelManager::DrawFallbackFastMouse(const VECTOR& pos, float rotY, float animTime) {
    unsigned int bodyColor = GetColor(240, 190, 50);  // 金色/黄色
    unsigned int earColor  = GetColor(255, 140, 120); // 濃いピンク
    unsigned int eyeColor  = GetColor(220, 20, 20);   // 赤目

    float sinR = std::sin(rotY);
    float cosR = std::cos(rotY);
    VECTOR forward = VGet(sinR, 0.0f, cosR);
    VECTOR right   = VGet(cosR, 0.0f, -sinR);

    // 1. 体
    VECTOR bodyPos = VGet(pos.x, pos.y + 6.5f, pos.z);
    DrawSphere3D(bodyPos, 6.8f, 12, bodyColor, bodyColor, TRUE);

    // 2. 頭部
    VECTOR headPos = VAdd(bodyPos, VScale(forward, 5.8f));
    DrawSphere3D(headPos, 5.0f, 12, bodyColor, bodyColor, TRUE);

    // 3. 耳
    VECTOR leftEarPos  = VAdd(headPos, VAdd(VScale(right, -3.5f), VGet(0.0f, 4.5f, 0.0f)));
    VECTOR rightEarPos = VAdd(headPos, VAdd(VScale(right, 3.5f), VGet(0.0f, 4.5f, 0.0f)));
    DrawSphere3D(leftEarPos, 2.8f, 8, earColor, earColor, TRUE);
    DrawSphere3D(rightEarPos, 2.8f, 8, earColor, earColor, TRUE);

    // 4. 目（赤く光る）
    VECTOR leftEyePos  = VAdd(headPos, VAdd(VScale(forward, 4.0f), VAdd(VScale(right, -2.0f), VGet(0.0f, 1.4f, 0.0f))));
    VECTOR rightEyePos = VAdd(headPos, VAdd(VScale(forward, 4.0f), VAdd(VScale(right, 2.0f), VGet(0.0f, 1.4f, 0.0f))));
    DrawSphere3D(leftEyePos, 1.4f, 6, eyeColor, eyeColor, TRUE);
    DrawSphere3D(rightEyePos, 1.4f, 6, eyeColor, eyeColor, TRUE);

    // 5. 尻尾
    VECTOR tailBase = VSub(bodyPos, VScale(forward, 6.0f));
    VECTOR tailTip  = VAdd(tailBase, VAdd(VScale(forward, -7.5f), VGet(0.0f, 3.8f + std::sin(animTime * 20.0f) * 3.0f, 0.0f)));
    DrawCapsule3D(tailBase, tailTip, 1.6f, 6, earColor, earColor, TRUE);

    // 高速移動スピードエフェクトライン
    VECTOR backPos = VSub(pos, VScale(forward, 11.0f));
    DrawLine3D(pos, backPos, GetColor(255, 230, 100));
}

void ModelManager::DrawFallbackObstacle(const VECTOR& pos, float width, float height, float depth, unsigned int mainColor, unsigned int frameColor, const std::string& name) {
    // 3D直方体（トレーニング器具の台）
    VECTOR minPos = VGet(pos.x - width * 0.5f, pos.y, pos.z - depth * 0.5f);
    VECTOR maxPos = VGet(pos.x + width * 0.5f, pos.y + height, pos.z + depth * 0.5f);

    // 直方体の本体描画
    DrawCube3D(minPos, maxPos, mainColor, mainColor, TRUE);
    // 直方体のワイヤーフレーム（輪郭線）
    DrawCube3D(minPos, maxPos, frameColor, frameColor, FALSE);

    // 器具上のポールや装飾（器具らしさを演出）
    VECTOR leftPoleTop = VGet(pos.x - width * 0.35f, pos.y + height + 12.0f, pos.z);
    VECTOR rightPoleTop = VGet(pos.x + width * 0.35f, pos.y + height + 12.0f, pos.z);
    VECTOR leftPoleBot = VGet(pos.x - width * 0.35f, pos.y + height, pos.z);
    VECTOR rightPoleBot = VGet(pos.x + width * 0.35f, pos.y + height, pos.z);

    DrawCapsule3D(leftPoleBot, leftPoleTop, 1.5f, 6, frameColor, frameColor, TRUE);
    DrawCapsule3D(rightPoleBot, rightPoleTop, 1.5f, 6, frameColor, frameColor, TRUE);
    DrawCapsule3D(leftPoleTop, rightPoleTop, 1.2f, 6, GetColor(200, 200, 210), GetColor(200, 200, 210), TRUE);
}

void ModelManager::DrawFallbackStage(float halfW, float halfD, float wallH) {
    // 1. 床面（明るいチェッカーボード調フロア）
    const float tileSize = 40.0f;
    unsigned int floorColor1 = GetColor(232, 238, 248);
    unsigned int floorColor2 = GetColor(214, 224, 238);
    unsigned int gridLineColor = GetColor(180, 195, 215);

    for (float x = -halfW; x < halfW; x += tileSize) {
        for (float z = -halfD; z < halfD; z += tileSize) {
            float x2 = (x + tileSize > halfW) ? halfW : (x + tileSize);
            float z2 = (z + tileSize > halfD) ? halfD : (z + tileSize);

            int tileIndex = static_cast<int>(std::floor((x + halfW) / tileSize) + std::floor((z + halfD) / tileSize));
            unsigned int c = (tileIndex % 2 == 0) ? floorColor1 : floorColor2;

            VECTOR p0 = VGet(x,  0.0f, z);
            VECTOR p1 = VGet(x2, 0.0f, z);
            VECTOR p2 = VGet(x2, 0.0f, z2);
            VECTOR p3 = VGet(x,  0.0f, z2);

            DrawTriangle3D(p0, p1, p2, c, TRUE);
            DrawTriangle3D(p0, p2, p3, c, TRUE);
        }
    }

    // 2. 外周の壁・フェンス（視界を遮らない半透明ガラスフェンス＆くっきり手すり）
    unsigned int wallColor = GetColor(220, 235, 255);
    unsigned int wallBorderColor = GetColor(50, 140, 240);

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);

    // 北壁 (Z = +halfD)
    VECTOR nw_b = VGet(-halfW, 0.0f, halfD);
    VECTOR ne_b = VGet( halfW, 0.0f, halfD);
    VECTOR nw_t = VGet(-halfW, wallH, halfD);
    VECTOR ne_t = VGet( halfW, wallH, halfD);
    DrawTriangle3D(nw_b, ne_b, ne_t, wallColor, TRUE);
    DrawTriangle3D(nw_b, ne_t, nw_t, wallColor, TRUE);

    // 南壁 (Z = -halfD)
    VECTOR sw_b = VGet(-halfW, 0.0f, -halfD);
    VECTOR se_b = VGet( halfW, 0.0f, -halfD);
    VECTOR sw_t = VGet(-halfW, wallH, -halfD);
    VECTOR se_t = VGet( halfW, wallH, -halfD);
    DrawTriangle3D(se_b, sw_b, sw_t, wallColor, TRUE);
    DrawTriangle3D(se_b, sw_t, se_t, wallColor, TRUE);

    // 東壁 (X = +halfW)
    DrawTriangle3D(ne_b, se_b, se_t, wallColor, TRUE);
    DrawTriangle3D(ne_b, se_t, ne_t, wallColor, TRUE);

    // 西壁 (X = -halfW)
    DrawTriangle3D(sw_b, nw_b, nw_t, wallColor, TRUE);
    DrawTriangle3D(sw_b, nw_t, sw_t, wallColor, TRUE);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 手すり（境界ライン）
    DrawLine3D(nw_t, ne_t, wallBorderColor);
    DrawLine3D(sw_t, se_t, wallBorderColor);
    DrawLine3D(se_t, ne_t, wallBorderColor);
    DrawLine3D(sw_t, nw_t, wallBorderColor);
}
