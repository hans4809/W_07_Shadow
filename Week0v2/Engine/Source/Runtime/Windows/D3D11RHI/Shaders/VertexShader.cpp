#include "VertexShader.h"

#include "EditorEngine.h"

FVertexShader::FVertexShader(const FName InShaderName, const FString& InFullPath, ID3D11VertexShader* InVs, ID3DBlob* InShaderBlob,
                             D3D_SHADER_MACRO* InShaderMacro, const std::filesystem::file_time_type InWriteTime)
{
    ShaderName = InShaderName;
    FullPath = InFullPath;
    VS = InVs;
    ShaderBlob = InShaderBlob;
    ShaderMacro = InShaderMacro;
    LastWriteTime = InWriteTime;
}

void FVertexShader::Bind()
{
}

void FVertexShader::Release()
{
    SAFE_RELEASE(VS);
    FShader::Release();
}

void FVertexShader::UpdateShader()
{
    const FGraphicsDevice GraphicDevice = GEngine->graphicDevice;
    FRenderer Renderer = GEngine->renderer;
    FRenderResourceManager* RenderResourceManager = GEngine->renderer.GetResourceManager();
    
    RenderResourceManager->UpdateVertexShader(ShaderName.ToString(), FullPath, ShaderMacro);
    
    const std::filesystem::file_time_type CurrentLastWriteTime = std::filesystem::last_write_time(*FullPath);
    LastWriteTime = CurrentLastWriteTime;
}
