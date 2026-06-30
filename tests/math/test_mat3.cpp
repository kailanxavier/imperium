#include "math_test_helper.h"
using namespace imp::math;

// Construction and accessors
TEST(Mat3, DefaultIsIdentity)
{
    Mat3f m;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            EXPECT_FLOAT_EQ(m(r, c), r == c ? 1.f : 0.f); // if diagonal is 1
}

TEST(Mat3, ElementAccessRowCol)
{
    Mat3f m;
    m(1, 2) = 7.f;
    EXPECT_FLOAT_EQ(m(1, 2), 7.f);
    EXPECT_FLOAT_EQ(m.col[2][1], 7.f);
}

TEST(Mat3, ExtractFromMat4StripsTranslation)
{
    Mat4f m4 = makeTranslation(Vec3f(10, 20, 30));
    Mat3f m3(m4);
    // Upper-left 3x3 of a translation matrix is identity
    EXPECT_MAT3_NEAR(m3, Mat3f::identity(), kEpsF);
}

// Arithmetic
TEST(Mat3, MultiplyByIdentityIsIdempotent)
{
    Mat3f r = makeRotation3(Vec3f(0, 1, 0), 0.5f);
    EXPECT_MAT3_NEAR(r * Mat3f::identity(), r, kEpsF);
    EXPECT_MAT3_NEAR(Mat3f::identity() * r, r, kEpsF);
}

TEST(Mat3, MultiplyVec3)
{
    Mat3f id;
    Vec3f v(1, 2, 3);
    EXPECT_VEC3_NEAR(id * v, v, kEpsF);
}

// Transpose
TEST(Mat3, TransposeOfIdentityIsIdentity)
{
    EXPECT_MAT3_NEAR(transpose(Mat3f::identity()), Mat3f::identity(), kEpsF);
}

TEST(Mat3, TransposeSwapsOffDiagonal)
{
    Mat3f m;
    m(0, 1) = 5.f; m(1, 0) = 0.f;
    Mat3f t = transpose(m);
    EXPECT_FLOAT_EQ(t(1, 0), 5.f);
    EXPECT_FLOAT_EQ(t(0, 1), 0.f);
}

TEST(Mat3, DoubleTransposeIsOriginal)
{
    Mat3f m = makeRotation3(normalise(Vec3f(1, 1, 1)), 1.2f);
    EXPECT_MAT3_NEAR(transpose(transpose(m)), m, kEpsF);
}

// Inverse
TEST(Mat3, InverseOfIdentityIsIdentity)
{
    EXPECT_MAT3_NEAR(inverse(Mat3f::identity()), Mat3f::identity(), kEpsF);
}

TEST(Mat3, InverseTimesOriginalIsIdentity)
{
    Mat3f m = makeRotation3(normalise(Vec3f(1, 2, 3)), 0.8f);
    Mat3f result = inverse(m) * m;
    EXPECT_MAT3_NEAR(result, Mat3f::identity(), kEpsFF);
}

// Normal matrix
TEST(Mat3, NormalMatrixOfPureRotationIsRotationItself)
{
    // For orthogonal matrices inverse == transpose, so normal matrix == original
    Mat4f rot = makeRotationY(0.7f);
    Mat3f nm = makeNormalMatrix(rot);
    Mat3f m3(rot);
    EXPECT_MAT3_NEAR(nm, m3, kEpsFF);
}

// Edge cases
TEST(Mat3, DeterminantOfIdentityIsOne)
{
    EXPECT_NEAR(determinant(Mat3f::identity()), 1.f, kEpsF);
}

TEST(Mat3, DeterminantOfScaledMatrixIsScaleCubed)
{
    Mat3f m = makeRotation3(Vec3f(0, 1, 0), 0.f);
    // Scale each basis by 2 manually
    for (int c = 0; c < 3; ++c) m.col[c] = m.col[c] * 2.f;
    EXPECT_NEAR(determinant(m), 8.f, kEpsFF);
}
