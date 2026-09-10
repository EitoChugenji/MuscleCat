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
    : GameObject3D(pos, 12.0f, ObjectType::Player)
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
    // キー入力受付（筋トレ: SPACE/Z、飛びつき: SHIFT/X/C）
    // ------------------------------------------------------------------------
    bool currentTriggerKey = (CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_Z));
    bool isTriggerJustPressed = currentTriggerKey && !m_prevTriggerKey;
    m_prevTriggerKey = currentTriggerKey;

    bool currentPounceKey = (CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT) ||
                             CheckHitKey(KEY_INPUT_X) || CheckHitKey(KEY_INPUT_C));
    bool isPounceJustPressed = currentPounceKey && !m_prevPounceKey;
    m_prevPounceKey = currentPounceKey;

    if (m_resultShowTimer > 0) {
        m_resultShowTimer--;
    }
    if (m_pounceCooldown > 0) {
        m_pounceCooldown--;
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
                // ★【4 Rep以上特権】飛びつき発動チェック
                if (CanPounce() && isPounceJustPressed) {
                    m_isPouncing = true;
                    m_pounceTimer = GetPounceDuration();

                    // 向いている方向を飛びつき突進方向に設定
                    m_pounceDir = VGet(std::sin(m_rotY), 0.0f, std::cos(m_rotY));
                    return;
                }

                // 移動入力処理 (WASD / 矢印キー) - カメラのXZ平面基準
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

                    // 難易度に応じたゾーン設定（Rep数が増えるほどゾーンが狭く・速くなる）
                    float zoneWidth = 0.30f - static_cast<float>(m_repCount) * 0.02f;
                    if (zoneWidth < 0.12f) zoneWidth = 0.12f;
                    m_scZoneStart = 0.45f + static_cast<float>(rand() % 25) / 100.0f;
                    if (m_scZoneStart + zoneWidth > 0.95f) {
                        m_scZoneStart = 0.95f - zoneWidth;
                    }
                    m_scZoneEnd = m_scZoneStart + zoneWidth;
                }
            } else {
                // ============================================================
                // スキルチェックQTE実行中（針の移動とタイミング判定）
                // ============================================================
                float cursorSpeed = 0.022f + static_cast<float>(m_repCount) * 0.003f;
                m_scCursor += cursorSpeed;

                if (isTriggerJustPressed) {
                    // タイミング判定
                    if (m_scCursor >= m_scZoneStart && m_scCursor <= m_scZoneEnd) {
                        // 【成功】Rep追加 & 10秒タイマーリセット
                        m_repCount++;
                        m_pumpDecayTimer = 600; // 10秒
                        m_isSkillChecking = false;
                        m_lastResultSuccess = true;
                        m_resultShowTimer = 60;
                    } else {
                        // 【失敗】筋肉痛ペナルティ (5秒停止)
                        m_isSkillChecking = false;
                        m_state = MuscleState::Soreness;
                        m_sorenessTimer = 300; // 5秒
                        m_repCount = 0;
                        m_lastResultSuccess = false;
                        m_resultShowTimer = 90;
                    }
                } else if (m_scCursor >= 1.0f) {
                    // 【タイムアウト見逃し失敗】
                    m_isSkillChecking = false;
                    m_state = MuscleState::Soreness;
                    m_sorenessTimer = 300;
                    m_repCount = 0;
                    m_lastResultSuccess = false;
                    m_resultShowTimer = 90;
                }
            }
        }
    }
}

void Player3D::Draw3D() {
    // 1. 飛びつき残像の描画
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
            m_pos, m_rotY, m_repCount, isSoreness, m_isPouncing, m_animTime);
    }
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

        // 針（赤色）
        int cursorX = barX + static_cast<int>(m_scCursor * barW);
        DrawBox(cursorX - 3, barY - 6, cursorX + 3, barY + barH + 6, GetColor(255, 60, 60), TRUE);

        // ガイドテキスト
        DrawStringToHandle(barX + 60, barY - 28, "緑のゾーンで [SPACE] を押せ！", GetColor(255, 240, 80), font16);
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
}
