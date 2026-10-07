#include "Player3D.h"
#include "Camera3D.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "FontManager.h"
#include "EffectManager.h"
#include "Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

Player3D::Player3D(const VECTOR& pos)
    : GameObject3D(pos, 18.0f, ObjectType::Player)
    , m_speed(SPEED_INITIAL)
    , m_state(MuscleState::Normal)
    , m_repCount(0)
    , m_pumpDecayTimer(0)
{
    m_rotY = 0.0f;

    // 3Dモデルおよびアニメーションの初期化
    InitModel();
}

Player3D::~Player3D()
{
    if (m_modelHandle != -1)
    {
        MV1DeleteModel(m_modelHandle);
        m_modelHandle = -1;
    }

    if (m_barbellModelHandle != -1)
    {
        MV1DeleteModel(m_barbellModelHandle);
        m_barbellModelHandle = -1;
    }
}

void Player3D::InitModel()
{
    // ベースモデルを読み込み、アニメーション個別制御用に複製
    int baseHandle = ModelManager::GetInstance().LoadModelHandle(ModelConfig::CAT_MODEL_PATH);

    if (baseHandle != -1)
    {
        m_modelHandle = MV1DuplicateModel(baseHandle);

        if (m_modelHandle != -1)
        {
            // モデル内のアニメーション名から各インデックスを自動検出（大文字小文字問わず柔軟に検索）
            auto findAnim = [this](const std::vector<std::string>& keywords, int fallbackIndex) -> int
            {
                int num = MV1GetAnimNum(m_modelHandle);
                for (int i = 0; i < num; ++i)
                {
                    const char* name = MV1GetAnimName(m_modelHandle, i);
                    if (!name) continue;
                    std::string s(name);
                    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    for (const auto& kw : keywords)
                    {
                        if (s.find(kw) != std::string::npos)
                        {
                            return i;
                        }
                    }
                }
                return fallbackIndex;
            };

            m_animIndexRun      = findAnim({ "01_run", "run" }, 0);
            m_animIndexAttack   = findAnim({ "02_attack", "attack" }, 1);
            m_animIndexSquat    = findAnim({ "03_squat", "squat" }, 2);
            m_animIndexIdol     = findAnim({ "04_idol", "idol", "idle" }, 3);
            m_animIndexPush     = findAnim({ "05_push", "push" }, 4);
            m_animIndexSoreness = findAnim({ "06_musclesoreness", "musclesorena", "soreness", "sore" }, 5);

            // 初期状態はアイドルアニメーション
            m_currentAnimIndex = m_animIndexIdol;
            m_attachAnimIndex = MV1AttachAnim(m_modelHandle, m_currentAnimIndex, -1, FALSE);
            m_animPlayTime = 0.0f;

            // バーベル配置用の両手ボーンフレームを検索・キャッシュ
            m_lHandFrame = MV1SearchFrame(m_modelHandle, "mixamorig:LeftHand");
            m_rHandFrame = MV1SearchFrame(m_modelHandle, "mixamorig:RightHand");
            m_lHandKnuckleFrame = MV1SearchFrame(m_modelHandle, "mixamorig:LeftHandMiddle1");
            m_rHandKnuckleFrame = MV1SearchFrame(m_modelHandle, "mixamorig:RightHandMiddle1");

            // テクスチャ抜け防止および自動復旧
            int texNum = MV1GetTextureNum(m_modelHandle);
            for (int i = 0; i < texNum; ++i)
            {
                int grHandle = MV1GetTextureGraphHandle(m_modelHandle, i);
                if (grHandle == -1)
                {
                    const char* candidatePaths[] = {
                        "Resource/Models/CatModel/MascleCat.fbm/0.jpg",
                        "Resource/Models/CatModel/cat.fbm/0.jpg",
                        "Resource/Models/CatModel/0.jpg"
                    };

                    for (const char* cPath : candidatePaths)
                    {
                        int loadedGr = LoadGraph(cPath);
                        if (loadedGr != -1)
                        {
                            MV1SetTextureGraphHandle(m_modelHandle, i, loadedGr, FALSE);
                            break;
                        }
                    }
                }
            }
        }
    }

    // バーベルアクセサリモデルの読み込み
    int barbellBase = ModelManager::GetInstance().LoadModelHandle(ModelConfig::BARBELL_MODEL_PATH);
    if (barbellBase != -1)
    {
        m_barbellModelHandle = MV1DuplicateModel(barbellBase);
    }
}

void Player3D::UpdateAnimation(bool isMoving)
{
    if (m_modelHandle == -1)
    {
        return;
    }

    // 再生すべきアニメーションの判定
    // 優先度1: タックル（Eキー: Pushアニメーション）
    // 優先度2: 飛びつき（SHIFT/X/Cキー: Attackアニメーション）
    // 優先度3: 筋肉痛（QTE失敗時など: MuscleSorena / MuscleSorenessアニメーション）
    // 優先度4: スクワット（筋トレQTE中: squatアニメーション）
    // 優先度5: 走り（移動中: Runアニメーション）
    // 優先度6: 待機（アイドル: Idolアニメーション）
    int targetAnim = m_animIndexIdol;
    float playSpeed = 0.5f;

    if (m_isTackling)
    {
        targetAnim = m_animIndexPush;      // Armature|05_Push
        playSpeed = 1.0f;
    }
    else if (m_isPouncing)
    {
        targetAnim = m_animIndexAttack;    // Armature|02_Attack
        playSpeed = 1.0f;
    }
    else if (m_state == MuscleState::Soreness)
    {
        targetAnim = m_animIndexSoreness;  // Armature|06_MuscleSoreness
        playSpeed = 0.6f;
    }
    else if (m_isSkillChecking || m_isTitleSquat)
    {
        targetAnim = m_animIndexSquat;     // Armature|03_squat
        playSpeed = 1.0f;
    }
    else if (isMoving)
    {
        targetAnim = m_animIndexRun;       // Armature|01_Run
        playSpeed = 0.7f + (m_speed / SPEED_INITIAL) * 0.4f;
    }

    // アニメーションの切り替え判定
    if (m_currentAnimIndex != targetAnim)
    {
        if (m_attachAnimIndex != -1)
        {
            MV1DetachAnim(m_modelHandle, m_attachAnimIndex);
            m_attachAnimIndex = -1;
        }

        m_currentAnimIndex = targetAnim;
        m_attachAnimIndex = MV1AttachAnim(m_modelHandle, m_currentAnimIndex, -1, FALSE);
        m_animPlayTime = 0.0f;
    }

    // アニメーション再生時間の進行
    if (m_attachAnimIndex != -1)
    {
        float totalTime = MV1GetAnimTotalTime(m_modelHandle, m_currentAnimIndex);
        m_animPlayTime += playSpeed;

        if (totalTime > 0.0f)
        {
            while (m_animPlayTime >= totalTime)
            {
                m_animPlayTime -= totalTime;
            }
        }

        MV1SetAttachAnimTime(m_modelHandle, m_attachAnimIndex, m_animPlayTime);
    }
}

void Player3D::Update()
{
    // 通常のUpdate（後方互換）
}

void Player3D::UpdateWithCamera(const Camera3D& camera, const std::vector<VECTOR>& targetPositions)
{
    m_animFrame++;
    m_animTime += 1.0f / 60.0f;

    bool isMoving = false;

    // キー入力受付（筋トレ: SPACE/Z、飛びつき: SHIFT/X/C、タックル: E）
    bool currentTriggerKey = (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_Z));
    bool isTriggerJustPressed = currentTriggerKey && !m_prevTriggerKey;
    bool isTriggerJustReleased = !currentTriggerKey && m_prevTriggerKey;
    m_prevTriggerKey = currentTriggerKey;

    bool currentPounceKey = (CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT) ||
                             CheckHitKey(KEY_INPUT_X) || CheckHitKey(KEY_INPUT_C));
    bool isPounceJustPressed = currentPounceKey && !m_prevPounceKey;
    m_prevPounceKey = currentPounceKey;

    bool currentTackleKey = CheckHitKey(KEY_INPUT_E);
    bool isTackleJustPressed = currentTackleKey && !m_prevTackleKey;
    m_prevTackleKey = currentTackleKey;

    if (m_resultShowTimer > 0)
    {
        m_resultShowTimer--;
    }

    if (m_cheatNoCooldown)
    {
        m_pounceCooldown = 0;
        m_tackleCooldown = 0;
        m_stunTimer = 0;
        m_sorenessTimer = 0;
        m_pumpDecayTimer = PUMP_DECAY_FRAMES;
    }
    
    else
    {
        if (m_pounceCooldown > 0)
        {
            m_pounceCooldown--;
        }
        
        if (m_tackleCooldown > 0)
        {
            m_tackleCooldown--;
        }
    }

    // 残像トレイルのフェードアウト処理（素早く抜けて本体を見えやすく）
    for (auto it = m_trails.begin(); it != m_trails.end(); )
    {
        it->alpha -= 35;
        if (it->alpha <= 0)
        {
            it = m_trails.erase(it);
        }
        
        else
        {
            ++it;
        }
    }

    // スタン（壁・家具激突）処理
    if (m_stunTimer > 0)
    {
        m_stunTimer--;
        m_isTackling = false;
        m_isPouncing = false;
        m_pos.y = 0.0f;
        UpdateAnimation(false);
        return;
    }

    // タックル（Tackle）アクション実行中処理
    if (m_isTackling)
    {
        m_tackleTimer--;

        // 残像記録（半透明で控えめにし、本体のPushモーションを際立たせる）
        if (m_tackleTimer % 2 == 0)
        {
            m_trails.push_back({ m_pos, m_rotY, m_repCount, 85 });
        }

        // 地面を滑るような超高速直進
        float tSpeed = GetTackleSpeed();
        m_pos.x += m_tackleDir.x * tSpeed;
        m_pos.z += m_tackleDir.z * tSpeed;
        m_pos.y = 0.0f;

        if (m_tackleTimer <= 0)
        {
            m_isTackling = false;
            m_tackleCooldown = GetTackleCooldownMax();
        }

        // タックル中のアニメーション（Push）を更新
        UpdateAnimation(false);
        return;
    }

    // 飛びつき（Pounce）アクション実行中処理
    if (m_isPouncing)
    {
        m_pounceTimer--;

        int duration = GetPounceDuration();
        float progress = 1.0f - static_cast<float>(m_pounceTimer) / static_cast<float>(duration);
        float jumpHeight = 15.0f + static_cast<float>(m_repCount - 4) * 3.0f;
        float currentY = std::sin(progress * MathHelper::PI) * jumpHeight;
        m_pos.y = currentY;

        // 3D残像記録（控えめな半透明）
        if (m_pounceTimer % 2 == 0)
        {
            m_trails.push_back({ m_pos, m_rotY, m_repCount, 75 });
        }

        // 飛翔中の緩やかな追尾ステアリング（最寄りのネズミへ少し曲がる）
        if (!targetPositions.empty())
        {
            float closestDistSq = 260.0f * 260.0f;
            VECTOR steerTarget = VGet(0.0f, 0.0f, 0.0f);
            bool steerFound = false;

            for (const auto& tPos : targetPositions)
            {
                float dx = tPos.x - m_pos.x;
                float dz = tPos.z - m_pos.z;
                float distSq = dx * dx + dz * dz;
                if (distSq < closestDistSq && distSq > 4.0f)
                {
                    float dist = std::sqrt(distSq);
                    float dot = m_pounceDir.x * (dx / dist) + m_pounceDir.z * (dz / dist);
                    // 前方約±75度以内のターゲット
                    if (dot > 0.25f)
                    {
                        closestDistSq = distSq;
                        steerTarget = tPos;
                        steerFound = true;
                    }
                }
            }

            if (steerFound)
            {
                float targetAngle = std::atan2(steerTarget.x - m_pos.x, steerTarget.z - m_pos.z);
                // 毎フレーム少しずつ滑らかにネズミの逃走方向へ誘導（自然なカーブ）
                m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.12f);
                m_pounceDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
            }
        }

        // Rep数に応じた突進速度で直進
        float pSpeed = GetPounceSpeed();
        m_pos.x += m_pounceDir.x * pSpeed;
        m_pos.z += m_pounceDir.z * pSpeed;

        if (m_pounceTimer <= 0)
        {
            m_isPouncing = false;
            m_pos.y = 0.0f;
            m_pounceCooldown = GetPounceCooldownMax();
        }
    }
    
    else
    {
        m_pos.y = 0.0f;

        // 通常 / 筋肉痛 / 筋トレ（Rep進行 & 15秒減衰）処理
        if (m_state == MuscleState::Soreness)
        {
            // 筋肉痛状態: 5秒間完全移動不可（速度 0.0）
            m_speed = SPEED_SORENESS;
            m_isSkillChecking = false;
            m_pumpDecayTimer = 0;

            if (--m_sorenessTimer <= 0)
            {
                m_sorenessTimer = 0;
                m_state = MuscleState::Normal;
                m_repCount = 0;
                m_speed = SPEED_INITIAL;
            }
        }
        
        else
        {
            // 15秒間何もしなければ 0 Rep に戻る減衰処理
            if (m_repCount > 0 && !m_isSkillChecking)
            {
                if (--m_pumpDecayTimer <= 0)
                {
                    m_pumpDecayTimer = 0;
                    m_repCount = 0;
                    m_speed = SPEED_INITIAL;
                    m_state = MuscleState::Normal;
                    m_resultShowTimer = 60;
                    m_wasDecayed = true;
                }
            }

            if (m_repCount > 0)
            {
                m_speed = SPEED_INITIAL + static_cast<float>(GetEffectiveRep()) * 0.15f;
                m_state = MuscleState::Muscular;
            }
            
            else
            {
                m_speed = SPEED_INITIAL;
                m_state = MuscleState::Normal;
            }

            if (!m_isSkillChecking)
            {
                // タックル発動チェック（Eキー）
                if (CanTackle() && isTackleJustPressed)
                {
                    m_isTackling = true;
                    m_tackleTimer = GetTackleDuration();
                    m_tackleDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));

                    // Effekseer タックル衝撃波リングエフェクト再生
                    EffectManager::GetInstance().PlayTackleEffect(m_pos, m_tackleDir);

                    // 即座にタックルアニメーション（Push）を開始
                    UpdateAnimation(false);
                    return;
                }

                // 飛びつき発動チェック（4 Rep以上特権）
                if (CanPounce() && isPounceJustPressed)
                {
                    m_isPouncing = true;
                    m_pounceTimer = GetPounceDuration();

                    VECTOR currentFacing = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));

                    // 前方の最寄りネズミを探索して発動方向をアシスト補正（多少の追尾）
                    float bestDistSq = 260.0f * 260.0f;
                    VECTOR bestTarget = VGet(0.0f, 0.0f, 0.0f);
                    bool foundTarget = false;

                    for (const auto& tPos : targetPositions)
                    {
                        float dx = tPos.x - m_pos.x;
                        float dz = tPos.z - m_pos.z;
                        float distSq = dx * dx + dz * dz;
                        if (distSq < bestDistSq && distSq > 4.0f)
                        {
                            float dist = std::sqrt(distSq);
                            float dot = currentFacing.x * (dx / dist) + currentFacing.z * (dz / dist);
                            // 前方視野（約±70度）以内のネズミ
                            if (dot > 0.35f)
                            {
                                bestDistSq = distSq;
                                bestTarget = tPos;
                                foundTarget = true;
                            }
                        }
                    }

                    if (foundTarget)
                    {
                        float targetAngle = std::atan2(bestTarget.x - m_pos.x, bestTarget.z - m_pos.z);
                        // 発動時の向きをターゲット方向へ自然に補正（約55%引き寄せ）
                        m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.55f);
                        m_pounceDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
                    }
                    else
                    {
                        m_pounceDir = currentFacing;
                    }

                    // 即座に飛びつきアニメーション（Attack）を開始
                    UpdateAnimation(false);
                    return;
                }

                // 移動入力処理 (WASDキー) - カメラのXZ平面基準
                VECTOR forwardXZ = camera.GetForwardXZ();
                VECTOR rightXZ   = camera.GetRightXZ();
                VECTOR moveDir   = VGet(0.0f, 0.0f, 0.0f);

                if (CheckHitKey(KEY_INPUT_W))
                {
                    moveDir = VAdd(moveDir, forwardXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_S))
                {
                    moveDir = VSub(moveDir, forwardXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_D))
                {
                    moveDir = VAdd(moveDir, rightXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_A))
                {
                    moveDir = VSub(moveDir, rightXZ);
                }

                // マウス左クリック長押しによる移動
                m_isMouseMoving = false;
                if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0)
                {
                    int mx = 0, my = 0;
                    GetMousePoint(&mx, &my);
                    m_mouseTargetX = mx;
                    m_mouseTargetY = my;

                    VECTOR catScreen = ConvWorldPosToScreenPos(m_pos);
                    // カメラ前方（描画範囲内）にあるかチェック
                    if (catScreen.z > 0.0f)
                    {
                        float dx = static_cast<float>(mx) - catScreen.x;
                        float dy = static_cast<float>(my) - catScreen.y;
                        float distSq = dx * dx + dy * dy;

                        // 猫の足元から15ピクセル以上離れていれば移動
                        if (distSq > 15.0f * 15.0f)
                        {
                            m_isMouseMoving = true;
                            float dist = std::sqrt(distSq);
                            float ndx = dx / dist;
                            float ndy = dy / dist;

                            // スクリーン空間: 上(-dy)はカメラ奥(forwardXZ)、右(+dx)はカメラ右(rightXZ)
                            VECTOR mouseMoveDir = VAdd(VScale(rightXZ, ndx), VScale(forwardXZ, -ndy));
                            mouseMoveDir.y = 0.0f;

                            float keyLenSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
                            if (keyLenSq < 0.0001f)
                            {
                                moveDir = mouseMoveDir;
                            }
                            
                            else
                            {
                                moveDir = VAdd(moveDir, mouseMoveDir);
                            }
                        }
                    }
                }

                float inputLengthSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
                if (inputLengthSq > 0.0001f)
                {
                    isMoving = true;
                    float inputLen = std::sqrt(inputLengthSq);
                    moveDir.x /= inputLen;
                    moveDir.z /= inputLen;

                    // 目標回転角に向かってスムーズに旋回
                    float targetAngle = std::atan2(moveDir.x, moveDir.z);
                    m_rotY = MathHelper::LerpAngle(m_rotY, targetAngle, 0.25f);

                    // 移動
                    m_pos.x += moveDir.x * m_speed;
                    m_pos.z += moveDir.z * m_speed;
                }

                // 筋トレ開始チェック
                if (isTriggerJustPressed)
                {
                    m_isSkillChecking = true;
                    m_scCursor = 0.0f;
                    m_wasDecayed = false;
                    m_scStoppedTimer = 0;

                    // 難易度を緩和（初期ゾーン幅を0.38f、高Repでも最低0.20fを保証）
                    float zoneWidth = 0.38f - static_cast<float>(m_repCount) * 0.02f;
                    if (zoneWidth < 0.20f)
                    {
                        zoneWidth = 0.20f;
                    }
                    
                    m_scZoneStart = 0.40f + static_cast<float>(rand() % 20) / 100.0f;
                    if (m_scZoneStart + zoneWidth > 0.95f)
                    {
                        m_scZoneStart = 0.95f - zoneWidth;
                    }
                    
                    m_scZoneEnd = m_scZoneStart + zoneWidth;
                }
            }
            
            else
            {
                // スキルチェックQTE実行中（針の移動とタイミング判定）
                if (m_scStoppedTimer > 0)
                {
                    // キーを押した瞬間に針をピタッと止めて確認できる演出（約0.3秒）
                    m_scStoppedTimer--;
                    if (m_scStoppedTimer <= 0)
                    {
                        m_isSkillChecking = false;
                        if (!m_lastResultSuccess)
                        {
                            m_state = MuscleState::Soreness;
                            m_sorenessTimer = 300; // 5秒
                            m_repCount = 0;
                            m_resultShowTimer = 90;
                        }
                    }
                }
                
                else
                {
                    // 基本速度は押しやすい0.014f、レベル（Rep）が上がるごとに約1.2倍ずつ速くなる
                    float speedMultiplier = std::pow(1.20f, static_cast<float>(m_repCount));

                    if (speedMultiplier > 2.8f)
                    {
                        speedMultiplier = 2.8f; // 上限リミット
                    }

                    float cursorSpeed = 0.014f * speedMultiplier;
                    m_scCursor += cursorSpeed;

                    // タイミングよくキーを離した瞬間にバーをピタッと止めて判定！
                    if (isTriggerJustReleased)
                    {
                        // キーを離した瞬間の座標で即座にピタッと止める
                        if (m_scCursor > 1.0f)
                        {
                            m_scCursor = 1.0f;
                        }

                        // タイミング判定
                        if (m_scCursor >= m_scZoneStart && m_scCursor <= m_scZoneEnd)
                        {
                            // 【成功】Rep追加 & 15秒タイマーリセット
                            m_repCount++;
                            m_pumpDecayTimer = PUMP_DECAY_FRAMES; // 15秒
                            m_lastResultSuccess = true;
                            m_resultShowTimer = 60;
                            m_scStoppedTimer = 18; // 約0.3秒間針を止めて成功位置を表示

                            // 筋トレ成功エフェクト再生（無効化中）
                            // EffectManager::GetInstance().PlayPumpSuccessEffect(m_pos, m_repCount);
                        }
                        
                        else
                        {
                            // 【失敗】針を止めて位置を確認させてから筋肉痛へ
                            m_lastResultSuccess = false;
                            m_resultShowTimer = 90;
                            m_scStoppedTimer = 22; // 約0.36秒間針を止めて失敗位置を表示
                        }
                    }
                    
                    else if (m_scCursor >= 1.0f)
                    {
                        // 【タイムアウト見逃し失敗】
                        m_scCursor = 1.0f;
                        m_lastResultSuccess = false;
                        m_resultShowTimer = 90;
                        m_scStoppedTimer = 18;
                    }
                }
            }
        }
    }

    // Effekseer マッスルオーラの更新（QTE筋トレ中またはパンプアップ状態）
    bool auraActive = ((m_isSkillChecking || (m_state == MuscleState::Muscular && m_repCount > 0)) &&
                       m_state != MuscleState::Soreness && m_stunTimer <= 0);

    EffectManager::GetInstance().UpdateMuscleAura(m_pos, auraActive, m_repCount);

    // 3Dモデルのアニメーション更新
    UpdateAnimation(isMoving);
}

void Player3D::DrawStunEffect()
{
    if (!IsStunned())
    {
        return;
    }

    // 猫の頭上で回転する星・気絶マーク（大きな星と光輪）
    float currentScale = (m_modelHandle != -1) ? (ModelConfig::CAT_MODEL_SCALE + static_cast<float>(GetEffectiveRep()) * 0.0025f) : 0.06f;
    float headY = (m_modelHandle != -1) ? (m_pos.y + 435.4f * currentScale + 8.0f) : (m_pos.y + 34.0f);
    float spinSpeed = m_animTime * 12.0f;
    float ringRadius = (m_modelHandle != -1) ? (20.0f * (currentScale / 0.12f)) : 16.0f;

    // くるくる回る4つの星
    for (int i = 0; i < 4; ++i)
    {
        float angle = spinSpeed + static_cast<float>(i) * (MathHelper::PI * 0.5f);
        float starX = m_pos.x + std::cos(angle) * ringRadius;
        float starZ = m_pos.z + std::sin(angle) * ringRadius;
        float starY = headY + std::sin(angle * 2.5f) * 3.5f;

        VECTOR starPos = VGet(starX, starY, starZ);
        DrawSphere3D(starPos, 3.5f, 8, GetColor(255, 220, 50), GetColor(255, 240, 120), TRUE);
    }

    // 気絶リング
    int ringSegments = 20;
    for (int i = 0; i < ringSegments; ++i)
    {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / ringSegments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / ringSegments);
        VECTOR p1 = VGet(m_pos.x + std::cos(a1) * ringRadius, headY, m_pos.z + std::sin(a1) * ringRadius);
        VECTOR p2 = VGet(m_pos.x + std::cos(a2) * ringRadius, headY, m_pos.z + std::sin(a2) * ringRadius);
        DrawLine3D(p1, p2, GetColor(255, 230, 80));
    }
}

void Player3D::Draw3D()
{
    // マッチョ度（Rep数）に応じたモデル拡大率の計算（レベル15上限まで徐々にマッチョ化）
    float effectiveRep = static_cast<float>(GetEffectiveRep());
    float scale = ModelConfig::CAT_MODEL_SCALE + effectiveRep * 0.0025f;

    // 足元接地高さ補正（モデル原点が足元にあるためオフセット0で地面接地）
    float offsetY = 0.0f;

    // 1. 飛びつき・タックル残像の描画
    for (const auto& trail : m_trails)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, trail.alpha);

        if (m_modelHandle != -1)
        {
            int effectiveTrailRep = (trail.repCount > MAX_EFFECTIVE_REP) ? MAX_EFFECTIVE_REP : trail.repCount;
            float trailScale = ModelConfig::CAT_MODEL_SCALE + static_cast<float>(effectiveTrailRep) * 0.0025f;
            float trailOffsetY = 0.0f;
            VECTOR trailPos = VGet(trail.pos.x, trail.pos.y + trailOffsetY, trail.pos.z);

            MV1SetPosition(m_modelHandle, trailPos);
            MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, trail.rotY + ModelConfig::CAT_MODEL_ROT_Y, 0.0f));
            MV1SetScale(m_modelHandle, VGet(trailScale, trailScale, trailScale));
            MV1DrawModel(m_modelHandle);
        }
        
        else
        {
            ModelManager::GetInstance().DrawFallbackCat(trail.pos, trail.rotY, trail.repCount, false, true, m_animTime);
        }

        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 2. プレイヤー本体の描画（3Dアニメーションモデルまたはフォールバック）
    bool isSoreness = (m_state == MuscleState::Soreness);

    if (m_modelHandle != -1)
    {
        VECTOR drawPos = VGet(m_pos.x, m_pos.y + offsetY, m_pos.z);

        if (isSoreness)
        {
            // 筋肉痛時：少しだけ青ざめた青色にティント（青スケール強調＋ほのかな青エミッシブ）
            MV1SetDifColorScale(m_modelHandle, GetColorF(0.55f, 0.65f, 1.30f, 1.0f));
            MV1SetAmbColorScale(m_modelHandle, GetColorF(0.50f, 0.60f, 1.35f, 1.0f));
            MV1SetEmiColorScale(m_modelHandle, GetColorF(0.08f, 0.12f, 0.35f, 1.0f));
        }

        MV1SetPosition(m_modelHandle, drawPos);
        MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, m_rotY + ModelConfig::CAT_MODEL_ROT_Y, 0.0f));
        MV1SetScale(m_modelHandle, VGet(scale, scale, scale));
        MV1DrawModel(m_modelHandle);

        if (isSoreness)
        {
            // 通常カラーに復帰
            MV1SetDifColorScale(m_modelHandle, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
            MV1SetAmbColorScale(m_modelHandle, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
            MV1SetEmiColorScale(m_modelHandle, GetColorF(0.0f, 0.0f, 0.0f, 1.0f));
        }
    }
    
    else
    {
        ModelManager::GetInstance().DrawFallbackCat
        (
            m_pos, m_rotY, m_repCount, isSoreness, (m_isPouncing || m_isTackling), m_animTime
        );
    }

    // スクワット中（筋トレQTE中またはスクワットアニメーション中）に両手でバーベルを保持・描画
    if (m_isSkillChecking || m_currentAnimIndex == m_animIndexSquat)
    {
        DrawBarbell(scale);
    }

    // 3. バカゲー風コミカル・マッスル湯気＆マッチョオーラ（3D）
    if (m_state == MuscleState::Muscular && m_repCount > 0)
    {
        // コミカルな白い湯気（猫の表情や筋肉が見えるよう薄く控えめに立ち昇る）
        float shoulderBaseY = (m_modelHandle != -1) ? (m_pos.y + 310.0f * scale) : (m_pos.y + 20.0f);
        float puffRangeY = 24.0f * (scale / 0.12f);
        float puffRadiusBase = 2.2f * (scale / 0.12f);

        for (int i = 0; i < 3; ++i)
        {
            float phase = m_animTime * 5.0f + static_cast<float>(i) * 2.09f;
            float puffY = shoulderBaseY + std::fmod(phase * 10.0f, puffRangeY);
            float offsetX = std::sin(phase * 2.0f) * (9.0f * (scale / 0.12f));
            float offsetZ = std::cos(phase * 2.0f) * (9.0f * (scale / 0.12f));
            float puffRadius = puffRadiusBase + std::fmod(phase * 2.5f, 3.5f);
            int puffAlpha = static_cast<int>(50.0f * (1.0f - (puffY - shoulderBaseY) / puffRangeY));

            if (puffAlpha > 0)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, puffAlpha);
                DrawSphere3D(VGet(m_pos.x + offsetX, puffY, m_pos.z + offsetZ), puffRadius, 8,
                             GetColor(255, 255, 255), GetColor(240, 240, 255), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }

        // Lv15以上：バカゲー神マッスル黄金オーラ（猫本体を遮らない足元リング＆上品なスパーク）
        if (m_repCount >= MAX_EFFECTIVE_REP)
        {
            auto drawRingXZ = [](const VECTOR& center, float radius, unsigned int color, int segments = 16)
            {
                for (int s = 0; s < segments; ++s)
                {
                    float a1 = static_cast<float>(s) * (2.0f * MathHelper::PI / static_cast<float>(segments));
                    float a2 = static_cast<float>(s + 1) * (2.0f * MathHelper::PI / static_cast<float>(segments));
                    VECTOR p1 = VGet(center.x + std::cos(a1) * radius, center.y, center.z + std::sin(a1) * radius);
                    VECTOR p2 = VGet(center.x + std::cos(a2) * radius, center.y, center.z + std::sin(a2) * radius);
                    DrawLine3D(p1, p2, color);
                }
            };

            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
            float auraR = 24.0f * (scale / 0.12f);
            drawRingXZ(VGet(m_pos.x, m_pos.y + 1.0f, m_pos.z), auraR, GetColor(255, 215, 0));
            drawRingXZ(VGet(m_pos.x, m_pos.y + 1.0f, m_pos.z), auraR * 0.7f, GetColor(255, 235, 100));
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // 四方に小さく煌めく黄金スパーク星
            for (int k = 0; k < 4; ++k)
            {
                float aAngle = m_animTime * 6.0f + static_cast<float>(k) * (MathHelper::PI / 2.0f);
                float spkX = m_pos.x + std::cos(aAngle) * (auraR * 0.9f);
                float spkZ = m_pos.z + std::sin(aAngle) * (auraR * 0.9f);
                float spkY = m_pos.y + 6.0f + std::sin(aAngle * 2.5f) * 5.0f;
                DrawSphere3D(VGet(spkX, spkY, spkZ), 1.8f, 6, GetColor(255, 240, 50), GetColor(255, 255, 180), TRUE);
            }
        }
    }

    // 4. タックル突進オーラエフェクト（猫の体・Push動作を隠さないよう足元推進リングとして描画）
    if (m_isTackling)
    {
        auto drawRingXZ = [](const VECTOR& center, float radius, unsigned int color, int segments = 16)
        {
            for (int s = 0; s < segments; ++s)
            {
                float a1 = static_cast<float>(s) * (2.0f * MathHelper::PI / static_cast<float>(segments));
                float a2 = static_cast<float>(s + 1) * (2.0f * MathHelper::PI / static_cast<float>(segments));
                VECTOR p1 = VGet(center.x + std::cos(a1) * radius, center.y, center.z + std::sin(a1) * radius);
                VECTOR p2 = VGet(center.x + std::cos(a2) * radius, center.y, center.z + std::sin(a2) * radius);
                DrawLine3D(p1, p2, color);
            }
        };

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
        float r = GetTackleRadius() * 0.65f;
        drawRingXZ(VGet(m_pos.x, m_pos.y + 1.0f, m_pos.z), r, GetColor(255, 130, 20));
        drawRingXZ(VGet(m_pos.x, m_pos.y + 1.0f, m_pos.z), r * 0.5f, GetColor(255, 190, 40));
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 5. 猫のスタンエフェクト
    DrawStunEffect();
}

void Player3D::Draw2D()
{
    const auto& fm = FontManager::GetInstance();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    // スキルチェックQTEバー（画面中央下部に表示）
    if (m_isSkillChecking)
    {
        int barW = 340;
        int barH = 30;
        int barX = (Config::SCREEN_WIDTH - barW) / 2;
        int barY = Config::SCREEN_HEIGHT - 125;

        // ポップな極太枠付きバー背景（バカゲー風）
        DrawBox(barX - 6, barY - 6, barX + barW + 6, barY + barH + 6, GetColor(0, 0, 0), TRUE);
        DrawBox(barX - 3, barY - 3, barX + barW + 3, barY + barH + 3, GetColor(255, 220, 50), TRUE);
        DrawBox(barX, barY, barX + barW, barY + barH, GetColor(40, 40, 50), TRUE);

        // 成功ゾーン（ビビッドグリーン）
        int zoneX1 = barX + static_cast<int>(m_scZoneStart * barW);
        int zoneX2 = barX + static_cast<int>(m_scZoneEnd * barW);
        DrawBox(zoneX1, barY, zoneX2, barY + barH, GetColor(40, 240, 100), TRUE);
        DrawBox(zoneX1, barY, zoneX2, barY + barH, GetColor(255, 255, 255), FALSE);

        // 針（通常時はビビッドレッド、停止確定時は判定結果色）
        int cursorX = barX + static_cast<int>(m_scCursor * barW);
        unsigned int needleColor = GetColor(255, 40, 40);
        if (m_scStoppedTimer > 0)
        {
            needleColor = m_lastResultSuccess ? GetColor(255, 255, 50) : GetColor(255, 30, 30);
        }
        DrawBox(cursorX - 5, barY - 10, cursorX + 5, barY + barH + 10, GetColor(0, 0, 0), TRUE);
        DrawBox(cursorX - 3, barY - 8, cursorX + 3, barY + barH + 8, needleColor, TRUE);

        // ガイドテキスト
        if (m_scStoppedTimer > 0)
        {
            if (m_lastResultSuccess)
            {
                DrawStringToHandle(barX + 90, barY - 32, "★ NICE PUMP!! ★", GetColor(255, 255, 50), font24);
            }
            
            else
            {
                DrawStringToHandle(barX + 115, barY - 32, "× MISS! ×", GetColor(255, 60, 60), font24);
            }
        }
        
        else
        {
            DrawStringToHandle(barX + 35, barY - 30, "緑のゾーンで [SPACE] を離せ！", GetColor(255, 255, 80), font18);
        }
    }

    // スキルチェック結果・筋肉痛・減衰通知テキスト（バカゲー風ポップ装飾）
    if (m_resultShowTimer > 0)
    {
        int alpha = (m_resultShowTimer > 20) ? 255 : (m_resultShowTimer * 255 / 20);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        if (m_wasDecayed)
        {
            DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 140, Config::SCREEN_HEIGHT / 2 - 45,
                               "15秒放置: 筋肉が減衰した！ (0 Rep)", GetColor(180, 200, 255), font18);
        }
        
        else if (m_state == MuscleState::Soreness)
        {
            DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 160, Config::SCREEN_HEIGHT / 2 - 45,
                               "FAIL! 筋肉痛で5秒間動けない！", GetColor(255, 70, 70), font24);
        }
        
        else if (m_lastResultSuccess)
        {
            if (m_repCount >= MAX_EFFECTIVE_REP)
            {
                DrawFormatStringToHandle(Config::SCREEN_WIDTH / 2 - 180, Config::SCREEN_HEIGHT / 2 - 45,
                                         GetColor(255, 215, 0), font24,
                                         "★ GOD MUSCLE MAX!! ★ +1 REP (Lv.%d)", m_repCount);
            }
            
            else
            {
                DrawFormatStringToHandle(Config::SCREEN_WIDTH / 2 - 120, Config::SCREEN_HEIGHT / 2 - 45,
                                         GetColor(255, 230, 40), font24,
                                         "PUMP UP!! +1 REP (Lv.%d)", m_repCount);
            }
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 猫の激突スタン通知テキスト
    if (m_stunTimer > 0)
    {
        int alpha = (m_stunTimer % 10 < 5) ? 255 : 180;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawFormatStringToHandle(Config::SCREEN_WIDTH / 2 - 140, Config::SCREEN_HEIGHT / 2 - 70, GetColor(255, 230, 40), font24, "★ STUNNED!! 残り%.1fs ★", GetCatStunRemainingSeconds());
        DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 120, Config::SCREEN_HEIGHT / 2 - 40, "壁・家具に激突して気絶中！", GetColor(255, 240, 120), font16);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // マウス左クリック長押し移動時の方向ガイドライン＆マーカー
    if (m_isMouseMoving && m_state != MuscleState::Soreness && m_stunTimer <= 0)
    {
        VECTOR catScreen = ConvWorldPosToScreenPos(m_pos);
        if (catScreen.z > 0.0f)
        {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 160);
            // 猫の足元からマウスカーソルへの方向ガイドライン
            DrawLine(static_cast<int>(catScreen.x), static_cast<int>(catScreen.y),
                     m_mouseTargetX, m_mouseTargetY, GetColor(100, 220, 255), 2);
            // マウスカーソル位置のガイドターゲット円
            DrawCircle(m_mouseTargetX, m_mouseTargetY, 12, GetColor(100, 220, 255), FALSE);
            DrawCircle(m_mouseTargetX, m_mouseTargetY, 4, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
    }
}

void Player3D::AddRep(int amount)
{
    m_repCount += amount;
    m_pumpDecayTimer = PUMP_DECAY_FRAMES; // 15秒リセット
    m_state = MuscleState::Muscular;
    m_speed = SPEED_INITIAL + static_cast<float>(GetEffectiveRep()) * 0.15f;
    m_lastResultSuccess = true;
    m_resultShowTimer = 60;

    // 筋トレ成功エフェクト再生（無効化中）
    // EffectManager::GetInstance().PlayPumpSuccessEffect(m_pos, m_repCount);
}

void Player3D::DrawBarbell(float catScale)
{
    // 3Dモデル版バーベル描画
    if (m_barbellModelHandle != -1 && m_modelHandle != -1 && m_lHandFrame != -1 && m_rHandFrame != -1)
    {
        // 1. 猫の両手フレーム（手首および指付け根）の現在のワールド座標を取得
        VECTOR posL = MV1GetFramePosition(m_modelHandle, m_lHandFrame);
        VECTOR posR = MV1GetFramePosition(m_modelHandle, m_rHandFrame);

        // 指の付け根位置（手首と指付け根の中間を手のひら・握り位置とする）
        VECTOR handCenterL = posL;
        VECTOR handCenterR = posR;

        if (m_lHandKnuckleFrame != -1 && m_rHandKnuckleFrame != -1)
        {
            VECTOR posLKnuckle = MV1GetFramePosition(m_modelHandle, m_lHandKnuckleFrame);
            VECTOR posRKnuckle = MV1GetFramePosition(m_modelHandle, m_rHandKnuckleFrame);
            handCenterL = VAdd(posL, VScale(VSub(posLKnuckle, posL), 0.5f));
            handCenterR = VAdd(posR, VScale(VSub(posRKnuckle, posR), 0.5f));
        }

        // 2. シャフト方向（左手から右手へ向かうベクトル）
        VECTOR dir = VSub(handCenterR, handCenterL);
        float dist = VSize(dir);

        if (dist > 0.001f)
        {
            // バーベルのZ軸（長軸・シャフト方向）：左手から右手へ
            VECTOR basisZ = VNorm(dir);

            // 上方向の基準ベクトル
            VECTOR upRef = VGet(0.0f, 1.0f, 0.0f);

            // バーベルのX軸（水平直交）：Up × Z
            VECTOR basisX = VCross(upRef, basisZ);
            float lenX = VSize(basisX);
            if (lenX < 0.001f)
            {
                basisX = VGet(1.0f, 0.0f, 0.0f);
            }
            else
            {
                basisX = VNorm(basisX);
            }

            // バーベルのY軸（垂直直交）：Z × X
            VECTOR basisY = VNorm(VCross(basisZ, basisX));

            // 3. 両手の中央位置（手のひらの中間）
            VECTOR midPos = VScale(VAdd(handCenterL, handCenterR), 0.5f);

            // 4. バーベルのスケール
            // 基本両手距離 69.13f に対する比率でバーベルのスケールを自動調整
            float barbellScale = (dist / 69.13f) * ModelConfig::BARBELL_BASE_SCALE;

            // 各軸にスケールを適用
            VECTOR sx = VScale(basisX, barbellScale);
            VECTOR sy = VScale(basisY, barbellScale);
            VECTOR sz = VScale(basisZ, barbellScale);

            // 5. ワールド変換行列の構築 (行優先: Direct3D形式)
            MATRIX mat = {};
            mat.m[0][0] = sx.x; mat.m[0][1] = sx.y; mat.m[0][2] = sx.z; mat.m[0][3] = 0.0f;
            mat.m[1][0] = sy.x; mat.m[1][1] = sy.y; mat.m[1][2] = sy.z; mat.m[1][3] = 0.0f;
            mat.m[2][0] = sz.x; mat.m[2][1] = sz.y; mat.m[2][2] = sz.z; mat.m[2][3] = 0.0f;
            mat.m[3][0] = midPos.x; mat.m[3][1] = midPos.y; mat.m[3][2] = midPos.z; mat.m[3][3] = 1.0f;

            // 行列を設定してバーベルを描画
            MV1SetMatrix(m_barbellModelHandle, mat);
            MV1DrawModel(m_barbellModelHandle);
        }
    }
    else
    {
        // フォールバック描画（モデル未読み込み時）
        float forwardX = std::sin(m_rotY);
        float forwardZ = std::cos(m_rotY);
        float rightX = forwardZ;
        float rightZ = -forwardX;

        float barbellY = m_pos.y + 24.0f * (catScale / 0.12f);
        float centerX = m_pos.x + forwardX * 18.0f * (catScale / 0.12f);
        float centerZ = m_pos.z + forwardZ * 18.0f * (catScale / 0.12f);

        float halfWidth = 36.0f * (catScale / 0.12f);
        VECTOR p1 = VGet(centerX - rightX * halfWidth, barbellY, centerZ - rightZ * halfWidth);
        VECTOR p2 = VGet(centerX + rightX * halfWidth, barbellY, centerZ + rightZ * halfWidth);

        // シャフト（銀色）
        DrawCapsule3D(p1, p2, 2.0f * (catScale / 0.12f), 8, GetColor(200, 205, 215), GetColor(220, 225, 235), TRUE);

        // 両端のウェイトプレート（黒/濃紺）
        DrawSphere3D(p1, 8.0f * (catScale / 0.12f), 8, GetColor(40, 45, 50), GetColor(60, 65, 75), TRUE);
        DrawSphere3D(p2, 8.0f * (catScale / 0.12f), 8, GetColor(40, 45, 50), GetColor(60, 65, 75), TRUE);
    }
}