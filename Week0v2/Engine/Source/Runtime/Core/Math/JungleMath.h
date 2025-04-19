#pragma once
#include "Define.h"
#include "Quat.h"

class JungleMath
{
public:
    static FVector4 ConvertV3ToV4(FVector vec3);
    static FMatrix CreateModelMatrix(FVector translation, FVector rotation, FVector scale);
    static FMatrix CreateModelMatrix(FVector translation, FQuat rotation, FVector scale);
    static FMatrix CreateViewMatrix(FVector eye, FVector target, FVector up);
    static FMatrix CreateProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane);
    static FMatrix CreateOrthoProjectionMatrix(float width, float height, float nearPlane, float farPlane);
    // 카메라 프러스텀 슬라이스의 8개 코너를 월드 공간에서 계산
    static void GetFrustumCornersWS(const FMatrix& camProj, const FMatrix& camView, float zNear, float zFar, TArray<FVector>& outCorners);

    static void ComputeDirLightVP(const FVector& InLightDir, const FMatrix& InCamView, const FMatrix& InCamProj, float InCascadeNear, float InCascadeFar, FMatrix& OutLightView, FMatrix& OutLightProj);
    
    static FVector FVectorRotate(FVector& origin, const FVector& rotation);
    static FVector FVectorRotate(FVector& origin, const FQuat& rotation);
    static FMatrix CreateRotationMatrix(FVector rotation);
    static float   RadToDeg(float radian);
    static float DegToRad(float degree);
    static FQuat EulerToQuaternion(const FVector& eulerDegrees);
    static FVector QuaternionToEuler(const FQuat& quat);
};

