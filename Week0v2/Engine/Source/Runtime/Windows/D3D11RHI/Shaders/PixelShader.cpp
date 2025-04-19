#include "PixelShader.h"

#include "EditorEngine.h"

FPixelShader::FPixelShader(const FName InShaderName, const FString& InFullPath, ID3D11PixelShader* InPS, ID3DBlob* InShaderBlob,
                           D3D_SHADER_MACRO* InShaderMacro,
                           const std::filesystem::file_time_type InWriteTime)
{
    ShaderName = InShaderName;
    FullPath = InFullPath;
    PS = InPS;
    ShaderBlob = InShaderBlob;
    ShaderMacro = InShaderMacro;
    LastWriteTime = InWriteTime;
}

void FPixelShader::Bind()
{
    const FGraphicsDevice GraphicDevice = GEngine->graphicDevice;
    FRenderResourceManager* RenderResourceManager = GEngine->renderer.GetResourceManager();
    
    ID3D11VertexShader* PixelShader = RenderResourceManager->GetVertexShader(ShaderName);

    
}

void FPixelShader::Release()
{
    SAFE_RELEASE(PS);
    FShader::Release();
}

void FPixelShader::UpdateShader()
{
    const FGraphicsDevice GraphicDevice = GEngine->graphicDevice;
    FRenderer Renderer = GEngine->renderer;
    FRenderResourceManager* RenderResourceManager = GEngine->renderer.GetResourceManager();
    
    RenderResourceManager->UpdatePixelShader(ShaderName.ToString(), FullPath, ShaderMacro);
    
    const std::filesystem::file_time_type CurrentLastWriteTime = std::filesystem::last_write_time(*FullPath);
    LastWriteTime = CurrentLastWriteTime;
}
