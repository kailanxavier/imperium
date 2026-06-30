#include "math_test_helper.h"
using namespace imp::math;

// Construction and accessors
TEST(Vec2, DefaultContructorIsZero)
{
    Vec2f v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
}

TEST(Vec2, ScalarContructorSetsAll)
{
    Vec2f v(3.f);
    EXPECT_FLOAT_EQ(v.x, 3.f);
    EXPECT_FLOAT_EQ(v.y, 3.f);
}

TEST(Vec2, ComponentConstructor)
{
    Vec2f v(1.f, 2.f);
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
}

TEST(Vec2, IndexOperatorReadsComponents)
{
    Vec2f v(4.f, 5.f);
    EXPECT_FLOAT_EQ(v[0], 4.f);
    EXPECT_FLOAT_EQ(v[1], 5.f);
}

TEST(Vec2, IndexOperatorWritesComponents)
{
    Vec2f v;
    v[0] = 7.f; v[1] = 8.f;
    EXPECT_FLOAT_EQ(v.x, 7.f);
    EXPECT_FLOAT_EQ(v.y, 8.f);
}

TEST(Vec2, ConstantsAreCorrect)
{
    EXPECT_VEC2_NEAR(Vec2f::zero(), (Vec2f({0, 0})), kEpsF);
    EXPECT_VEC2_NEAR(Vec2f::one(), (Vec2f({1, 1})), kEpsF);
    EXPECT_VEC2_NEAR(Vec2f::unitX(), (Vec2f({1, 0})), kEpsF);
    EXPECT_VEC2_NEAR(Vec2f::unitY(), (Vec2f({0, 1})), kEpsF);
}

TEST(Vec2, ConversionBetweenFloatAndDouble)
{
    Vec2d v(1.0, 2.0);
    Vec2f f(v);

    EXPECT_FLOAT_EQ(f.x, 1.f);
    EXPECT_FLOAT_EQ(f.y, 2.f);
}

// Arithmetic operators
TEST(Vec2, Dot)
{
    Vec2f a(1, 0), b(0, 1);
    EXPECT_NEAR(dot(a, b), 0, kEpsF);
    EXPECT_NEAR(dot(a, a), 1, kEpsF);

    Vec2f c(3, 4), d(1, 2);
    EXPECT_NEAR(dot(c, d), 11.f, kEpsF);
}

TEST(Vec2, Cross2D)
{
    // Cross of parallel vectors should be 0
    Vec2f a(1, 0), b(2, 0);
    EXPECT_NEAR(cross(a, b), 0.f, kEpsF);

    // Cross of perpendicular unit vectors is 1 or -1
    Vec2f x(1, 0), y(0, 1);
    EXPECT_NEAR(cross(x, y), 1.f, kEpsF);
    EXPECT_NEAR(cross(y, x), -1.f, kEpsF);
}

TEST(Vec2, Length)
{
    EXPECT_NEAR(length(Vec2f(3.f, 4.f)), 5.f, kEpsF);
    EXPECT_NEAR(length(Vec2f(1.f, 0.f)), 1.f, kEpsF);
    EXPECT_NEAR(lengthSq(Vec2f(3.f, 4.f)), 25.f, kEpsF);
}

TEST(Vec2, Normalise)
{
    Vec2f v = normalise(Vec2f(3.f, 4.f));
    EXPECT_NEAR(length(v), 1.f, kEpsF);
    EXPECT_VEC2_NEAR(v, (Vec2f({0.6f, 0.8f})), kEpsF);
}

TEST(Vec2, Distance)
{
    Vec2f a(0, 0), b(3, 4);
    EXPECT_NEAR(distance(a, b), 5.f, kEpsF);
    EXPECT_NEAR(distanceSq(a, b), 25.f, kEpsF);
}

TEST(Vec2, AngleBetween)
{
    Vec2f x(1, 0), y(0, 1);
    EXPECT_NEAR(angleBetween(x, y), 3.14159265f * 0.5f, kEpsF);
    EXPECT_NEAR(angleBetween(x, x), 0.f, kEpsF);
}

// Interpolation
TEST(Vec2, LerpAtEndpoints)
{
    Vec2f a(0, 0), b(10, 20);
    EXPECT_VEC2_NEAR(lerp(a, b, 0.f), a, kEpsF);
    EXPECT_VEC2_NEAR(lerp(a, b, 1.f), b, kEpsF);
}

TEST(Vec2, LerpAtMidpoints)
{
    Vec2f a(0, 0), b(10, 20);
    EXPECT_VEC2_NEAR(lerp(a, b, 0.5f), (Vec2f({5.f, 10.f})), kEpsF);
}

// Edge cases
TEST(Vec2, NormalizeUnitVectorIsIdempotent)
{
    Vec2f v = normalise(Vec2f(1.f, 0.f));
    EXPECT_VEC2_NEAR(v, (Vec2f{1, 0}), kEpsF);
}

TEST(Vec2, LengthOfZeroVector)
{
    EXPECT_FLOAT_EQ(length(Vec2f(0, 0)), 0.f);
}
