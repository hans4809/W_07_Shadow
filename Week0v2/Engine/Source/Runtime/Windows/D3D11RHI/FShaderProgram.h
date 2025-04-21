#pragma once
#include "UObject/NameTypes.h"

class ID3D11InputLayout;

class FShaderProgram
{
public:
    FShaderProgram(const FName InVSName, const FName InPSName, const FName InCSName, const FName InGSName, const FName InInputLayoutName)
        : VSName(InVSName), PSName(InPSName), CSName(InCSName), GSName(InGSName), InputLayoutName(InInputLayoutName)
    {}

    FShaderProgram() = default;

    // 셰이더 및 입력 레이아웃 바인딩 함수
    void Bind() const;

    void Release();

    FName GetVSName() const { return VSName; }
    FName GetPSName() const { return PSName; }
    FName GetCSName() const { return CSName; }
    FName GetGSName() const { return GSName; }
    FName GetInputLayoutName() const { return InputLayoutName; }
    
    void SetVSName(const FName& InName) { VSName = InName;}
    void SetPSName(const FName& InName) { PSName = InName;}
    void SetCSName(const FName& InName) { CSName = InName;}
    void SetGSName(const FName& InName) { GSName = InName;}
    void SetInputLayoutName(const FName& InName) { InputLayoutName = InName; }
private:
    FName VSName;
    FName PSName;
    FName CSName;
    FName GSName;
    
    FName InputLayoutName;
};
