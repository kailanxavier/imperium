#include "math_test_helper.h"
using namespace imp::math;

// Construction and accessors
TEST(Mat4, DefaultIsIdentity)
{
    Mat4f m;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            EXPECT_FLOAT_EQ(m(r, c), r == c ? 1.f : 0.f);
}

TEST(Mat4, ElementAccessIsColumnMajor)
{
    Mat4f m;
    m(1, 3) = 42.f;
    EXPECT_FLOAT_EQ(m.col[3][1], 42.f);
}

TEST(Mat4, DataPointerLayoutIsColumnMajor)
{
    Mat4f m = makeTranslation(Vec3f(1, 2, 3));
    const float* p = m.data();

    // col[3] = { 1, 2, 3, 1 } starts at offset 12
    EXPECT_FLOAT_EQ(p[12], 1.f);
    EXPECT_FLOAT_EQ(p[13], 2.f);
    EXPECT_FLOAT_EQ(p[14], 3.f);
    EXPECT_FLOAT_EQ(p[15], 1.f);
}

// Matrix multiplication
TEST(Mat4, MultiplyByIdentityIsIdempotent)
{
    Mat4f m = makeTranslation(Vec3f(1, 2, 3));
    EXPECT_MAT4_NEAR(m * Mat4f::identity(), m, kEpsF);
    EXPECT_MAT4_NEAR(Mat4f::identity() * m, m, kEpsF);
}

TEST(Mat4, TranslationConcatenation)
{
    Mat4f t1 = makeTranslation(Vec3f(1, 0, 0));
    Mat4f t2 = makeTranslation(Vec3f(0, 2, 0));
    Mat4f combined = t1 * t2; // first t2, then t1
    Vec3f p = transformPoint(combined, Vec3f(0, 0, 0));
    EXPECT_VEC3_NEAR(p, Vec3f({1, 2, 0}), kEpsF);
}

TEST(Mat4, ScaleConcatenation)
{
    Mat4f s1 = makeScale(Vec3f(2, 2, 2));
    Mat4f s2 = makeScale(Vec3f(3, 3, 3));
    Vec3f p = transformPoint(s1 * s2, Vec3f(1, 1, 1));
    EXPECT_VEC3_NEAR(p, Vec3f({6, 6, 6}), kEpsF);
}

// Transform factories
TEST(Mat4, TranslationTransformsPoint)
{
    Mat4f t = makeTranslation(Vec3f(3, 4, 5));
    Vec3f p = transformPoint(t, Vec3f(1, 1, 1));
    EXPECT_VEC3_NEAR(p, Vec3f({4, 5, 6}), kEpsF);
}

TEST(Mat4, TranslationDoesNotAffectDirection)
{
    Mat4f t = makeTranslation(Vec3f(100, 200, 300));
    Vec3f d = transformDirection(t, Vec3f(0, 1, 0));
    EXPECT_VEC3_NEAR(d, Vec3f({0, 1, 0}), kEpsF);
}

TEST(Mat4, UniformScaleTransformsPoints)
{
    Mat4f s = makeScale(2.f);
    Vec3f p = transformPoint(s, Vec3f(1, 2, 3));
    EXPECT_VEC3_NEAR(p, Vec3f({2, 4, 6}), kEpsF);
}

TEST(Mat4, RotationXBy90DegreesTransformsYToZ)
{
    float angle = kPif * 0.5f;
    Mat4f r = makeRotationX(angle);
    Vec3f v = transformDirection(r, Vec3f(0, 1, 0));
    EXPECT_VEC3_NEAR(v, Vec3f({0, 0, -1}), kEpsFF);
}

TEST(Mat4, RotationYBy90DegreesTransformsZToNegX)
{
    float angle = kPif * 0.5f;
    Mat4f r = makeRotationY(angle);
    Vec3f v = transformDirection(r, Vec3f(0, 0, 1));
    EXPECT_VEC3_NEAR(v, Vec3f(-1, 0, 0), kEpsFF);
}

TEST(Mat4, RotationAxisAngleKnownResult)
{
    Mat4f r = makeRotationAxis(Vec3f(0, 1, 0), kPif);
    EXPECT_VEC3_NEAR(transformDirection(r, Vec3f(1, 0, 0)), Vec3f(-1, 0, 0), kEpsFF);
    EXPECT_VEC3_NEAR(transformDirection(r, Vec3f(0, 1, 0)), Vec3f(0, 1, 0), kEpsFF);
}

// Transpose
TEST(Mat4, TransposeOfIdentityIsIdentity)
{
    EXPECT_MAT4_NEAR(transpose(Mat4f::identity()), Mat4f::identity(), kEpsF);
}

TEST(Mat4, DoubleTransposeIsOriginal)
{
    Mat4f m = makeRotationAxis(normalise(Vec3f(1, 2, 3)), 1.f);
    EXPECT_MAT4_NEAR(transpose(transpose(m)), m, kEpsF);
}

// Inverse
TEST(Mat4, InverseTimesOriginalIsIdentity)
{
    Mat4f m = makeTranslation(Vec3f(1, 2, 3)) * makeRotationY(0.5f) * makeScale(2.f);
    Mat4f result = inverse(m) * m;
    EXPECT_MAT4_NEAR(result, Mat4f::identity(), kEpsFF);
}

// TEST(Mat4, InverseRigidMatchesFullInverseForRigidbody)
// {
//     Mat4f m = makeTranslation(Vec3f(3, -1, 2)) * makeRotationAxis(normalise(Vec3f(1, 1, 0)), 0.9f);
//     Mat4f full = inverse(m);
//     Mat4f rigid = inverseRigidbody(m);
//     EXPECT_MAT4_NEAR(full, rigid, kEpsFF);
// }

// View matrix
TEST(Mat4, LookAtTransformsEyeToOrigin)
{
    Vec3f eye(0, 0, -5), target(0, 0, 0), up(0, 1, 0);
    Mat4f view = makeLookAtLH(eye, target, up);
    Vec3f eyeInView = transformPoint(view, eye);
    EXPECT_VEC3_NEAR(eyeInView, Vec3f::zero(), kEpsFF);
}

TEST(Mat4, LookAtTransformsTargetToPositiveZ)
{
    Vec3f eye(0, 0, -5), target(0, 0, 0), up(0, 1, 0);
    Mat4f view = makeLookAtLH(eye, target, up);
    Vec3f targetInView = transformPoint(view, target);
    EXPECT_NEAR(targetInView.z, 5.f, kEpsFF);
    EXPECT_NEAR(targetInView.x, 0.f, kEpsFF);
    EXPECT_NEAR(targetInView.y, 0.f, kEpsFF);
}

// Projection matrices
TEST(Mat4, PerspectiveNearPlaneMapsToDXNearZ)
{
    // A point exactly at zNear should map to NDC z = 0.0 (DX convention)
    const float fov = kPif * 0.5f; // 90 deg
    const float aspect = 1.f, zNear = 1.f, zFar = 100.f;
    Mat4f proj = makePerspectiveLH(fov, aspect, zNear, zFar);

    // A point on the near place at the centre; (0, 0, zNear) as a view-space point
    Vec4f clip = proj * Vec4f(0.f, 0.f, zNear, 1.f);
    float ndcZ = clip.z / clip.w;
    EXPECT_NEAR(ndcZ, 0.f, kEpsFF);
}

TEST(Mat4, PerspectiveFarPlaneMapsToDXFarZ)
{
    const float fov = kPif * 0.5f;
    const float aspect = 1.f, zNear = 1.f, zFar = 100.f;
    Mat4f proj = makePerspectiveLH(fov, aspect, zNear, zFar);

    Vec4f clip = proj * Vec4f(0.f, 0.f, zFar, 1.f);
    float ndcZ = clip.z / clip.w;
    EXPECT_NEAR(ndcZ, 1.f, kEpsFF);
}

TEST(Mat4, OrthographicCentreMapsToDXZero)
{
    Mat4f ortho = makeOrthographicLH(2.f, 2.f, 0.f, 100.f);
    Vec4f clip = ortho * Vec4f(0.f, 0.f, 0.f, 1.f);

    EXPECT_NEAR(clip.x, 0.f, kEpsFF);
    EXPECT_NEAR(clip.y, 0.f, kEpsFF);
    EXPECT_NEAR(clip.z, 0.f, kEpsFF);
    EXPECT_NEAR(clip.w, 1.f, kEpsFF);
}

TEST(Mat4, OrthographicFarPlaneMapsToDXOne)
{
    Mat4f ortho = makeOrthographicLH(2.f, 2.f, 0.f, 100.f);
    Vec4f clip = ortho * Vec4f(0.f, 0.f, 100.f, 1.f);
    EXPECT_NEAR(clip.z / clip.w, 1.f, kEpsFF);
}

// Edge cases
TEST(Mat4, TRSDecompositionRoundTrip)
{
    Vec3f translation(1, 2, 3);
    Mat4f rotation = makeRotationY(kPif * 0.5f);
    Vec3f scale(2, 2, 2);

    Mat4f trs = makeTranslation(translation) * rotation * makeScale(scale);

    Vec3f result = transformPoint(trs, Vec3f(1, 0, 0));
    EXPECT_VEC3_NEAR(result, Vec3f(1.f, 2.f, 5.f), kEpsFF);
}
