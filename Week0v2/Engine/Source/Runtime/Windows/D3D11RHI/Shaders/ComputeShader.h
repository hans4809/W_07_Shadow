#pragma once
#include "Shader.h"

class FComputeShader : public FShader
{
public:
    FComputeShader(FName InShaderName, const FString& InFullPath, ID3D11ComputeShader* InCS, ID3DBlob* InShaderBlob, D3D_SHADER_MACRO* InShaderMacro,
                  std::filesystem::file_time_type InWriteTime);
    void Bind() override;
    void Release() override;

    ID3D11ComputeShader* GetComputeShader() const { return CS; }
    void UpdateShader() override;

private:
    ID3D11ComputeShader* CS;
};

