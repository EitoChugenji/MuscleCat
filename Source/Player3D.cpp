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
}

void Player3D::Update()
{
    // 通常のUpdate（後方互換）
}

void Player3D::UpdateWithCamera(const Camera3D& camera)
{
    m_animFrame++;
    m_animTime += 1.0f / 60.0f;

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

                            // Effekseer 筋トレ成功パンプアップ光柱エフェクト再生
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
}

void Player3D::DrawStunEffect()
{
    if (!IsStunned())
    {
        return;
    }

    // 猫の頭上で回転する星・気絶マーク（大きな星と光輪）
    float headY = m_pos.y + 34.0f;
    float spinSpeed = m_animTime * 12.0f;
    float ringRadius = 16.0f;

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
    // 1. 飛びつき・タックル残像の描画
    for (const auto& trail : m_trails)
    {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, trail.alpha);
        ModelManager::GetInstance().DrawFallbackCat(trail.pos, trail.rotY, trail.repCount, false, true, m_animTime);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 2. プレイヤー本体の描画（3Dモデルまたはフォールバック）
    bool isSoreness = (m_state == MuscleState::Soreness);
    if (!ModelManager::GetInstance().DrawModelIfLoaded
        (
        ModelConfig::CAT_MODEL_PATH, m_pos, m_rotY + ModelConfig::CAT_MODEL_ROT_Y,
        ModelConfig::CAT_MODEL_SCALE
        ))
    {
        ModelManager::GetInstance().DrawFallbackCat
        (
            m_pos, m_rotY, m_repCount, isSoreness, (m_isPouncing || m_isTackling), m_animTime
        );
    }

    // 3. バカゲー風コミカル・マッスル湯気＆マッチョオーラ（3D）
    if (m_state == MuscleState::Muscular && m_repCount > 0)
    {
        // コミカルな白い湯気（肩や背中からポフポフ立ち昇る）
        for (int i = 0; i < 4; ++i)
        {
            float phase = m_animTime * 6.0f + static_cast<float>(i) * 1.57f;
            float puffY = m_pos.y + 20.0f + std::fmod(phase * 12.0f, 24.0f);
            float offsetX = std::sin(phase * 2.0f) * 10.0f;
            float offsetZ = std::cos(phase * 2.0f) * 10.0f;
            float puffRadius = 3.0f + std::fmod(phase * 4.0f, 6.0f);
            int puffAlpha = static_cast<int>(180.0f * (1.0f - (puffY - m_pos.y - 20.0f) / 24.0f));

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
            float auraR = 24.0f + std::sin(m_animTime * 15.0f) * 3.0f;
            DrawSphere3D(VGet(m_pos.x, m_pos.y + 16.0f, m_pos.z), auraR, 10,
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

    // 筋トレ成功パンプアップエフェクト再生
    EffectManager::GetInstance().PlayPumpSuccessEffect(m_pos, m_repCount);
}
