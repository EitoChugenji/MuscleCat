#include "Player3D.h"
#include "Camera3D.h"
#include "ModelConfig.h"
#include "ModelManager.h"
#include "FontManager.h"
#include "Config.h"
#include "DxLib.h"
#include <cmath>
#include <cstdlib>

Player3D::Player3D(const VECTOR& pos)
    : GameObject3D(pos, 18.0f, ObjectType::Player)
    , m_speed(SPEED_INITIAL)
    , m_state(MuscleState::Normal)
    , m_repCount(0)
    , m_pumpDecayTimer(0) {
    m_rotY = 0.0f;
}

void Player3D::Update() {
    // 通常のUpdate（後方互換）
}

void Player3D::UpdateWithCamera(const Camera3D& camera) {
    m_animFrame++;
    m_animTime += 1.0f / 60.0f;

    // ------------------------------------------------------------------------
    // キー入力受付（筋トレ: SPACE/Z、飛びつき: SHIFT/X/C、タックル: E）
    // ------------------------------------------------------------------------
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

    if (m_resultShowTimer > 0) {
        m_resultShowTimer--;
    }
    if (m_pounceCooldown > 0) {
        m_pounceCooldown--;
    }
    if (m_tackleCooldown > 0) {
        m_tackleCooldown--;
    }

    // 残像トレイルのフェードアウト処理
    for (auto it = m_trails.begin(); it != m_trails.end(); ) {
        it->alpha -= 25;
        if (it->alpha <= 0) {
            it = m_trails.erase(it);
        } else {
            ++it;
        }
    }

    // ========================================================================
    // スタン（壁・家具激突）処理
    // ========================================================================
    if (m_stunTimer > 0) {
        m_stunTimer--;
        m_isTackling = false;
        m_isPouncing = false;
        m_pos.y = 0.0f;
        return;
    }

    // ========================================================================
    // タックル（Tackle）アクション実行中処理
    // ========================================================================
    if (m_isTackling) {
        m_tackleTimer--;

        // 残像記録
        m_trails.push_back({ m_pos, m_rotY, m_repCount, 220 });

        // 地面を滑るような超高速直進
        float tSpeed = GetTackleSpeed();
        m_pos.x += m_tackleDir.x * tSpeed;
        m_pos.z += m_tackleDir.z * tSpeed;
        m_pos.y = 0.0f;

        if (m_tackleTimer <= 0) {
            m_isTackling = false;
            m_tackleCooldown = GetTackleCooldownMax();
        }
        return;
    }

    // ========================================================================
    // 飛びつき（Pounce）アクション実行中処理
    // ========================================================================
    if (m_isPouncing) {
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

        if (m_pounceTimer <= 0) {
            m_isPouncing = false;
            m_pos.y = 0.0f;
            m_pounceCooldown = GetPounceCooldownMax();
        }
    } else {
        m_pos.y = 0.0f;

        // ====================================================================
        // 通常 / 筋肉痛 / 筋トレ（Rep進行 & 10秒減衰）処理
        // ====================================================================
        if (m_state == MuscleState::Soreness) {
            // 【筋肉痛状態】5秒間完全移動不可（速度 0.0）
            m_speed = SPEED_SORENESS;
            m_isSkillChecking = false;
            m_pumpDecayTimer = 0;

            if (--m_sorenessTimer <= 0) {
                m_sorenessTimer = 0;
                m_state = MuscleState::Normal;
                m_repCount = 0;
                m_speed = SPEED_INITIAL;
            }
        } else {
            // ★【10秒間何もしなければ 0 Rep に戻る減衰処理】
            if (m_repCount > 0 && !m_isSkillChecking) {
                if (--m_pumpDecayTimer <= 0) {
                    m_pumpDecayTimer = 0;
                    m_repCount = 0;
                    m_speed = SPEED_INITIAL;
                    m_state = MuscleState::Normal;
                    m_resultShowTimer = 60;
                    m_wasDecayed = true;
                }
            }

            if (m_repCount > 0) {
                m_speed = SPEED_INITIAL + static_cast<float>(m_repCount) * 0.15f;
                m_state = MuscleState::Muscular;
            } else {
                m_speed = SPEED_INITIAL;
                m_state = MuscleState::Normal;
            }

            if (!m_isSkillChecking) {
                // ★【初期から使用可能】タックル発動チェック（Eキー）
                if (CanTackle() && isTackleJustPressed) {
                    m_isTackling = true;
                    m_tackleTimer = GetTackleDuration();
                    m_tackleDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
                    return;
                }

                // ★【4 Rep以上特権】飛びつき発動チェック
                if (CanPounce() && isPounceJustPressed) {
                    m_isPouncing = true;
                    m_pounceTimer = GetPounceDuration();

                    // 向いている方向を飛びつき突進方向に設定
                    m_pounceDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
                    return;
                }

                // 移動入力処理 (WASD / 矢印キー / マウス左クリック長押し) - カメラのXZ平面基準
                VECTOR forwardXZ = camera.GetForwardXZ();
                VECTOR rightXZ   = camera.GetRightXZ();
                VECTOR moveDir   = VGet(0.0f, 0.0f, 0.0f);

                if (CheckHitKey(KEY_INPUT_W) || CheckHitKey(KEY_INPUT_UP)) {
                    moveDir = VAdd(moveDir, forwardXZ);
                }
                if (CheckHitKey(KEY_INPUT_S) || CheckHitKey(KEY_INPUT_DOWN)) {
                    moveDir = VSub(moveDir, forwardXZ);
                }
                if (CheckHitKey(KEY_INPUT_D) || CheckHitKey(KEY_INPUT_RIGHT)) {
                    moveDir = VAdd(moveDir, rightXZ);
                }
                if (CheckHitKey(KEY_INPUT_A) || CheckHitKey(KEY_INPUT_LEFT)) {
                    moveDir = VSub(moveDir, rightXZ);
                }

                // ★ マウス左クリック長押しによる移動
                // 猫の画面上の位置から見てマウスカーソルがある方向に進む
                m_isMouseMoving = false;
                if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
                    int mx = 0, my = 0;
                    GetMousePoint(&mx, &my);
                    m_mouseTargetX = mx;
                    m_mouseTargetY = my;

                    VECTOR catScreen = ConvWorldPosToScreenPos(m_pos);
                    // カメラ前方（描画範囲内）にあるかチェック
                    if (catScreen.z > 0.0f) {
                        float dx = static_cast<float>(mx) - catScreen.x;
                        float dy = static_cast<float>(my) - catScreen.y;
                        float distSq = dx * dx + dy * dy;

                        // 猫の足元から15ピクセル以上離れていれば移動（真上近辺での小刻みな揺れを防止）
                        if (distSq > 15.0f * 15.0f) {
                            m_isMouseMoving = true;
                            float dist = std::sqrt(distSq);
                            float ndx = dx / dist;
                            float ndy = dy / dist;

                            // スクリーン空間: 上(-dy)はカメラ奥(forwardXZ)、右(+dx)はカメラ右(rightXZ)
                            VECTOR mouseMoveDir = VAdd(VScale(rightXZ, ndx), VScale(forwardXZ, -ndy));
                            mouseMoveDir.y = 0.0f;

                            float keyLenSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
                            if (keyLenSq < 0.0001f) {
                                moveDir = mouseMoveDir;
                            } else {
                                moveDir = VAdd(moveDir, mouseMoveDir);
                            }
                        }
                    }
                }

                float inputLengthSq = moveDir.x * moveDir.x + moveDir.z * moveDir.z;
                if (inputLengthSq > 0.0001f) {
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
                if (isTriggerJustPressed) {
                    m_isSkillChecking = true;
                    m_scCursor = 0.0f;
                    m_wasDecayed = false;
                    m_scStoppedTimer = 0;

                    // 難易度を緩和（初期ゾーン幅を0.38fと大幅に広くし、高Repでも最低0.20fを保証）
                    float zoneWidth = 0.38f - static_cast<float>(m_repCount) * 0.02f;
                    if (zoneWidth < 0.20f) zoneWidth = 0.20f;
                    m_scZoneStart = 0.40f + static_cast<float>(rand() % 20) / 100.0f;
                    if (m_scZoneStart + zoneWidth > 0.95f) {
                        m_scZoneStart = 0.95f - zoneWidth;
                    }
                    m_scZoneEnd = m_scZoneStart + zoneWidth;
                }
            } else {
                // ============================================================
                // スキルチェックQTE実行中（針の移動とタイミング判定）
                // ============================================================
                if (m_scStoppedTimer > 0) {
                    // ★ キーを押した瞬間に針をピタッと止めて確認できる演出（約0.3秒）
                    m_scStoppedTimer--;
                    if (m_scStoppedTimer <= 0) {
                        m_isSkillChecking = false;
                        if (!m_lastResultSuccess) {
                            m_state = MuscleState::Soreness;
                            m_sorenessTimer = 300; // 5秒
                            m_repCount = 0;
                            m_resultShowTimer = 90;
                        }
                    }
                } else {
                    // 基本速度は押しやすい0.014f、レベル（Rep）が上がるごとに約1.2倍ずつ速くなる
                    float speedMultiplier = std::pow(1.20f, static_cast<float>(m_repCount));
                    if (speedMultiplier > 2.8f) speedMultiplier = 2.8f; // 上限リミット
                    float cursorSpeed = 0.014f * speedMultiplier;
                    m_scCursor += cursorSpeed;

                    if (isTriggerJustPressed) {
                        // ★ キーを押した瞬間の座標で即座にピタッと止める
                        if (m_scCursor > 1.0f) m_scCursor = 1.0f;

                        // タイミング判定
                        if (m_scCursor >= m_scZoneStart && m_scCursor <= m_scZoneEnd) {
                            // 【成功】Rep追加 & 10秒タイマーリセット
                            m_repCount++;
                            m_pumpDecayTimer = 600; // 10秒
                            m_lastResultSuccess = true;
                            m_resultShowTimer = 60;
                            m_scStoppedTimer = 18; // 約0.3秒間針を止めて成功位置を表示
                        } else {
                            // 【失敗】針を止めて位置を確認させてから筋肉痛へ
                            m_lastResultSuccess = false;
                            m_resultShowTimer = 90;
                            m_scStoppedTimer = 22; // 約0.36秒間針を止めて失敗位置を表示
                        }
                    } else if (m_scCursor >= 1.0f) {
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
}

void Player3D::DrawStunEffect() {
    if (!IsStunned()) return;

    // 猫の頭上で回転する星・気絶マーク（大きな星と光輪）
    float headY = m_pos.y + 34.0f;
    float spinSpeed = m_animTime * 12.0f;
    float ringRadius = 16.0f;

    // くるくる回る4つの星
    for (int i = 0; i < 4; ++i) {
        float angle = spinSpeed + static_cast<float>(i) * (MathHelper::PI * 0.5f);
        float starX = m_pos.x + std::cos(angle) * ringRadius;
        float starZ = m_pos.z + std::sin(angle) * ringRadius;
        float starY = headY + std::sin(angle * 2.5f) * 3.5f;

        VECTOR starPos = VGet(starX, starY, starZ);
        DrawSphere3D(starPos, 3.5f, 8, GetColor(255, 220, 50), GetColor(255, 240, 120), TRUE);
    }

    // 気絶リング
    int ringSegments = 20;
    for (int i = 0; i < ringSegments; ++i) {
        float a1 = static_cast<float>(i) * (2.0f * MathHelper::PI / ringSegments);
        float a2 = static_cast<float>(i + 1) * (2.0f * MathHelper::PI / ringSegments);
        VECTOR p1 = VGet(m_pos.x + std::cos(a1) * ringRadius, headY, m_pos.z + std::sin(a1) * ringRadius);
        VECTOR p2 = VGet(m_pos.x + std::cos(a2) * ringRadius, headY, m_pos.z + std::sin(a2) * ringRadius);
        DrawLine3D(p1, p2, GetColor(255, 230, 80));
    }
}

void Player3D::Draw3D() {
    // 1. 飛びつき・タックル残像の描画
    for (const auto& trail : m_trails) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, trail.alpha);
        ModelManager::GetInstance().DrawFallbackCat(trail.pos, trail.rotY, trail.repCount, false, true, m_animTime);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // 2. プレイヤー本体の描画（3Dモデルまたはフォールバック）
    bool isSoreness = (m_state == MuscleState::Soreness);
    if (!ModelManager::GetInstance().DrawModelIfLoaded(
            ModelConfig::CAT_MODEL_PATH, m_pos, m_rotY + ModelConfig::CAT_MODEL_ROT_Y,
            ModelConfig::CAT_MODEL_SCALE)) {
        ModelManager::GetInstance().DrawFallbackCat(
            m_pos, m_rotY, m_repCount, isSoreness, (m_isPouncing || m_isTackling), m_animTime);
    }

    // 3. タックル突進オーラエフェクト
    if (m_isTackling) {
        float r = GetTackleRadius();
        DrawSphere3D(VGet(m_pos.x, m_pos.y + 15.0f, m_pos.z), r, 12, GetColor(255, 160, 40), GetColor(255, 220, 80), FALSE);
    }

    // 4. 猫のスタンエフェクト
    DrawStunEffect();
}

void Player3D::Draw2D() {
    const auto& fm = FontManager::GetInstance();
    int font16 = fm.GetFont16();
    int font18 = fm.GetFont18();
    int font24 = fm.GetFont24();

    // ------------------------------------------------------------------------
    // スキルチェックQTEバー（画面中央下部に表示）
    // ------------------------------------------------------------------------
    if (m_isSkillChecking) {
        int barW = 320;
        int barH = 26;
        int barX = (Config::SCREEN_WIDTH - barW) / 2;
        int barY = Config::SCREEN_HEIGHT - 120;

        // バー背景
        DrawBox(barX - 4, barY - 4, barX + barW + 4, barY + barH + 4, GetColor(20, 20, 30), TRUE);
        DrawBox(barX, barY, barX + barW, barY + barH, GetColor(60, 60, 70), TRUE);

        // 成功ゾーン（緑色）
        int zoneX1 = barX + static_cast<int>(m_scZoneStart * barW);
        int zoneX2 = barX + static_cast<int>(m_scZoneEnd * barW);
        DrawBox(zoneX1, barY, zoneX2, barY + barH, GetColor(60, 220, 90), TRUE);

        // 針（通常時は赤、停止確定時は判定結果色）
        int cursorX = barX + static_cast<int>(m_scCursor * barW);
        unsigned int needleColor = GetColor(255, 60, 60);
        if (m_scStoppedTimer > 0) {
            needleColor = m_lastResultSuccess ? GetColor(255, 255, 50) : GetColor(255, 40, 40);
        }
        DrawBox(cursorX - 4, barY - 8, cursorX + 4, barY + barH + 8, needleColor, TRUE);

        // ガイドテキスト
        if (m_scStoppedTimer > 0) {
            if (m_lastResultSuccess) {
                DrawStringToHandle(barX + 110, barY - 28, "★ NICE PUMP!! ★", GetColor(255, 240, 60), font18);
            } else {
                DrawStringToHandle(barX + 120, barY - 28, "× MISS! ×", GetColor(255, 80, 80), font18);
            }
        } else {
            DrawStringToHandle(barX + 60, barY - 28, "緑のゾーンで [SPACE] を押せ！", GetColor(255, 240, 80), font16);
        }
    }

    // ------------------------------------------------------------------------
    // スキルチェック結果・筋肉痛・減衰通知テキスト
    // ------------------------------------------------------------------------
    if (m_resultShowTimer > 0) {
        int alpha = (m_resultShowTimer > 20) ? 255 : (m_resultShowTimer * 255 / 20);
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        if (m_wasDecayed) {
            DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 120, Config::SCREEN_HEIGHT / 2 - 40, "10秒放置: 筋肉が減衰した！ (0 Rep)", GetColor(180, 180, 220), font18);
        } else if (m_state == MuscleState::Soreness) {
            DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 140, Config::SCREEN_HEIGHT / 2 - 40, "FAIL! 筋肉痛で5秒間動けない！", GetColor(255, 70, 70), font24);
        } else if (m_lastResultSuccess) {
            DrawFormatStringToHandle(Config::SCREEN_WIDTH / 2 - 100, Config::SCREEN_HEIGHT / 2 - 40, GetColor(255, 220, 50), font24, "PUMP UP!! +1 REP (%d Rep)", m_repCount);
        }
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ------------------------------------------------------------------------
    // 猫の激突スタン通知テキスト
    // ------------------------------------------------------------------------
    if (m_stunTimer > 0) {
        int alpha = (m_stunTimer % 10 < 5) ? 255 : 180;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawFormatStringToHandle(Config::SCREEN_WIDTH / 2 - 130, Config::SCREEN_HEIGHT / 2 - 65, GetColor(255, 220, 40), font24, "★ STUNNED!! 残り%.1fs ★", GetCatStunRemainingSeconds());
        DrawStringToHandle(Config::SCREEN_WIDTH / 2 - 110, Config::SCREEN_HEIGHT / 2 - 35, "壁・家具に激突して気絶中！", GetColor(255, 240, 120), font16);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // ------------------------------------------------------------------------
    // マウス左クリック長押し移動時の方向ガイドライン＆マーカー
    // ------------------------------------------------------------------------
    if (m_isMouseMoving && m_state != MuscleState::Soreness && m_stunTimer <= 0) {
        VECTOR catScreen = ConvWorldPosToScreenPos(m_pos);
        if (catScreen.z > 0.0f) {
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
