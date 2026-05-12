#include "math_test_helper.h"
using namespace imp::math;

// Constructors and accessors
TEST(Vec4, DefaultConstructorIsZero)
{
    Vec4f v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
    EXPECT_FLOAT_EQ(v.w, 0.f);
}

TEST(Vec4, ComponentConstructor)
{
    Vec4f v(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
    EXPECT_FLOAT_EQ(v.z, 3.f);
    EXPECT_FLOAT_EQ(v.w, 4.f);
}

TEST(Vec4, PromotionFromVec3DefaultsWToZero)
{
    Vec3f xyz(1, 2, 3);
    Vec4f v(xyz);

    EXPECT_FLOAT_EQ(v.w, 0.f);
}

TEST(Vec4, IndexOperatorReadsAndWrites)
{
    Vec4f v(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(v[3], 4.f);
    v[3] = 99.f;
    EXPECT_FLOAT_EQ(v.w, 99.f);
}

TEST(Vec4, XyzExtractorMatchesComponents)
{
    Vec4f v(1, 2, 3, 4);
    Vec3f xyz = v.xyz();

    EXPECT_VEC3_NEAR(v, Vec3f(1, 2, 3), kEpsF);
}

// Arithmetic operations
TEST(Vec4, Addition)
{
    EXPECT_VEC4_NEAR(Vec4f({1, 2, 3, 4}) +
        Vec4f(5, 6, 7, 8),
        Vec4f({6, 8, 10, 12}), kEpsF);
}

TEST(Vec4, ScalarMultiply)
{
    EXPECT_VEC4_NEAR(Vec4f({1, 2, 3, 4}) * 2.f,
        Vec4f({2, 4, 6, 8}), kEpsF);

    EXPECT_VEC4_NEAR(2.f * Vec4f({1, 2, 3, 4}),
        Vec4f({2, 4, 6, 8}), kEpsF);
}

TEST(Vec4, Negation)
{
    EXPECT_VEC4_NEAR(-Vec4f(1, -2, 3, -4),
            Vec4f(-1, 2, -3, 4), kEpsF
        );
}

// Geometric operations
TEST(Vec4, Dot)
{
    EXPECT_NEAR(dot(Vec4f(1, 0, 0, 0), Vec4f(0, 1, 0, 0)), 0.f, kEpsF);
    EXPECT_NEAR(dot(Vec4f(1, 2, 3, 4), Vec4f(1, 2, 3, 4)), 30.f, kEpsF);
}

TEST(Vec4, LengthAndNormalise)
{
    Vec4f v(1, 0, 0, 0);
    EXPECT_NEAR(length(v), 1.f, kEpsF);

    Vec4f n = normalise(Vec4f(1, 1, 1, 1));
    EXPECT_NEAR(length(n), 1.f, kEpsF);
}

TEST(Vec4, Distance)
{
    Vec4f a(0, 0, 0, 0), b(1, 0, 0, 0);
    EXPECT_NEAR(distance(a, b), 1.f, kEpsF);
}

// Homogenous helpers
TEST(Vec4, PerspectiveDivideByOne)
{
    Vec4f v(3, 6, 9, 1);
    Vec3f r = perspectiveDivide(v);
    EXPECT_VEC3_NEAR(r, Vec3f(3, 6, 9), kEpsF);
}

TEST(Vec4, PerspectiveDivideByTwo)
{
    Vec4f v(4, 8, 10, 2);
    Vec3f r = perspectiveDivide(v);
    EXPECT_VEC3_NEAR(r, Vec3f(2, 4, 5), kEpsF);
}

// Interpolation
TEST(Vec4, LerpEndpoints)
{
    Vec4f a(0, 0, 0, 0), b(2, 4, 6, 8);
    EXPECT_VEC4_NEAR(lerp(a, b, 0.f), a, kEpsF);
    EXPECT_VEC4_NEAR(lerp(a, b, 1.f), b, kEpsF);
}

TEST(Vec4, LerpMidpoint)
{
    EXPECT_VEC4_NEAR(lerp(Vec4f(0, 0, 0, 0), Vec4f(2, 4, 6, 8), 0.5f), Vec4f(1, 2, 3, 4), kEpsF);
}

TEST(Vec4, SlerpMidpointIsUnit)
{
    Vec4f a = normalise(Vec4f(1, 0, 0, 0));
    Vec4f b = normalise(Vec4f(0, 1, 0, 0));
    Vec4f m = slerp(a, b, 0.5f);
    EXPECT_NEAR(length(m), 1.f, kEpsF);
}

TEST(Vec4, LengthOfZeroVector)
{
    EXPECT_FLOAT_EQ(length(Vec4f(0, 0, 0, 0)), 0.f);
}