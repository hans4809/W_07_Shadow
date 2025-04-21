#include "Engine/Source/Runtime/Core/Math/JungleMath.h"
#include <DirectXMath.h>

#include "MathUtility.h"

using namespace DirectX;
FVector4 JungleMath::ConvertV3ToV4(FVector vec3)
{
	FVector4 newVec4;
	newVec4.x = vec3.x;
	newVec4.y = vec3.y;
	newVec4.z = vec3.z;
	return newVec4;
}



FMatrix JungleMath::CreateModelMatrix(FVector translation, FVector rotation, FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);

    FMatrix Rotation = FMatrix::CreateRotation(rotation.x, rotation.y, rotation.z);
    //FMatrix Rotation = JungleMath::EulerToQuaternion(rotation).ToMatrix();

    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}

FMatrix JungleMath::CreateModelMatrix(FVector translation, FQuat rotation, FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);
    FMatrix Rotation = rotation.ToMatrix();
    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}
FMatrix JungleMath::CreateViewMatrix(FVector eye, FVector target, FVector up)
{
    FVector zAxis = (target - eye).Normalize();  // DirectX는 LH이므로 -z가 아니라 +z 사용
    FVector xAxis = (up.Cross(zAxis)).Normalize();
    FVector yAxis = zAxis.Cross(xAxis);

    FMatrix View;
    View.M[0][0] = xAxis.x; View.M[0][1] = yAxis.x; View.M[0][2] = zAxis.x; View.M[0][3] = 0;
    View.M[1][0] = xAxis.y; View.M[1][1] = yAxis.y; View.M[1][2] = zAxis.y; View.M[1][3] = 0;
    View.M[2][0] = xAxis.z; View.M[2][1] = yAxis.z; View.M[2][2] = zAxis.z; View.M[2][3] = 0;
    View.M[3][0] = -xAxis.Dot(eye);
    View.M[3][1] = -yAxis.Dot(eye);
    View.M[3][2] = -zAxis.Dot(eye);
    View.M[3][3] = 1;

    return View;
}

FMatrix JungleMath::CreateProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane)
{
    float tanHalfFOV = tan(fov / 2.0f);
    float depth = farPlane - nearPlane;

    FMatrix Projection = {};
    Projection.M[0][0] = 1.0f / (aspect * tanHalfFOV);
    Projection.M[1][1] = 1.0f / tanHalfFOV;
    Projection.M[2][2] = farPlane / depth;
    Projection.M[2][3] = 1.0f;
    Projection.M[3][2] = -(nearPlane * farPlane) / depth;
    Projection.M[3][3] = 0.0f;  

    return Projection;
}

FMatrix JungleMath::CreateOrthoProjectionMatrix(float width, float height, float nearPlane, float farPlane)
{
    float r = width * 0.5f;
    float t = height * 0.5f;
    float invDepth = 1.0f / (farPlane - nearPlane);

    FMatrix Projection = {};
    Projection.M[0][0] = 1.0f / r;
    Projection.M[1][1] = 1.0f / t;
    Projection.M[2][2] = invDepth;
    Projection.M[3][2] = -nearPlane * invDepth;
    Projection.M[3][3] = 1.0f;

    return Projection;
}

void JungleMath::GetFrustumCornersWS(const FMatrix& camView, const FMatrix& camProj, float sliceNear, float sliceFar, float cameraNear, float cameraFar, TArray<FVector>& outCorners)
{
    // 1. 전체 카메라 프러스텀의 8개 NDC 코너 정의 (DirectX 스타일: near z = 0, far z = 1)
    const float ndcCorners[8][3] = {
        {-1.0f, -1.0f, 0.0f}, // 0: near bottom left
         { 1.0f, -1.0f, 0.0f}, // 1: near bottom right
         { 1.0f,  1.0f, 0.0f}, // 2: near top right
        {-1.0f,  1.0f, 0.0f}, // 3: near top left
        {-1.0f, -1.0f, 1.0f}, // 4: far bottom left
         { 1.0f, -1.0f, 1.0f}, // 5: far bottom right
         { 1.0f,  1.0f, 1.0f}, // 6: far top right
        {-1.0f,  1.0f, 1.0f}  // 7: far top left
    };

    // 2. 전체 프러스텀의 월드 코너 구하기
    FMatrix invViewProj = FMatrix::Inverse(camView * camProj);
    FVector worldCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        FVector4 ptNdc;
        ptNdc.x = ndcCorners[i][0];
        ptNdc.y = ndcCorners[i][1];
        ptNdc.z = ndcCorners[i][2];
        ptNdc.w = 1.0f;
        FVector4 ptWorld4 = invViewProj.TransformFVector4(ptNdc);
        worldCorners[i] = ptWorld4.xyz() / ptWorld4.w;
    }

    // 3. 전체 카메라 프러스텀의 깊이는 cameraNear ~ cameraFar입니다.
    //    슬라이스의 near/far 비율을 계산합니다.
    float nearFrac = (sliceNear - cameraNear) / (cameraFar - cameraNear);
    float farFrac = (sliceFar - cameraNear) / (cameraFar - cameraNear);

    // 4. 각 전체 프러스텀 코너의 선분(near와 far 코너 사이)을 선형 보간하여 슬라이스의 코너를 계산합니다.
    //    - 먼저, 전체 프러스텀의 네 개의 near 코너 (인덱스 0~3)와 far 코너 (인덱스 4~7)의 선분을 이용합니다.
    outCorners.Empty();
    outCorners.Reserve(8);
    // 슬라이스의 near 평면 코너 (인덱스 0~3)
    for (int i = 0; i < 4; ++i)
    {
        FVector interpNear = worldCorners[i] + (worldCorners[i + 4] - worldCorners[i]) * nearFrac;
        outCorners.Add(interpNear);
    }
    // 슬라이스의 far 평면 코너 (인덱스 4~7)
    for (int i = 0; i < 4; ++i)
    {
        FVector interpFar = worldCorners[i] + (worldCorners[i + 4] - worldCorners[i]) * farFrac;
        outCorners.Add(interpFar);
    }
}

FVector JungleMath::IntersectThreePlanes(const FPlane& p1, const FPlane& p2, const FPlane& p3)
{
    FVector n1(p1.A, p1.B, p1.C);
    FVector n2(p2.A, p2.B, p2.C);
    FVector n3(p3.A, p3.B, p3.C);

    float denom = n1.Dot(n2.Cross(n3));
    if (fabs(denom) < 1e-6f) return FVector(0, 0, 0); // 평면이 평행하거나 잘못된 경우

    FVector result =
        (n2.Cross(n3) * -p1.D -
            n3.Cross(n1) * -p2.D -
            n1.Cross(n2) * -p3.D) / denom;
    return result;
}

void JungleMath::ComputeDirLightVP(const FVector& InLightDir, const FMatrix& InCamView, const FMatrix& InCamProj, const float InCascadeNear,
    const float InCascadeFar, const float InCameraNear, const float InCameraFar, FMatrix& OutLightView, FMatrix& OutLightProj)
{
    // 1) 카메라 프러스텀 슬라이스 코너 World Space
    TArray<FVector> corners;
    GetFrustumCornersWS(InCamView, InCamProj, InCascadeNear, InCascadeFar, InCameraNear, InCameraFar, corners);

    // 2) 슬라이스 중심
    FVector center(0,0,0);
    for (auto& c : corners) center += c;
    center = center / corners.Num();

    // 3) Light View 계산 (up은 Y축)
    const FVector eye = center - InLightDir.Normalize() * 1000.0f;
    OutLightView = CreateViewMatrix(eye, center, FVector(0,0,1));

    // 4) Light Space에서 AABB 구하기
    FVector mins( FLT_MAX,  FLT_MAX,  FLT_MAX );
    FVector maxs(-FLT_MAX, -FLT_MAX, -FLT_MAX );
    for (auto& c : corners)
    {
        FVector ls = OutLightView.TransformPosition(c);
        mins.x = FMath::Min(mins.x, ls.x);
        mins.y = FMath::Min(mins.y, ls.y);
        mins.z = FMath::Min(mins.z, ls.z);
        maxs.x = FMath::Max(maxs.x, ls.x);
        maxs.y = FMath::Max(maxs.y, ls.y);
        maxs.z = FMath::Max(maxs.z, ls.z);
    }

    // 5) 직교 투영: 중심을 0,0으로 맞추기 위해 width/height는 extents*2
    float width  = (maxs.x - mins.x);
    float height = (maxs.y - mins.y);
    float nearZ  = mins.z;
    float farZ   = maxs.z;

    width = FMath::Max(width, 10.0f);
    height = FMath::Max(height, 10.0f);

    // JungleMath의 대칭 오쏘 투영 (가로/세로 전체 크기, near, far)
    OutLightProj = CreateOrthoProjectionMatrix(width, height, nearZ, farZ);
}

FVector JungleMath::FVectorRotate(FVector& origin, const FVector& rotation)
{
    FQuat quaternion = JungleMath::EulerToQuaternion(rotation);
    // 쿼터니언을 이용해 벡터 회전 적용
    return quaternion.RotateVector(origin);
}
FQuat JungleMath::EulerToQuaternion(const FVector& eulerDegrees)
{
    float yaw = DegToRad(eulerDegrees.z);   // Z축 Yaw
    float pitch = DegToRad(eulerDegrees.y); // Y축 Pitch
    float roll = DegToRad(eulerDegrees.x);  // X축 Roll

    float halfYaw = yaw * 0.5f;
    float halfPitch = pitch * 0.5f;
    float halfRoll = roll * 0.5f;

    float cosYaw = cos(halfYaw);
    float sinYaw = sin(halfYaw);
    float cosPitch = cos(halfPitch);
    float sinPitch = sin(halfPitch);
    float cosRoll = cos(halfRoll);
    float sinRoll = sin(halfRoll);

    FQuat quat;
    quat.w = cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll;
    quat.x = cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll;
    quat.y = cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll;
    quat.z = sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll;

    return quat.Normalize();
}
FVector JungleMath::QuaternionToEuler(const FQuat& quat)
{
    FVector euler;

    // 쿼터니언 정규화
    FQuat q = quat;
    FQuat normalizedQ = q.Normalize();

    // Yaw (Z 축 회전)
    float sinYaw = 2.0f * (normalizedQ.w * normalizedQ.z + normalizedQ.x * normalizedQ.y);
    float cosYaw = 1.0f - 2.0f * (normalizedQ.y * normalizedQ.y + normalizedQ.z * normalizedQ.z);
    euler.z = RadToDeg(atan2(sinYaw, cosYaw));

    // Pitch (Y 축 회전, 짐벌락 방지)
    float sinPitch = 2.0f * (normalizedQ.w * normalizedQ.y - normalizedQ.z * normalizedQ.x);
    if (fabs(sinPitch) >= 1.0f)
    {
        euler.y = RadToDeg(static_cast<float>(copysign(PI / 2, sinPitch))); // 🔥 Gimbal Lock 방지
    }
    else
    {
        euler.y = RadToDeg(asin(sinPitch));
    }

    // Roll (X 축 회전)
    float sinRoll = 2.0f * (normalizedQ.w * normalizedQ.x + normalizedQ.y * normalizedQ.z);
    float cosRoll = 1.0f - 2.0f * (normalizedQ.x * normalizedQ.x + normalizedQ.y * normalizedQ.y);
    euler.x = RadToDeg(atan2(sinRoll, cosRoll));
    return euler;
}
FVector JungleMath::FVectorRotate(FVector& origin, const FQuat& rotation)
{
    return rotation.RotateVector(origin);
}

FMatrix JungleMath::CreateRotationMatrix(FVector rotation)
{
    XMVECTOR quatX = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), DegToRad(rotation.x));
    XMVECTOR quatY = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), DegToRad(rotation.y));
    XMVECTOR quatZ = XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), DegToRad(rotation.z));

    XMVECTOR rotationQuat = XMQuaternionMultiply(quatZ, XMQuaternionMultiply(quatY, quatX));
    rotationQuat = XMQuaternionNormalize(rotationQuat);  // 정규화 필수

    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotationQuat);
    FMatrix result = FMatrix::Identity;  // 기본값 설정 (단위 행렬)

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.M[i][j] = rotationMatrix.r[i].m128_f32[j];  // XMMATRIX에서 FMatrix로 값 복사
        }
    }
    return result;
}


float JungleMath::RadToDeg(float radian)
{
    return static_cast<float>(radian * (180.0f / PI));
}

float JungleMath::DegToRad(float degree)
{
    return static_cast<float>(degree * (PI / 180.0f));
}
