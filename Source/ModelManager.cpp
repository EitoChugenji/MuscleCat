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
        bodyColor = GetColor(90, 180, 255);  // 筋肉痛: コミカルな青ざめ水色
    } else if (isPouncing) {
        bodyColor = GetColor(255, 80, 20);   // 飛びつき: ド派手なフレイムレッド
    } else if (repCount >= 15) {
        bodyColor = GetColor(255, 220, 0);   // 15 Rep以上: カンスト神マッスルゴールド
    } else if (repCount >= 4) {
        bodyColor = GetColor(255, 175, 20);  // 4 Rep以上: 黄金マッチョ
    } else if (repCount > 0) {
        bodyColor = GetColor(255, 195, 80);  // 1-3 Rep: パンプアップオレンジ
    } else {
        bodyColor = GetColor(255, 210, 140); // 0 Rep: 明るいアニメ茶白猫
    }

    // マッチョ度に応じたスケール増分（最大Lv15で頭打ち）
    int effRep = (repCount > 15) ? 15 : repCount;
    float muscleBonus = static_cast<float>(effRep) * 0.6f;
    if (muscleBonus > 8.0f) muscleBonus = 8.0f;

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

    // 4. アニメ調のパッチリ目（黒目＋白ハイライト）
    VECTOR leftEyePos  = VAdd(headPos, VAdd(VScale(forward, 10.0f), VAdd(VScale(right, -4.0f), VGet(0.0f, 2.2f, 0.0f))));
    VECTOR rightEyePos = VAdd(headPos, VAdd(VScale(forward, 10.0f), VAdd(VScale(right, 4.0f), VGet(0.0f, 2.2f, 0.0f))));
    DrawSphere3D(leftEyePos, 2.4f, 8, eyeColor, eyeColor, TRUE);
    DrawSphere3D(rightEyePos, 2.4f, 8, eyeColor, eyeColor, TRUE);
    // キラッと光るハイライト
    VECTOR leftHi  = VAdd(leftEyePos, VAdd(VScale(forward, 1.0f), VGet(0.0f, 0.8f, 0.0f)));
    VECTOR rightHi = VAdd(rightEyePos, VAdd(VScale(forward, 1.0f), VGet(0.0f, 0.8f, 0.0f)));
    DrawSphere3D(leftHi, 0.9f, 6, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);
    DrawSphere3D(rightHi, 0.9f, 6, GetColor(255, 255, 255), GetColor(255, 255, 255), TRUE);

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
    // 3D直方体（家具の本体）
    VECTOR minPos = VGet(pos.x - width * 0.5f, pos.y, pos.z - depth * 0.5f);
    VECTOR maxPos = VGet(pos.x + width * 0.5f, pos.y + height, pos.z + depth * 0.5f);

    // 家具本体の描画
    DrawCube3D(minPos, maxPos, mainColor, mainColor, TRUE);
    DrawCube3D(minPos, maxPos, frameColor, frameColor, FALSE);

    // ------------------------------------------------------------------------
    // 家具の種類に応じたディテール装飾（家の中らしさを演出）
    // ------------------------------------------------------------------------
    if (name.find("SOFA") != std::string::npos) {
        // ソファー：背もたれと肘掛け
        VECTOR backMin = VGet(minPos.x, maxPos.y, maxPos.z - depth * 0.3f);
        VECTOR backMax = VGet(maxPos.x, maxPos.y + 16.0f, maxPos.z);
        DrawCube3D(backMin, backMax, frameColor, frameColor, TRUE);
        DrawCube3D(backMin, backMax, mainColor, mainColor, FALSE);

        // 左右の肘掛け
        VECTOR armLMin = VGet(minPos.x, maxPos.y, minPos.z);
        VECTOR armLMax = VGet(minPos.x + width * 0.18f, maxPos.y + 10.0f, maxPos.z);
        VECTOR armRMin = VGet(maxPos.x - width * 0.18f, maxPos.y, minPos.z);
        VECTOR armRMax = VGet(maxPos.x, maxPos.y + 10.0f, maxPos.z);
        DrawCube3D(armLMin, armLMax, frameColor, frameColor, TRUE);
        DrawCube3D(armRMin, armRMax, frameColor, frameColor, TRUE);
    } else if (name.find("TV") != std::string::npos) {
        // テレビ台：薄型テレビの画面とスタンド
        VECTOR tvMin = VGet(pos.x - width * 0.38f, maxPos.y + 3.0f, pos.z - 3.0f);
        VECTOR tvMax = VGet(pos.x + width * 0.38f, maxPos.y + 36.0f, pos.z + 3.0f);
        DrawCube3D(tvMin, tvMax, GetColor(25, 25, 30), GetColor(25, 25, 30), TRUE);
        // 画面の青白い発光
        VECTOR screenMin = VGet(tvMin.x + 3.0f, tvMin.y + 3.0f, tvMin.z - 0.5f);
        VECTOR screenMax = VGet(tvMax.x - 3.0f, tvMax.y - 3.0f, tvMin.z);
        DrawCube3D(screenMin, screenMax, GetColor(160, 200, 240), GetColor(160, 200, 240), TRUE);
    } else if (name.find("TABLE") != std::string::npos) {
        // テーブル：テーブルクロスと食器
        VECTOR clothMin = VGet(pos.x - width * 0.42f, maxPos.y + 0.5f, pos.z - depth * 0.42f);
        VECTOR clothMax = VGet(pos.x + width * 0.42f, maxPos.y + 1.0f, pos.z + depth * 0.42f);
        DrawCube3D(clothMin, clothMax, GetColor(250, 245, 235), GetColor(250, 245, 235), TRUE);
        // マグカップ
        DrawCapsule3D(VGet(pos.x, maxPos.y + 1.0f, pos.z), VGet(pos.x, maxPos.y + 8.0f, pos.z), 3.0f, 8, GetColor(230, 80, 70), GetColor(230, 80, 70), TRUE);
    } else if (name.find("CAT TOWER") != std::string::npos) {
        // キャットタワー：支柱と展望台
        VECTOR towerTop = VGet(pos.x, maxPos.y + 26.0f, pos.z);
        DrawCapsule3D(VGet(pos.x, maxPos.y, pos.z), towerTop, 3.5f, 8, GetColor(200, 180, 140), GetColor(200, 180, 140), TRUE);
        DrawCube3D(VGet(pos.x - 22.0f, towerTop.y, pos.z - 22.0f), VGet(pos.x + 22.0f, towerTop.y + 5.0f, pos.z + 22.0f), GetColor(235, 215, 180), GetColor(180, 150, 100), TRUE);
    } else {
        // 本棚やチェスト：天板の装飾
        VECTOR topDecoMin = VGet(minPos.x + 4.0f, maxPos.y, minPos.z + 4.0f);
        VECTOR topDecoMax = VGet(maxPos.x - 4.0f, maxPos.y + 6.0f, maxPos.z - 4.0f);
        DrawCube3D(topDecoMin, topDecoMax, frameColor, frameColor, TRUE);
    }
}

void ModelManager::DrawFallbackStage(float halfW, float halfD, float wallH) {
    // ------------------------------------------------------------------------
    // 1. 床面：温かみのあるナチュラルウッドの木目調フローリング
    // ------------------------------------------------------------------------
    const float plankW = 25.0f; // フローリング板の幅
    const float plankL = 95.0f; // 板の長さ
    unsigned int woodBase1 = GetColor(250, 222, 175); // 明るいアニメ調のゴールデンメープル
    unsigned int woodBase2 = GetColor(240, 208, 158); // 木目バリエーション
    unsigned int woodJoint = GetColor(190, 150, 105); // 板の目地ライン

    for (float x = -halfW; x < halfW; x += plankW) {
        float x2 = (x + plankW > halfW) ? halfW : (x + plankW);
        int colIndex = static_cast<int>(std::floor((x + halfW) / plankW));
        float zOffset = (colIndex % 3) * (plankL * 0.33f); // レンガ積み調の千鳥配置

        for (float z = -halfD - zOffset; z < halfD; z += plankL) {
            float zStart = (z < -halfD) ? -halfD : z;
            float zEnd = (z + plankL > halfD) ? halfD : (z + plankL);
            if (zStart >= zEnd) continue;

            int rowIndex = static_cast<int>(std::floor((z + halfD) / plankL));
            unsigned int c = ((colIndex + rowIndex) % 2 == 0) ? woodBase1 : woodBase2;

            VECTOR p0 = VGet(x,  0.0f, zStart);
            VECTOR p1 = VGet(x2, 0.0f, zStart);
            VECTOR p2 = VGet(x2, 0.0f, zEnd);
            VECTOR p3 = VGet(x,  0.0f, zEnd);

            DrawTriangle3D(p0, p1, p2, c, TRUE);
            DrawTriangle3D(p0, p2, p3, c, TRUE);

            // 板の継ぎ目ライン
            DrawLine3D(p0, p1, woodJoint);
            DrawLine3D(p1, p2, woodJoint);
        }
    }

    // ------------------------------------------------------------------------
    // 2. リビングの中央ラグマット（アニメ調のポップなパステルカラー）
    // ------------------------------------------------------------------------
    float rugW = 340.0f;
    float rugD = 240.0f;
    unsigned int rugColor = GetColor(120, 215, 210);       // 鮮やかなアニメ調ミントターコイズ
    unsigned int rugBorderColor = GetColor(255, 235, 90);  // ポップなイエローのフチ
    VECTOR r0 = VGet(-rugW * 0.5f, 0.3f, -rugD * 0.5f);
    VECTOR r1 = VGet( rugW * 0.5f, 0.3f, -rugD * 0.5f);
    VECTOR r2 = VGet( rugW * 0.5f, 0.3f,  rugD * 0.5f);
    VECTOR r3 = VGet(-rugW * 0.5f, 0.3f,  rugD * 0.5f);
    DrawTriangle3D(r0, r1, r2, rugColor, TRUE);
    DrawTriangle3D(r0, r2, r3, rugColor, TRUE);
    DrawLine3D(r0, r1, rugBorderColor);
    DrawLine3D(r1, r2, rugBorderColor);
    DrawLine3D(r2, r3, rugBorderColor);
    DrawLine3D(r3, r0, rugBorderColor);

    // ------------------------------------------------------------------------
    // 3. お部屋の壁・巾木（明るいアニメ調ホワイト壁）
    // ------------------------------------------------------------------------
    unsigned int wallBaseColor = GetColor(255, 252, 245); // 明るいアニメホワイト
    unsigned int skirtingBoardColor = GetColor(180, 125, 75); // ポップな木製巾木
    unsigned int wallTrimColor = GetColor(120, 80, 50);       // クッキリ枠線ライン

    float skirtH = 7.0f; // 巾木の高さ

    // 北壁 (Z = +halfD)
    VECTOR nw_b = VGet(-halfW, 0.0f, halfD);
    VECTOR ne_b = VGet( halfW, 0.0f, halfD);
    VECTOR nw_s = VGet(-halfW, skirtH, halfD);
    VECTOR ne_s = VGet( halfW, skirtH, halfD);
    VECTOR nw_t = VGet(-halfW, wallH, halfD);
    VECTOR ne_t = VGet( halfW, wallH, halfD);
    // 巾木
    DrawTriangle3D(nw_b, ne_b, ne_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(nw_b, ne_s, nw_s, skirtingBoardColor, TRUE);
    // 壁本体
    DrawTriangle3D(nw_s, ne_s, ne_t, wallBaseColor, TRUE);
    DrawTriangle3D(nw_s, ne_t, nw_t, wallBaseColor, TRUE);

    // 南壁 (Z = -halfD) - 手前側はカメラ視界確保のため低めの腰壁フェンス風
    VECTOR sw_b = VGet(-halfW, 0.0f, -halfD);
    VECTOR se_b = VGet( halfW, 0.0f, -halfD);
    VECTOR sw_s = VGet(-halfW, skirtH, -halfD);
    VECTOR se_s = VGet( halfW, skirtH, -halfD);
    VECTOR sw_t = VGet(-halfW, wallH * 0.6f, -halfD);
    VECTOR se_t = VGet( halfW, wallH * 0.6f, -halfD);
    DrawTriangle3D(se_b, sw_b, sw_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(se_b, sw_s, se_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(se_s, sw_s, sw_t, wallBaseColor, TRUE);
    DrawTriangle3D(se_s, sw_t, se_t, wallBaseColor, TRUE);

    // 東壁 (X = +halfW)
    VECTOR ee_s = VGet(halfW, skirtH, -halfD);
    VECTOR en_s = VGet(halfW, skirtH,  halfD);
    VECTOR ee_t = VGet(halfW, wallH, -halfD);
    VECTOR en_t = VGet(halfW, wallH,  halfD);
    DrawTriangle3D(ne_b, se_b, ee_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(ne_b, ee_s, en_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(en_s, ee_s, ee_t, wallBaseColor, TRUE);
    DrawTriangle3D(en_s, ee_t, en_t, wallBaseColor, TRUE);

    // 西壁 (X = -halfW)
    VECTOR ww_s = VGet(-halfW, skirtH, -halfD);
    VECTOR wn_s = VGet(-halfW, skirtH,  halfD);
    VECTOR ww_t = VGet(-halfW, wallH, -halfD);
    VECTOR wn_t = VGet(-halfW, wallH,  halfD);
    DrawTriangle3D(sw_b, nw_b, wn_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(sw_b, wn_s, ww_s, skirtingBoardColor, TRUE);
    DrawTriangle3D(ww_s, wn_s, wn_t, wallBaseColor, TRUE);
    DrawTriangle3D(ww_s, wn_t, ww_t, wallBaseColor, TRUE);

    // 壁上の枠線トリムライン
    DrawLine3D(nw_t, ne_t, wallTrimColor);
    DrawLine3D(sw_t, se_t, wallTrimColor);
    DrawLine3D(ee_t, en_t, wallTrimColor);
    DrawLine3D(ww_t, wn_t, wallTrimColor);
}
