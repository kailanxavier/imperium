#include "math_test_helper.h"
using namespace imp::math;

TEST(Vec3, DefaultConstructorIsZero)
{
    Vec3f v;
    EXPECT_FLOAT_EQ(v.x, 0.f);
    EXPECT_FLOAT_EQ(v.y, 0.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
}

TEST(Vec3, ScalarConstructorSetsAll)
{
    Vec3f v(5.f);
    EXPECT_FLOAT_EQ(v.x, 5.f);
    EXPECT_FLOAT_EQ(v.y, 5.f);
    EXPECT_FLOAT_EQ(v.z, 5.f);
}

TEST(Vec3, ComponentConstructor)
{
    Vec3f v(1.f, 2.f, 3.f);
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
    EXPECT_FLOAT_EQ(v.z, 3.f);
}

TEST(Vec3, PromotionFromVec3DefaultsZToZero)
{
    Vec2f xy(1.f, 2.f);
    Vec3f v(xy);
    EXPECT_FLOAT_EQ(v.x, 1.f);
    EXPECT_FLOAT_EQ(v.y, 2.f);
    EXPECT_FLOAT_EQ(v.z, 0.f);
}

TEST(Vec3, PromotionFromVec3WithExplicitZ)
{
    Vec2f xy(1.f, 2.f);
    Vec3f v(xy, 9.f);
    EXPECT_FLOAT_EQ(v.z, 9.f);
}

TEST(Vec3, IndexOperatorReadAndWrites)
{
    Vec3f v(1, 2, 3);
    EXPECT_FLOAT_EQ(v[0], 1.f);
    EXPECT_FLOAT_EQ(v[1], 2.f);
    EXPECT_FLOAT_EQ(v[2], 3.f);
    v[1] = 99.f;
    EXPECT_FLOAT_EQ(v.y, 99.f);
}

TEST(Vec3, ConstantsAreCorrect)
{
    EXPECT_VEC3_NEAR(Vec3f::zero(), (Vec3f({0, 0, 0})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::one(), (Vec3f({1, 1, 1})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::unitX(), (Vec3f({1, 0, 0})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::unitY(), (Vec3f({0, 1, 0})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::unitZ(), (Vec3f({0, 0, 1})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::up(), (Vec3f({0, 1, 0})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::right(), (Vec3f({1, 0, 0})), kEpsF);
    EXPECT_VEC3_NEAR(Vec3f::forward(), (Vec3f({0, 0, -1})), kEpsF);
}

// Arithmetic operators
TEST(Vec3, Addition)
{
    EXPECT_VEC3_NEAR(Vec3f(1, 2, 3) +
        Vec3f(4, 5, 6),
        Vec3f({5, 7, 9}), kEpsF);
}

TEST(Vec3, Subtraction)
{
    EXPECT_VEC3_NEAR(Vec3f(4, 5, 6) -
        Vec3f(1, 2, 3),
        Vec3f({3, 3, 3}), kEpsF);
}

TEST(Vec3, ScalarMultiply)
{
    EXPECT_VEC3_NEAR(Vec3f(1.f, 2.f, 3.f) * 2.f,
        Vec3f({2, 4, 6}), kEpsF);

    EXPECT_VEC3_NEAR(Vec3f(2.f * Vec3f(1.f, 2.f, 3.f)),
        Vec3f({2, 4, 6}), kEpsF);
}

TEST(Vec3, ScalarDivide)
{
    EXPECT_VEC3_NEAR(Vec3f(2.f, 4.f, 6.f) / 2.f,
        Vec3f({1, 2, 3}), kEpsF);
}

TEST(Vec3, ComponentWiseMultiply)
{
    EXPECT_VEC3_NEAR(Vec3f(2, 3, 4) *
        Vec3f(5, 6, 7), Vec3f(10, 18, 28), kEpsF);
}

TEST(Vec3, Negation)
{
    EXPECT_VEC3_NEAR(-Vec3f(1, -2, 3),
        Vec3f({-1, 2, -3}), kEpsF);
}

TEST(Vec3, CompoundAssignment)
{
    Vec3f v(1, 2, 3);
    v += Vec3f(1, 1, 1); EXPECT_VEC3_NEAR(v, Vec3f(2, 3, 4), kEpsF);
    v -= Vec3f(1, 1, 1); EXPECT_VEC3_NEAR(v, Vec3f(1, 2, 3), kEpsF);
    v *= 3.f; EXPECT_VEC3_NEAR(v, Vec3f(3, 6, 9), kEpsF);
    v /= 3.f; EXPECT_VEC3_NEAR(v, Vec3f(1, 2, 3), kEpsF);
}

// Geometric operations

TEST(Vec3, DotProductOrthogonal)
{
    EXPECT_NEAR(dot(Vec3f(1, 0, 0), Vec3f(0, 1, 0)), 0.f, kEpsF);
}

TEST(Vec3, DotProductParallel)
{
    EXPECT_NEAR(dot(Vec3f(1, 0, 0), Vec3f(1, 0, 0)), 1.f, kEpsF);
}

TEST(Vec3, DotProductKnownValue)
{
    EXPECT_NEAR(dot(Vec3f(1, 2, 3), Vec3f(4, 5, 6)), 32.f, kEpsF);
}

TEST(Vec3, CrossProductBasisVectors)
{
    EXPECT_VEC3_NEAR(cross(Vec3f(1, 0, 0),
        Vec3f(0, 1, 0)),
        Vec3f(0, 0, 1), kEpsF);

    EXPECT_VEC3_NEAR(cross(Vec3f(0, 1, 0),
        Vec3f(0, 0, 1)),
        Vec3f(1, 0, 0), kEpsF);

    EXPECT_VEC3_NEAR(cross(Vec3f(0, 0, 1),
        Vec3f(1, 0, 0)),
        Vec3f(0, 1, 0), kEpsF);
}

TEST(Vec3, CrossProductAnticommutative)
{
    Vec3f a(1, 2, 3), b(4, 5, 6);
    EXPECT_VEC3_NEAR(cross(a, b), -cross(b, a), kEpsF);
}

TEST(Vec3, CrossProductParallelVectorIsZero)
{
    EXPECT_VEC3_NEAR(cross(Vec3f(1, 0, 0),
        Vec3f(2, 0, 0)),
        Vec3f::zero(), kEpsF);
}

TEST(Vec3, Length)
{
    EXPECT_NEAR(length(Vec3f(1, 2, 2)), 3.f, kEpsF);
    EXPECT_NEAR(lengthSq(Vec3f(1, 2, 2)), 9.f, kEpsF);
}

TEST(Vec3, NormaliseProducesUnitVector)
{
    Vec3f v = normalise(Vec3f(1.f, 2.f, 2.f));
    EXPECT_NEAR(length(v), 1.f, kEpsF);
    EXPECT_VEC3_NEAR(v, Vec3f(1.f / 3.f, 2.f / 3.f, 2.f / 3.f), kEpsF);
}

TEST(Vec3, Distance)
{
    EXPECT_NEAR(distance(Vec3f(0, 0, 0), Vec3f(1, 2, 2)), 3.f, kEpsF);
    EXPECT_NEAR(distanceSq(Vec3f(0, 0, 0), Vec3f(1, 2, 2)), 9.f, kEpsF);
}

TEST(Vec3, AngleBetweenOrthogonalIsHalfPi)
{
    EXPECT_NEAR(angleBetween(Vec3f(1, 0, 0), Vec3f(0, 1, 0)),
        kPif * 0.5f, kEpsF);
}

TEST(Vec3, AngleBetweenSameVectorIsZero)
{
    EXPECT_NEAR(angleBetween(Vec3f(0, 1, 0), Vec3f(0, 1, 0)), 0.f, kEpsF);
}

TEST(Vec3, AngleBetweenOppositeIsPi)
{
    EXPECT_NEAR(angleBetween(Vec3f(1, 0, 0), Vec3f(-1, 0, 0)), kPif, kEpsF);
}

// Interpolation
TEST(Vec3, LerpEndpoints)
{
    Vec3f a(0, 0, 0), b(1, 2, 3);
    EXPECT_VEC3_NEAR(lerp(a, b, 0.f), a, kEpsF);
    EXPECT_VEC3_NEAR(lerp(a, b, 1.f), b, kEpsF);
}

TEST(Vec3, LerpMidpoint)
{
    EXPECT_VEC3_NEAR(lerp(Vec3f(0, 0, 0),
        Vec3f(2, 4, 6), 0.5f),
        Vec3f(1, 2, 3), kEpsF);
}

TEST(Vec3, SlerpEndpoints)
{
    Vec3f a = normalise(Vec3f(1, 0, 0));
    Vec3f b = normalise(Vec3f(0, 1, 0));
    EXPECT_VEC3_NEAR(slerp(a, b, 0.f), a, kEpsFF);
    EXPECT_VEC3_NEAR(slerp(a, b, 1.f), b, kEpsFF);
}

TEST(Vec3, SlerpMidpointIsUnitVector)
{
    // Slerp at t = 0.5 between two unit vectors must itself be a unit vector
    Vec3f a = normalise(Vec3f(1, 0, 0));
    Vec3f b = normalise(Vec3f(0, 1, 0));
    Vec3f m = slerp(a, b, 0.5f);
    EXPECT_NEAR(length(m), 1.f, kEpsFF);

    // Midpoint on the great circle between +x and +y (sqrt(2)/2, sqrt(2)/2, 0)
    EXPECT_VEC3_NEAR(m, Vec3f({0.70710678f, 0.70710678f, 0.f}), kEpsFF);
}

TEST(Vec3, SlerpFallsBackToLerpForNearlyIdenticalVectors)
{
    Vec3f a(1, 0, 0);
    Vec3f b = normalise(Vec3f(1.f + 1e-8f, 0, 0));

    // Should NOT NaN/assert
    Vec3f r = slerp(a, b, 0.5f);
    EXPECT_NEAR(length(r), 1.f, kEpsFF);
}

// Edge cases
TEST(Vec3, LengthOfZeroVector)
{
    EXPECT_FLOAT_EQ(length(Vec3f(0, 0, 0)), 0.f);
}

TEST(Vec3, NormaliseAlreadyUnitVectorIsStable)
{
    Vec3f v = normalise(Vec3f(0.f, 1.f, 0.f));
    EXPECT_NEAR(length(v), 1.f, kEpsF);
}
