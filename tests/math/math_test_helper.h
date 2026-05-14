#pragma once

#include <gtest/gtest.h>
#include <core/math/math.h>
#include <cmath>

static constexpr float kEpsF = 1e-5f;
static constexpr float kEpsFF = 1e-4f; // relaxed version for slerp, projection, etc
static constexpr double kEpsD = 1e-10f;

inline bool nearEq(float a, float b, float eps = kEpsF) { return std::abs(a - b) <= eps; }
inline bool nearEq(double a, double b, double eps = kEpsD) { return std::abs(a - b) <= eps; }

#define EXPECT_VEC2_NEAR(a, b, eps) \
    EXPECT_TRUE(nearEq((a).x, (b).x, eps)) << " x: " << (a).x << " vs " << (b).x; \
    EXPECT_TRUE(nearEq((a).y, (b).y, eps)) << " y: " << (a).y << " vs " << (b).y

#define EXPECT_VEC3_NEAR(a, b, eps) \
    EXPECT_TRUE(nearEq((a).x, (b).x, eps)) << " x: " << (a).x << " vs " << (b).x; \
    EXPECT_TRUE(nearEq((a).y, (b).y, eps)) << " y: " << (a).y << " vs " << (b).y; \
    EXPECT_TRUE(nearEq((a).z, (b).z, eps)) << " z: " << (a).z << " vs " << (b).z

#define EXPECT_VEC4_NEAR(a, b, eps) \
    EXPECT_TRUE(nearEq((a).x, (b).x, eps)) << " x: " << (a).x << " vs " << (b).x; \
    EXPECT_TRUE(nearEq((a).y, (b).y, eps)) << " y: " << (a).y << " vs " << (b).y; \
    EXPECT_TRUE(nearEq((a).z, (b).z, eps)) << " z: " << (a).z << " vs " << (b).z; \
    EXPECT_TRUE(nearEq((a).w, (b).w, eps)) << " w: " << (a).w << " vs " << (b).w

#define EXPECT_MAT3_NEAR(a, b, eps) \
    for (int _c = 0; _c < 3; ++_c) \
        for (int _r = 0; _r < 3; ++_r) \
            EXPECT_TRUE(nearEq((a)(_r, _c), (b)(_r, _c), eps)) \
                << " [" << _r << "][" << _c << "]: " << (a)(_r, _c) << " vs " << (b)(_r, _c)

#define EXPECT_MAT4_NEAR(a, b, eps) \
    for (int _c = 0; _c < 4; ++_c) \
        for (int _r = 0; _r < 4; ++_r) \
            EXPECT_TRUE(nearEq((a)(_r, _c), (b)(_r, _c), eps)) \
                << " [" << _r << "][" << _c << "]: " << (a)(_r, _c) << " vs " << (b)(_r, _c)

#define EXPECT_QUAT_NEAR(a, b, eps) \
    { \
        bool _pos = nearEq((a).x,(b).x,eps) && nearEq((a).y,(b).y,eps) \
                 && nearEq((a).z,(b).z,eps) && nearEq((a).w,(b).w,eps); \
        bool _neg = nearEq((a).x,-(b).x,eps) && nearEq((a).y,-(b).y,eps) \
                 && nearEq((a).z,-(b).z,eps) && nearEq((a).w,-(b).w,eps); \
        EXPECT_TRUE(_pos || _neg) \
            << "  quat (" << (a).x<<","<<(a).y<<","<<(a).z<<","<<(a).w \
            << ") vs ("   << (b).x<<","<<(b).y<<","<<(b).z<<","<<(b).w << ")"; \
    }