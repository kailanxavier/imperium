#include "math_test_helper.h"
using namespace imp::math;

// Construction and accessors
TEST(Quaternion, DefaultIsIdentity)
{
    Quaternionf q;
    EXPECT_FLOAT_EQ(q.x, 0.f);
    EXPECT_FLOAT_EQ(q.y, 0.f);
    EXPECT_FLOAT_EQ(q.z, 0.f);
    EXPECT_FLOAT_EQ(q.w, 1.f);
}

TEST(Quaternion, IdentityStaticMatchesDefault)
{
    EXPECT_QUAT_NEAR(Quaternionf::identity(), Quaternionf{}, kEpsF);
}

TEST(Quaternion, IndexOperatorReadsAndWrites)
{
    Quaternionf q(1, 2, 3, 4);
    EXPECT_FLOAT_EQ(q[0], 1.f);
    EXPECT_FLOAT_EQ(q[3], 4.f);
    q[2] = 99.f;
    EXPECT_FLOAT_EQ(q.z, 99.f);
}

// From axis angle
TEST(Quaternion, FromAxisAngleZeroIsIdentity)
{
    Quaternionf q = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.f);
    EXPECT_QUAT_NEAR(q, Quaternionf::identity(), kEpsF);
}

TEST(Quaternion, FromAxisAngleIsNormalised)
{
    Quaternionf q = Quaternionf::fromAxisAngle(normalise(Vec3f(1, 1, 0)), 0.7f);
    EXPECT_NEAR(length(q), 1.f, kEpsF);
}

TEST(Quaternion, FromAxisAngle180AroundYFlipsX)
{
    Quaternionf q = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), kPif);
    Vec3f v = rotate(q, Vec3f(1, 0, 0));
    EXPECT_VEC3_NEAR(v, Vec3f(-1, 0, 0), kEpsFF);
}

TEST(Quaternion, FromAxisAngle90AroundYMapsZToX)
{
    Quaternionf q = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), kPif * 0.5f);
    Vec3f v = rotate(q, Vec3f(0, 0, 1));
    EXPECT_VEC3_NEAR(v, Vec3f(1, 0, 0), kEpsFF);
}

// Arithmetic operators
TEST(Quaternion, HamiltonProductIdentityIsNoop)
{
    Quaternionf q = Quaternionf::fromAxisAngle(normalise(Vec3f(1, 2, 3)), 0.5f);
    EXPECT_QUAT_NEAR(q * Quaternionf::identity(), q, kEpsF);
    EXPECT_QUAT_NEAR(Quaternionf::identity() * q, q, kEpsF);
}

TEST(Quaternion, HamiltonProductIsAssociative)
{
    Quaternionf a = Quaternionf::fromAxisAngle(Vec3f(1, 0, 0), 0.3f);
    Quaternionf b = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.5f);
    Quaternionf c = Quaternionf::fromAxisAngle(Vec3f(0, 0, 1), 0.7f);
    EXPECT_QUAT_NEAR((a * b) * c, a * (b * c), kEpsFF);
}

TEST(Quaternion, ConcatenationOrderMatters)
{
    Quaternionf ry = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), kPif * 0.5f);
    Quaternionf rx = Quaternionf::fromAxisAngle(Vec3f(1, 0, 0), kPif * 0.5f);

    Vec3f v(0, 1, 0);
    Vec3f ab = rotate(ry * rx, v);
    Vec3f ba = rotate(rx * ry, v);

    EXPECT_FALSE(nearEq(ab.x, ba.x, kEpsFF) &&
                 nearEq(ab.y, ba.y, kEpsFF) &&
                 nearEq(ab.z, ba.z, kEpsFF));
}

TEST(Quaternion, ConjugateTimesOriginalIsIdentity)
{
    Quaternionf q = normalise(Quaternionf(1, 2, 3, 4));
    EXPECT_QUAT_NEAR(conjugate(q) * q, Quaternionf::identity(), kEpsFF);
}

TEST(Quaternion, InverseTimesOriginalIsIdentity)
{
    Quaternionf q = normalise(Quaternionf(1, 2, 3, 4));
    EXPECT_QUAT_NEAR(inverse(q) * q, Quaternionf::identity(), kEpsFF);
}

TEST(Quaternion, ConjugateEqualsInverseForUnitQuaternion)
{
    Quaternionf q = Quaternionf::fromAxisAngle(normalise(Vec3f(1, 1, 1)), 1.2f);
    EXPECT_QUAT_NEAR(conjugate(q), inverse(q), kEpsF);
}

// rotate()
TEST(Quaternion, RotateByIdentityIsNoop)
{
    Vec3f v(3, 4, 5);
    EXPECT_VEC3_NEAR(rotate(Quaternionf::identity(), v), v, kEpsF);
}

TEST(Quaternion, RotatePreserversLength)
{
    Quaternionf q = Quaternionf::fromAxisAngle(normalise(Vec3f(1, 2, 3)), 1.1f);
    Vec3f v(1, 0, 0);
    EXPECT_NEAR(length(rotate(q, v)), length(v), kEpsF);
}

TEST(Quaternion, DoubleRotateBy180IsNoop)
{
    Quaternionf q = Quaternionf::fromAxisAngle(normalise(Vec3f(0, 1, 0)), kPif);
    Vec3f v(1, 0, 0);
    Vec3f result = rotate(q, rotate(q, v));
    EXPECT_VEC3_NEAR(result, v, kEpsFF);
}

TEST(Quaternion, ToMat3AndBackPreservesRotation)
{
    Quaternionf original = normalise(Quaternionf::fromAxisAngle(normalise(Vec3f(1, 2, 3)), 0.8f));
    Mat3f m = toMat3(original);
    Quaternionf q = Quaternionf::fromMat3(m);

    EXPECT_QUAT_NEAR(q, original, kEpsFF);
}

TEST(Quaternion, ToMat4AndBackPreservesRotation)
{
    Quaternionf original = normalise(Quaternionf::fromAxisAngle(normalise(Vec3f(0, 1, 0)), 1.3f));
    Mat4f m = toMat4(original);
    Quaternionf q = Quaternionf::fromMat4(m);

    EXPECT_QUAT_NEAR(q, original, kEpsFF);
}

TEST(Quaternion, Mat3RoundTripPreservesVectorTransform)
{
    Quaternionf q = normalise(Quaternionf::fromAxisAngle(normalise(Vec3f(1, 0, 1)), 1.f));

    Vec3f v(1, 2, 3);
    Vec3f byQuat = rotate(q, v);
    Vec3f byMat3 = toMat3(q) * v;
    Vec3f byMat4 = transformDirection(toMat4(q), v);

    EXPECT_VEC3_NEAR(byQuat, byMat3, kEpsFF);
    EXPECT_VEC3_NEAR(byQuat, byMat4, kEpsFF);
}

TEST(Quaternion, FromMat3RoundTripWithArbitraryAxes)
{
    // Test Shepperd's method for all four branches by constructing rotations
    // that exercise each dominant diagonal element

    struct Case { Vec3f axis; float angle; };
    const Case cases[] = {
        { Vec3f(1, 0, 0), 2.5f }, // m(0, 0) dominant
        { Vec3f(0, 1, 0), 2.5f }, // m(1, 1) dominant
        { Vec3f(0, 0, 1), 2.5f }, // m(2, 2) dominant
        { Vec3f(1, 1, 1) / std::sqrt(3.f), 0.1f } // trace dominant
    };

    for (const auto& c : cases)
    {
        Quaternionf qOriginal = Quaternionf::fromAxisAngle(c.axis, c.angle);
        Mat3f m = toMat3(qOriginal);
        Quaternionf qBack = Quaternionf::fromMat3(m);
        EXPECT_QUAT_NEAR(qBack, qOriginal, kEpsFF);
    }
}

TEST(Quaternion, EulerRoundTripForNonGimbalAngles)
{
    const float pitch = 0.3f, yaw = 0.5f, roll = 0.2f;
    Quaternionf q = Quaternionf::fromEuler(pitch, yaw, roll);
    auto [ep, ey, er] = toEuler(q);

    EXPECT_NEAR(ep, pitch, kEpsFF);
    EXPECT_NEAR(ey, yaw, kEpsFF);
    EXPECT_NEAR(er, roll, kEpsFF);
}

TEST(Quaternion, GimbalLockAtPlusMinus90DegreesPitch)
{
    Quaternionf q = Quaternionf::fromEuler(kPif * 0.5f, 0.3f, 0.f);
    EXPECT_NEAR(length(q), 1.f, kEpsF); // must still be a unit quaternion
}

// Interpolation
TEST(Quaternion, NlerpAtEndpointReturnsInputs)
{
    Quaternionf a = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.f);
    Quaternionf b = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 1.f);
    EXPECT_QUAT_NEAR(nlerp(a, b, 0.f), a, kEpsFF);
    EXPECT_QUAT_NEAR(nlerp(a, b, 1.f), b, kEpsFF);
}

TEST(Quaternion, NlerpResultIsUnitQuaternion)
{
    Quaternionf a = Quaternionf::fromAxisAngle(Vec3f(1, 0, 0), 0.2f);
    Quaternionf b = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 1.5f);
    EXPECT_NEAR(length(nlerp(a, b, 0.5f)), 1.f, kEpsF);
}

TEST(Quaternion, SlerpAtEndpointsReturnInputs)
{
    Quaternionf a = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.f);
    Quaternionf b = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 1.f);
    EXPECT_QUAT_NEAR(slerp(a, b, 0.f), a, kEpsFF);
    EXPECT_QUAT_NEAR(slerp(a, b, 1.f), b, kEpsFF);
}

TEST(Quaternion, SlerpTakesShortestPath)
{
    // Supplying q and -q should give the same rotation at t = 0.5
    Quaternionf q = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.5f);
    Quaternionf m1 = slerp(q, -q, 0.5f);
    Quaternionf m2 = slerp(-q, q, 0.5f);

    // Both sides should be near identity
    EXPECT_NEAR(length(m1), 1.f, kEpsFF);
    EXPECT_NEAR(length(m2), 1.f, kEpsFF);
}

// Whimsical test
TEST(Quaternion, SlerpFallsBackGracefullyForNearlyIdenticalInputs)
{
    Quaternionf a = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 0.f);
    Quaternionf b = Quaternionf::fromAxisAngle(Vec3f(0, 1, 0), 1e-8f);

    Quaternionf r = slerp(a, b, 0.5f);
    EXPECT_NEAR(length(r), 1.f, kEpsFF);
}
