#pragma once
#include "DxLib.h"
#include <cmath>

// ============================================================================
// オブジェクト種別定義
// ============================================================================
enum class ObjectType {
    Player,
    NormalMouse,
    FastMouse,
    Obstacle
};

// ============================================================================
// 数学・ベクトル補助関数
// ============================================================================
namespace MathHelper {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    inline float Clamp(float v, float minVal, float maxVal) {
        if (v < minVal) return minVal;
        if (v > maxVal) return maxVal;
        return v;
    }

    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // 2点間のXZ平面距離
    inline float DistanceXZ(const VECTOR& a, const VECTOR& b) {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    // 角度の最短補間（ラジアン）
    inline float LerpAngle(float from, float to, float t) {
        float diff = std::fmod(to - from + PI, PI * 2.0f);
        if (diff < 0.0f) diff += PI * 2.0f;
        diff -= PI;
        return from + diff * t;
    }
}
