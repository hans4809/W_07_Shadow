#pragma once
#include "Shader.h"

class FGeometryShader : public FShader
{
public:
    FGeometryShader(FName InShaderName, const FString& InFullPath, ID3D11GeometryShader* InGS, ID3DBlob* InShaderBlob, D3D_SHADER_MACRO* InShaderMacro,
                  std::filesystem::file_time_type InWriteTime);
    void Bind() override;
    void Release() override;

    ID3D11GeometryShader* GetGeometryShader() const { return GS; }
    void UpdateShader() override;

private:
    ID3D11GeometryShader* GS;
};

