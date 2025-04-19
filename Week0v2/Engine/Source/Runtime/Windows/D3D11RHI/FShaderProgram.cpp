#include "FShaderProgram.h"

#include "EditorEngine.h"
#include "Renderer/Renderer.h"

extern UEditorEngine* GEngine;

void FShaderProgram::Bind() const
{
    const FGraphicsDevice GraphicDevice = GEngine->graphicDevice;
    FRenderResourceManager* RenderResourceManager = GEngine->renderer.GetResourceManager();
    
    ID3D11VertexShader* VertexShader = RenderResourceManager->GetVertexShader(VSName);
    ID3D11GeometryShader* GeometryShader = RenderResourceManager->GetGeometryShader(GSName);
    ID3D11PixelShader* PixelShader = RenderResourceManager->GetPixelShader(PSName);
    ID3D11ComputeShader* ComputeShader = RenderResourceManager->GetComputeShader(CSName);
    ID3D11InputLayout* InputLayout = RenderResourceManager->GetInputLayout(InputLayoutName);

    if (VertexShader)
    {
        GraphicDevice.DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    }
    else
    {
        GraphicDevice.DeviceContext->VSSetShader(nullptr, nullptr, 0);
    }

    if (GeometryShader)
    {
        GraphicDevice.DeviceContext->GSSetShader(GeometryShader, nullptr, 0);
    }
    else
    {
        GraphicDevice.DeviceContext->GSSetShader(nullptr, nullptr, 0);
    }

    if (PixelShader)
    {
        GraphicDevice.DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    }
    else
    {
        GraphicDevice.DeviceContext->PSSetShader(nullptr, nullptr, 0);
    }

    if (ComputeShader)
    {
        GraphicDevice.DeviceContext->CSSetShader(ComputeShader, nullptr, 0);
    }
    else
    {
        GraphicDevice.DeviceContext->CSSetShader(nullptr, nullptr, 0);
    }

    if (InputLayout)
    {
        GraphicDevice.DeviceContext->IASetInputLayout(InputLayout);
    }
    else
    {
        GraphicDevice.DeviceContext->IASetInputLayout(nullptr);
    }
}

void FShaderProgram::Release()
{
}
