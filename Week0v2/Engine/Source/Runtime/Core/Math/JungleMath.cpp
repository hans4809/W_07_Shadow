#include "Engine/Source/Runtime/Core/Math/JungleMath.h"
#include <DirectXMath.h>

#include "MathUtility.h"
#include "D3D11RHI/ShadowMapConfig.h"

using namespace DirectX;
FVector4 JungleMath::ConvertV3ToV4(const FVector vec3)
{
	FVector4 newVec4;
	newVec4.x = vec3.x;
	newVec4.y = vec3.y;
	newVec4.z = vec3.z;
	return newVec4;
}



FMatrix JungleMath::CreateModelMatrix(const FVector translation, const FVector rotation, const FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);

    FMatrix Rotation = FMatrix::CreateRotation(rotation.x, rotation.y, rotation.z);
    //FMatrix Rotation = JungleMath::EulerToQuaternion(rotation).ToMatrix();

    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}

FMatrix JungleMath::CreateModelMatrix(const FVector translation, const FQuat rotation, const FVector scale)
{
    FMatrix Translation = FMatrix::CreateTranslationMatrix(translation);
    FMatrix Rotation = rotation.ToMatrix();
    FMatrix Scale = FMatrix::CreateScale(scale.x, scale.y, scale.z);
    return Scale * Rotation * Translation;
}
FMatrix JungleMath::CreateViewMatrix(const FVector eye, const FVector target, const FVector up)
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

FMatrix JungleMath::CreateProjectionMatrix(const float fov, const float aspect, const float nearPlane, const float farPlane)
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

FMatrix JungleMath::CreateOrthoProjectionMatrix(const float width, const float height, const float nearPlane, const float farPlane)
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

void JungleMath::GetFrustumCornersWS(const FMatrix& camProj, const FMatrix& camView, const float zNear, const float zFar, TArray<FVector>& outCorners)
{
    const FMatrix inv = FMatrix::Inverse(camView * camProj);

    // NDC 공간의 8개 코너 (Z: NDC 기준 -1~1)
    const float ndc[8][3] = {
        {-1,-1,-1}, {+1,-1,-1}, {-1,+1,-1}, {+1,+1,-1},
        {-1,-1,+1}, {+1,-1,+1}, {-1,+1,+1}, {+1,+1,+1},
    };

    outCorners.Empty();
    outCorners.Reserve(8);

    for (int i = 0; i < 8; ++i)
    {
        FVector4 ptNdc(ndc[i][0], ndc[i][1], ndc[i][2], 1.0f);

        // NDC → World
        FVector4 ptWorld4 = inv.TransformFVector4(ptNdc);

        FVector ptWorld = ptWorld4.xyz() / ptWorld4.w;
        outCorners.Add(ptWorld);
    }
}

void JungleMath::ComputeDirLightVP(const FVector& InLightDir, const FMatrix& InCamView, const FMatrix& InCamProj, const float InCascadeNear,
    const float InCascadeFar, FMatrix& OutLightView, FMatrix& OutLightProj)
{
    // 1) 카메라 프러스텀 슬라이스 코너 World Space
    TArray<FVector> corners;
    GetFrustumCornersWS(InCamProj, InCamView, InCascadeNear, InCascadeFar, corners);

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

void JungleMath::ComputeDirLightVP(const float centerX, const float centerY, const float centerZ, const float halfWidth, const float halfHeight,
                                   const float nearZ, const float farZ, const float lightDirX, const float lightDirY, const float lightDirZ, FMatrix& OutView, FMatrix& OutProj)
{
    // 1) 라이트 Forward 축(normalize)
    float fx = lightDirX, fy = lightDirY, fz = lightDirZ;
    const float invL = 1.0f / std::sqrt(fx*fx + fy*fy + fz*fz);
    fx *= invL; fy *= invL; fz *= invL;

    // 2) 라이트 Right 축 = normalize(Up × Forward), Up=(0,0,1)
    //    cross((0,0,1),(fx,fy,fz)) = (-fy, fx, 0)
    float rx = -fy, ry = fx, rz = 0.0f;
    const float invR = 1.0f / std::sqrt(rx*rx + ry*ry + rz*rz);
    rx *= invR; ry *= invR; rz *= invR;

    // 3) 라이트 Up 축 = Forward × Right
    //    cross((fx,fy,fz),(rx,ry,rz))
    const float ux = fy*rz - fz*ry;
    const float uy = fz*rx - fx*rz;
    const float uz = fx*ry - fy*rx;

    // 4) 카메라(라이트) 위치 = center - Forward * halfWidth
    //    (halfWidth을 Frustum X 방향 AABB 반폭으로 사용)
    const float px = centerX - fx * halfWidth;
    const float py = centerY - fy * halfWidth;
    const float pz = centerZ - fz * halfWidth;

    // 5) View 행렬 (row‑major)
    //    [ rx  ry  rz  fx  ]
    //    [ ux  uy  uz  fy ]
    //    [ -dot(R, P) -dot(U, P) -dot(F, P) fz ]
    //    [  0   0   0       1     ]
    OutView.M[0][0] = rx; OutView.M[0][1] = ry; OutView.M[0][2] = rz; OutView.M[0][3] = 0.0f;

    OutView.M[1][0] = ux; OutView.M[1][1] = uy; OutView.M[1][2] = uz; OutView.M[1][3] = 0.0f;

    OutView.M[2][0] = fx; OutView.M[2][1] = fy; OutView.M[2][2] = fz; OutView.M[2][3] = 0.0f;
    
    OutView.M[3][0] = -(rx*px + ry*py + rz*pz);  OutView.M[3][1] = -(ux*px + uy*py + uz*pz); OutView.M[3][2] = -(fx*px + fy*py + fz*pz); OutView.M[3][3] = 1.0f;

    // 6) OrthoLH Projection (row‑major)
    //    l = -halfWidth, r = +halfWidth → width = 2*halfWidth
    //    b = -halfHeight, t = +halfHeight → height = 2*halfHeight
    const float invW = 1.0f / halfWidth;        // = 2/(r-l)
    const float invH = 1.0f / halfHeight;       // = 2/(t-b)
    const float invD = 1.0f / (farZ - nearZ);    // = 1/(f-n)

    //    [ invW   0      0       0   ]
    //    [  0    invH    0       0   ]
    //    [  0     0     invD     0   ]
    //    [  0     0   -n*invD    1   ]
    OutProj.M[0][0] = invW;   OutProj.M[0][1] = 0.0f;  OutProj.M[0][2] = 0.0f;     OutProj.M[0][3] = 0.0f;
    OutProj.M[1][0] = 0.0f;   OutProj.M[1][1] = invH;  OutProj.M[1][2] = 0.0f;     OutProj.M[1][3] = 0.0f;
    OutProj.M[2][0] = 0.0f;   OutProj.M[2][1] = 0.0f;  OutProj.M[2][2] = invD;     OutProj.M[2][3] = 0.0f;
    OutProj.M[3][0] = 0.0f;   OutProj.M[3][1] = 0.0f;  OutProj.M[3][2] = -nearZ * invD; OutProj.M[3][3] = 1.0f;
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

FMatrix JungleMath::CreateRotationMatrix(const FVector rotation)
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


float JungleMath::RadToDeg(const float radian)
{
    return static_cast<float>(radian * (180.0f / PI));
}

float JungleMath::DegToRad(const float degree)
{
    return static_cast<float>(degree * (PI / 180.0f));
}
