#include "ComputeShader.h"

#include "D3D11RHI/GraphicDevice.h"

FComputeShader::FComputeShader(FName InShaderName, const FString& InFullPath, ID3D11ComputeShader* InCS, ID3DBlob* InShaderBlob,
                               D3D_SHADER_MACRO* InShaderMacro, std::filesystem::file_time_type InWriteTime)
{
    ShaderName = InShaderName;
    FullPath = InFullPath;
    CS = InCS;
    ShaderBlob = InShaderBlob;
    ShaderMacro = InShaderMacro;
    LastWriteTime = InWriteTime;
}

void FComputeShader::Bind()
{
}

void FComputeShader::Release()
{
    SAFE_RELEASE(CS);
    FShader::Release();
}

void FComputeShader::UpdateShader()
{
}
