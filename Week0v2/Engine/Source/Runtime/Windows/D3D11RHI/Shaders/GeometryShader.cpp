#include "GeometryShader.h"

#include "D3D11RHI/GraphicDevice.h"

FGeometryShader::FGeometryShader(FName InShaderName, const FString& InFullPath, ID3D11GeometryShader* InGS, ID3DBlob* InShaderBlob,
                                 D3D_SHADER_MACRO* InShaderMacro, std::filesystem::file_time_type InWriteTime)
{
    ShaderName = InShaderName;
    FullPath = InFullPath;
    GS = InGS;
    ShaderBlob = InShaderBlob;
    ShaderMacro = InShaderMacro;
    LastWriteTime = InWriteTime;
}

void FGeometryShader::Bind()
{
}

void FGeometryShader::Release()
{
    SAFE_RELEASE(GS);
    FShader::Release();
}

void FGeometryShader::UpdateShader()
{
}
