#pragma once

cbuffer FCameraConstant : register(b3)
{
    row_major matrix ViewMatrix;
    row_major matrix ProjMatrix;
    row_major matrix ViewProjMatrix;

    float3 CameraPos;
    float NearPlane;
    float3 CameraForward;
    float FarPlane;
};
