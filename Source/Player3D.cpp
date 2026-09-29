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
            // 初期状態はアイドルアニメーション（Armature|04_Idol: インデックス 3）
            m_currentAnimIndex = 3;
            m_attachAnimIndex = MV1AttachAnim(m_modelHandle, m_currentAnimIndex, -1, FALSE);
            m_animPlayTime = 0.0f;

            // バーベル配置用の両手ボーンフレームを検索・キャッシュ
            m_lHandFrame = MV1SearchFrame(m_modelHandle, "mixamorig:LeftHand");
            m_rHandFrame = MV1SearchFrame(m_modelHandle, "mixamorig:RightHand");
            m_lHandKnuckleFrame = MV1SearchFrame(m_modelHandle, "mixamorig:LeftHandMiddle1");
            m_rHandKnuckleFrame = MV1SearchFrame(m_modelHandle, "mixamorig:RightHandMiddle1");
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
    // 優先度1: 攻撃（飛びつき・タックル: インデックス 1）
    // 優先度2: スクワット（筋トレQTE中: インデックス 2）
    // 優先度3: 走り（移動中: インデックス 0）
    // 優先度4: 待機（アイドル: インデックス 3）
    int targetAnim = 3; // デフォルト: Idol
    float playSpeed = 0.5f;

    if (m_isPouncing || m_isTackling)
    {
        targetAnim = 1; // Armature|02_Attack
        playSpeed = 1.0f;
    }
    
    else if (m_isSkillChecking)
    {
        targetAnim = 2; // Armature|03_squat
        playSpeed = 1.0f;
    }
    
    else if (isMoving)
    {
        targetAnim = 0; // Armature|01_Run
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

void Player3D::UpdateWithCamera(const Camera3D& camera)
{
    m_animFrame++;
    m_animTime += 1.0f / 60.0f;

    bool isMoving = false;

    // キー入力受付（筋トレ: SPACE/Z、飛びつき: SHIFT/X/C、タックル: E）
    bool currentTriggerKey = (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_Z));
    bool isTriggerJustPressed = currentTriggerKey && !m_prevTriggerKey;
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

    // 残像トレイルのフェードアウト処理
    for (auto it = m_trails.begin(); it != m_trails.end(); )
    {
        it->alpha -= 25;
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
        return;
    }

    // タックル（Tackle）アクション実行中処理
    if (m_isTackling)
    {
        m_tackleTimer--;

        // 残像記録
        m_trails.push_back({ m_pos, m_rotY, m_repCount, 220 });

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

        // 3D残像記録
        m_trails.push_back({ m_pos, m_rotY, m_repCount, 180 });

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

                    return;
                }

                // 飛びつき発動チェック（4 Rep以上特権）
                if (CanPounce() && isPounceJustPressed)
                {
                    m_isPouncing = true;
                    m_pounceTimer = GetPounceDuration();
                    m_pounceDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
                    return;
                }

                // 移動入力処理 (WASD / 矢印キー / マウス左クリック長押し) - カメラのXZ平面基準
                VECTOR forwardXZ = camera.GetForwardXZ();
                VECTOR rightXZ   = camera.GetRightXZ();
                VECTOR moveDir   = VGet(0.0f, 0.0f, 0.0f);

                if (CheckHitKey(KEY_INPUT_W) || CheckHitKey(KEY_INPUT_UP))
                {
                    moveDir = VAdd(moveDir, forwardXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_S) || CheckHitKey(KEY_INPUT_DOWN))
                {
                    moveDir = VSub(moveDir, forwardXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_D) || CheckHitKey(KEY_INPUT_RIGHT))
                {
                    moveDir = VAdd(moveDir, rightXZ);
                }
                
                if (CheckHitKey(KEY_INPUT_A) || CheckHitKey(KEY_INPUT_LEFT))
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

                    if (isTriggerJustPressed)
                    {
                        // キーを押した瞬間の座標で即座にピタッと止める
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

                            // Effekseer 筋トレ成功エフェクト再生（モデル周囲の発光）
                            EffectManager::GetInstance().PlayPumpSuccessEffect(m_pos, m_repCount);
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

        MV1SetPosition(m_modelHandle, drawPos);
        MV1SetRotationXYZ(m_modelHandle, VGet(0.0f, m_rotY + ModelConfig::CAT_MODEL_ROT_Y, 0.0f));
        MV1SetScale(m_modelHandle, VGet(scale, scale, scale));
        MV1DrawModel(m_modelHandle);
    }
    
    else
    {
        ModelManager::GetInstance().DrawFallbackCat
        (
            m_pos, m_rotY, m_repCount, isSoreness, (m_isPouncing || m_isTackling), m_animTime
        );
    }

    // スクワット中（筋トレQTE中またはスクワットアニメーション中）に両手でバーベルを保持・描画
    if (m_isSkillChecking || m_currentAnimIndex == 2)
    {
        DrawBarbell(scale);
    }

    // 3. バカゲー風コミカル・マッスル湯気＆マッチョオーラ（3D）
    if (m_state == MuscleState::Muscular && m_repCount > 0)
    {
        // コミカルな白い湯気（肩や背中からポフポフ立ち昇る）
        float shoulderBaseY = (m_modelHandle != -1) ? (m_pos.y + 310.0f * scale) : (m_pos.y + 20.0f);
        float puffRangeY = 30.0f * (scale / 0.12f);
        float puffRadiusBase = 4.0f * (scale / 0.12f);

        for (int i = 0; i < 4; ++i)
        {
            float phase = m_animTime * 6.0f + static_cast<float>(i) * 1.57f;
            float puffY = shoulderBaseY + std::fmod(phase * 12.0f, puffRangeY);
            float offsetX = std::sin(phase * 2.0f) * (12.0f * (scale / 0.12f));
            float offsetZ = std::cos(phase * 2.0f) * (12.0f * (scale / 0.12f));
            float puffRadius = puffRadiusBase + std::fmod(phase * 4.0f, 6.0f);
            int puffAlpha = static_cast<int>(180.0f * (1.0f - (puffY - shoulderBaseY) / puffRangeY));

            if (puffAlpha > 0)
            {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, puffAlpha);
                DrawSphere3D(VGet(m_pos.x + offsetX, puffY, m_pos.z + offsetZ), puffRadius, 8,
                             GetColor(255, 255, 255), GetColor(240, 240, 255), TRUE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
        }

        // Lv15以上：バカゲー神マッスル黄金オーラ（激しいスパークリング）
        if (m_repCount >= MAX_EFFECTIVE_REP)
        {
            float auraCenterY = (m_modelHandle != -1) ? (m_pos.y + 200.0f * scale) : (m_pos.y + 16.0f);
            float auraR = (36.0f * (scale / 0.12f)) + std::sin(m_animTime * 15.0f) * 4.0f;
            DrawSphere3D(VGet(m_pos.x, auraCenterY, m_pos.z), auraR, 10,
                         GetColor(255, 215, 0), GetColor(255, 255, 100), FALSE);

            // 四方に飛び散る黄金スパーク星
            for (int k = 0; k < 6; ++k)
            {
                float aAngle = m_animTime * 8.0f + static_cast<float>(k) * (MathHelper::PI / 3.0f);
                float spkX = m_pos.x + std::cos(aAngle) * (auraR + 4.0f);
                float spkZ = m_pos.z + std::sin(aAngle) * (auraR + 4.0f);
                float spkY = m_pos.y + 12.0f + std::sin(aAngle * 3.0f) * 8.0f;
                DrawSphere3D(VGet(spkX, spkY, spkZ), 2.5f, 6, GetColor(255, 240, 50), GetColor(255, 255, 180), TRUE);
            }
        }
    }

    // 4. タックル突進オーラエフェクト
    if (m_isTackling)
    {
        float r = GetTackleRadius();
        DrawSphere3D(VGet(m_pos.x, m_pos.y + 15.0f, m_pos.z), r, 12, GetColor(255, 120, 20), GetColor(255, 220, 50), FALSE);
        DrawSphere3D(VGet(m_pos.x, m_pos.y + 15.0f, m_pos.z), r * 0.7f, 10, GetColor(255, 180, 40), GetColor(255, 240, 100), FALSE);
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
            DrawStringToHandle(barX + 45, barY - 30, "緑のゾーンで [SPACE] をキメろ！", GetColor(255, 255, 80), font18);
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

    // 筋トレ成功エフェクト再生（モデル周囲の発光）
    EffectManager::GetInstance().PlayPumpSuccessEffect(m_pos, m_repCount);
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