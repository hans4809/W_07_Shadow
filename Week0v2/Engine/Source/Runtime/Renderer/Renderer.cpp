#include "Renderer.h"
#include <d3dcompiler.h>

#include "LightManager.h"
#include "VBIBTopologyMapping.h"
#include "ComputeShader/ComputeTileLightCulling.h"
#include "Engine/World.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Launch/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/PrimitiveBatch.h"
#include "PropertyEditor/ShowFlags.h"
#include "UObject/UObjectIterator.h"
#include "D3D11RHI/FShaderProgram.h"
#include "RenderPass/EditorIconRenderPass.h"
#include "RenderPass/GizmoRenderPass.h"
#include "RenderPass/LineBatchRenderPass.h"
#include "RenderPass/StaticMeshRenderPass.h"
#include "RenderPass/ShadowMapRenderPass/DirectionalShadowMapRenderPass.h"
#include "RenderPass/ShadowMapRenderPass/ShadowMapRenderPass.h"

D3D_SHADER_MACRO FRenderer::GouradDefines[] =
{
    {"LIGHTING_MODEL_GOURAUD", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::LambertDefines[] = 
{
    {"LIGHTING_MODEL_LAMBERT", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::EditorGizmoDefines[] = 
{
    {"RENDER_GIZMO", "1"},
    {nullptr, nullptr}
};

D3D_SHADER_MACRO FRenderer::EditorIconDefines[] = 
{
    {"RENDER_ICON", "1"},
    {nullptr, nullptr}
};
D3D_SHADER_MACRO FRenderer::DirectionalDefines[] =
{
    {"DIRECTIONAL_LIGHT", "1"},
    {nullptr, nullptr}
};

void FRenderer::Initialize(FGraphicsDevice* graphics)
{
    Graphics = graphics;
    LightManager = new FLightManager();
    RenderResourceManager = new FRenderResourceManager(graphics);
    RenderResourceManager->Initialize();

    //CreateComputeShader();
    CreateComputeShader(TEXT("TileLightCulling"), nullptr);
    ComputeTileLightCulling = std::make_shared<FComputeTileLightCulling>(TEXT("TileLightCulling"));
    
    D3D_SHADER_MACRO defines[] = 
    {
        {"LIGHTING_MODEL_GOURAUD", "1"},
        {nullptr, nullptr}
    };
    //SetViewMode(VMI_Lit_Phong);
    
    CreateVertexPixelShader(TEXT("UberLit"), GouradDefines);
    FString GouradShaderName = TEXT("UberLit");
    GouradShaderName += GouradDefines->Name;
    GoroudRenderPass = std::make_shared<FStaticMeshRenderPass>(GouradShaderName);

    CreateVertexPixelShader(TEXT("UberLit"), LambertDefines);
    FString LamberShaderName = TEXT("UberLit");
    LamberShaderName += LambertDefines->Name;
    LambertRenderPass = std::make_shared<FStaticMeshRenderPass>(LamberShaderName);
    
    CreateVertexPixelShader(TEXT("UberLit"), nullptr);
    FString PhongShaderName = TEXT("UberLit");
    PhongRenderPass = std::make_shared<FStaticMeshRenderPass>(PhongShaderName);
    
    CreateVertexPixelShader(TEXT("Line"), nullptr);
    LineBatchRenderPass = std::make_shared<FLineBatchRenderPass>(TEXT("Line"));

    CreateVertexPixelShader(TEXT("DebugDepth"), nullptr);
    DebugDepthRenderPass = std::make_shared<FDebugDepthRenderPass>(TEXT("DebugDepth"));
    
    FString GizmoShaderName = TEXT("Editor");
    GizmoShaderName += EditorGizmoDefines->Name;
    CreateVertexPixelShader(TEXT("Editor"), EditorGizmoDefines);
    GizmoRenderPass = std::make_shared<FGizmoRenderPass>(GizmoShaderName);
    
    FString IconShaderName = TEXT("Editor");
    IconShaderName += EditorIconDefines->Name;
    CreateVertexPixelShader(TEXT("Editor"), EditorIconDefines);
    EditorIconRenderPass = std::make_shared<FEditorIconRenderPass>(IconShaderName);

    CreateVertexPixelShader(TEXT("HeightFog"), nullptr);
    FogRenderPass = std::make_shared<FFogRenderPass>(TEXT("HeightFog"));
    
    FString DirShadowMapName = TEXT("ShadowMap");
    DirShadowMapName += DirectionalDefines->Name;
    CreateVertexPixelShader(TEXT("ShadowMap"), DirectionalDefines);
    CreateGeometryShader(TEXT("ShadowMap"), DirectionalDefines);
    DirectionalShadowMapRenderPass = std::make_shared<FDirectionalShadowMapRenderPass>(DirShadowMapName);
}

void FRenderer::PrepareShader(const FName InShaderName)
{
    ShaderPrograms[InShaderName]->Bind();

    BindConstantBuffers(InShaderName);
}

void FRenderer::BindConstantBuffers(const FName InShaderName)
{
    TMap<FShaderConstantKey, uint32> curShaderBindedConstant = ShaderConstantNameAndSlots[InShaderName];
    for (const auto item : curShaderBindedConstant)
    {
        auto curConstantBuffer = RenderResourceManager->GetConstantBuffer(item.Key.ConstantName);
        if (item.Key.ShaderType == EShaderStage::VS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->VSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
        else if (item.Key.ShaderType == EShaderStage::PS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->PSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
        else if (item.Key.ShaderType == EShaderStage::GS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->PSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
        else if (item.Key.ShaderType == EShaderStage::CS)
        {
            if (curConstantBuffer)
                Graphics->DeviceContext->PSSetConstantBuffers(item.Value, 1, &curConstantBuffer);
        }
    }
}

void FRenderer::CreateMappedCB(TMap<FShaderConstantKey, uint32>& ShaderStageToCB, const TArray<FConstantBufferInfo>& CBArray, const EShaderStage Stage) const
{
    for (const FConstantBufferInfo& item : CBArray)
    {
        ShaderStageToCB[{Stage, item.Name}] = item.BindSlot;
        if (RenderResourceManager->GetConstantBuffer(item.Name) == nullptr)
        {
            ID3D11Buffer* ConstantBuffer = RenderResourceManager->CreateConstantBuffer(item.ByteWidth);
            RenderResourceManager->AddOrSetConstantBuffer(item.Name, ConstantBuffer);
        }
    }
}

void FRenderer::Release()
{
    RenderResourceManager->ReleaseResources();
    
    delete RenderResourceManager;
    RenderResourceManager = nullptr;
    
    for (const auto item : ShaderPrograms)
    {
        item.Value->Release();
    }
}

void FRenderer::CreateVertexPixelShader(const FString& InPrefix, D3D_SHADER_MACRO* pDefines)
{
    FString Prefix = InPrefix;
    if (pDefines != nullptr)
    {
#if USE_WIDECHAR
        Prefix += ConvertAnsiToWchar(pDefines->Name);
#else
        Prefix += pDefines->Name;
#endif
    }
    // 접미사를 각각 붙여서 전체 파일명 생성
    const FString VertexShaderFile = InPrefix + TEXT("VertexShader.hlsl");
    const FString PixelShaderFile  = InPrefix + TEXT("PixelShader.hlsl");

    const FString VertexShaderName = Prefix+ TEXT("VertexShader.hlsl");
    const FString PixelShaderName = Prefix+ TEXT("PixelShader.hlsl");
    
    RenderResourceManager->CreateVertexShader(VertexShaderName, VertexShaderFile, pDefines);
    RenderResourceManager->CreatePixelShader(PixelShaderName, PixelShaderFile, pDefines);

    ID3DBlob* VertexShaderBlob = RenderResourceManager->GetVertexShaderBlob(VertexShaderName);
    
    TArray<FConstantBufferInfo> VertexConstantInfos;
    ID3D11InputLayout* InputLayout = nullptr;
    Graphics->ExtractVertexShaderInfo(VertexShaderBlob, VertexConstantInfos, InputLayout);
    RenderResourceManager->AddOrSetInputLayout(VertexShaderName, InputLayout);

    ID3DBlob* PixelShaderBlob = RenderResourceManager->GetPixelShaderBlob(PixelShaderName);
    TArray<FConstantBufferInfo> PixelConstantInfos;
    Graphics->ExtractShaderConstantInfo(PixelShaderBlob, PixelConstantInfos);
    
    TMap<FShaderConstantKey, uint32> ShaderStageToCB;

    CreateMappedCB(ShaderStageToCB, VertexConstantInfos, EShaderStage::VS);  
    CreateMappedCB(ShaderStageToCB, PixelConstantInfos, EShaderStage::PS);
    
    MappingVSPSInputLayout(Prefix, VertexShaderName, PixelShaderName, VertexShaderName);
    MappingVSPSCBSlot(Prefix, ShaderStageToCB);
}

void FRenderer::CreateComputeShader(const FString& InPrefix, D3D_SHADER_MACRO* pDefines)
{
    FString Prefix = InPrefix;
    if (pDefines != nullptr)
    {
#if USE_WIDECHAR
        Prefix += ConvertAnsiToWchar(pDefines->Name);
#else
        Prefix += pDefines->Name;
#endif
    }
    // 접미사를 각각 붙여서 전체 파일명 생성
    const FString ComputeShaderFile = InPrefix + TEXT("ComputeShader.hlsl");
    const FString ComputeShaderName = Prefix + TEXT("ComputeShader.hlsl");
    RenderResourceManager->CreateComputeShader(ComputeShaderName, ComputeShaderFile, pDefines);

    ID3DBlob* ComputeShaderBlob = RenderResourceManager->GetComputeShaderBlob(ComputeShaderName);
    TArray<FConstantBufferInfo> ComputeConstantInfos;
    Graphics->ExtractShaderConstantInfo(ComputeShaderBlob, ComputeConstantInfos);
    
    TMap<FShaderConstantKey, uint32> ShaderStageToCB;

    CreateMappedCB(ShaderStageToCB, ComputeConstantInfos, EShaderStage::CS);  
}

void FRenderer::CreateGeometryShader(const FString& InPrefix, D3D_SHADER_MACRO* pDefines)
{
    FString Prefix = InPrefix;
    if (pDefines != nullptr)
    {
#if USE_WIDECHAR
        Prefix += ConvertAnsiToWchar(pDefines->Name);
#else
        Prefix += pDefines->Name;
#endif
    }
    // 접미사를 각각 붙여서 전체 파일명 생성
    const FString GeometryShaderFile = InPrefix + TEXT("GeometryShader.hlsl");
    const FString GeometryShaderName = Prefix + TEXT("GeometryShader.hlsl");
    RenderResourceManager->CreateGeometryShader(GeometryShaderName, GeometryShaderFile, pDefines);

    ID3DBlob* ComputeShaderBlob = RenderResourceManager->GetGeometryShaderBlob(GeometryShaderName);
    TArray<FConstantBufferInfo> GeometryConstantInfos;
    Graphics->ExtractShaderConstantInfo(ComputeShaderBlob, GeometryConstantInfos);
    
    TMap<FShaderConstantKey, uint32> ShaderStageToCB;

    CreateMappedCB(ShaderStageToCB, GeometryConstantInfos, EShaderStage::GS);  
}

#pragma region Shader

// void FRenderer::CreateComputeShader()
// {
//     ID3DBlob* CSBlob_LightCulling = nullptr;
//     
//     ID3D11ComputeShader* ComputeShader = RenderResourceManager->GetComputeShader(TEXT("TileLightCulling"));
//     
//     if (ComputeShader == nullptr)
//     {
//         Graphics->CreateComputeShader(TEXT("C:\\Users\\Jungle\\Desktop\\Github\\W_07_Shadow\\Week0v2\\Shaders\\TileLightCulling.compute"), nullptr, &CSBlob_LightCulling, &ComputeShader);
//     }
//     else
//     {
//         FGraphicsDevice::CompileComputeShader(TEXT("C:\\Users\\Jungle\\Desktop\\Github\\W_07_Shadow\\Week0v2\\Shaders\\TileLightCulling.compute"), nullptr,  &CSBlob_LightCulling);
//     }
//     RenderResourceManager->AddOrSetComputeShader(TEXT("TileLightCulling"), ComputeShader);
//     
//     TArray<FConstantBufferInfo> LightCullingComputeConstant;
//     Graphics->ExtractPixelShaderInfo(CSBlob_LightCulling, LightCullingComputeConstant);
//     
//     TMap<FShaderConstantKey, uint32> ShaderStageToCB;
//
//     for (const FConstantBufferInfo item : LightCullingComputeConstant)
//     {
//         ShaderStageToCB[{EShaderStage::CS, item.Name}] = item.BindSlot;
//         if (RenderResourceManager->GetConstantBuffer(item.Name) == nullptr)
//         {
//             ID3D11Buffer* ConstantBuffer = RenderResourceManager->CreateConstantBuffer(item.ByteWidth);
//             RenderResourceManager->AddOrSetConstantBuffer(item.Name, ConstantBuffer);
//         }
//     }
//
//     MappingVSPSCBSlot(TEXT("TileLightCulling"), ShaderStageToCB);
//     
//     ComputeTileLightCulling = std::make_shared<FComputeTileLightCulling>(TEXT("TileLightCulling"));
//
//     SAFE_RELEASE(CSBlob_LightCulling)
// }

void FRenderer::Render(UWorld* World, const std::shared_ptr<FEditorViewportClient>& ActiveViewport)
{
    SetViewMode(ActiveViewport->GetViewMode());
    Graphics->DeviceContext->RSSetViewports(1, &ActiveViewport->GetD3DViewport());

    if (ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Wireframe
        && ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Normal
        && ActiveViewport->GetViewMode() != EViewModeIndex::VMI_Depth
        && ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Fog))
    {
        FogRenderPass->PrePrepare(); //fog 렌더 여부 결정 및 준비
    }
    
    ComputeTileLightCulling->Dispatch(ActiveViewport);

    if (LightManager->GetDirectionalLight() != nullptr)
    {
        DirectionalShadowMapRenderPass->Prepare(ActiveViewport);
        DirectionalShadowMapRenderPass->Execute(ActiveViewport);
    }
    
    if (ActiveViewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_Primitives))
    {
        //TODO : FLAG로 나누기
        if (CurrentViewMode  == EViewModeIndex::VMI_Lit_Goroud)
        {
            GoroudRenderPass->Prepare(ActiveViewport);
            GoroudRenderPass->Execute(ActiveViewport);
        }
        else if (CurrentViewMode  == EViewModeIndex::VMI_Lit_Lambert)
        {
            LambertRenderPass->Prepare(ActiveViewport);
            LambertRenderPass->Execute(ActiveViewport);
        }
        else
        {
            PhongRenderPass->Prepare(ActiveViewport);
            PhongRenderPass->Execute(ActiveViewport);
        }
    }

    if (FogRenderPass->ShouldRender())
    {
        FogRenderPass->Prepare(ActiveViewport);
        FogRenderPass->Execute(ActiveViewport);
    }

    LineBatchRenderPass->Prepare(ActiveViewport);
    LineBatchRenderPass->Execute(ActiveViewport);

    if (ActiveViewport->GetViewMode() == EViewModeIndex::VMI_Depth)
    {
        DebugDepthRenderPass->Prepare(ActiveViewport);
        DebugDepthRenderPass->Execute(ActiveViewport);
    }
    
    EditorIconRenderPass->Prepare(ActiveViewport);
    EditorIconRenderPass->Execute(ActiveViewport);

    if (!World->GetSelectedActors().IsEmpty())
    {
        GizmoRenderPass->Prepare(ActiveViewport);
        GizmoRenderPass->Execute(ActiveViewport);
    }
}

void FRenderer::ClearRenderObjects() const
{
    DirectionalShadowMapRenderPass->ClearRenderObjects();
    GoroudRenderPass->ClearRenderObjects();
    LambertRenderPass->ClearRenderObjects();
    PhongRenderPass->ClearRenderObjects();
    LineBatchRenderPass->ClearRenderObjects();
    GizmoRenderPass->ClearRenderObjects();
    DebugDepthRenderPass->ClearRenderObjects();
    EditorIconRenderPass->ClearRenderObjects();
}

void FRenderer::SetViewMode(const EViewModeIndex evi)
{
    switch (evi)
    {
    case EViewModeIndex::VMI_Lit_Goroud:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Goroud;
        //TODO : Light 받는 거
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Lambert;
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Lit_Phong:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Phong;
        bIsLit = true;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Wireframe:
        CurrentRasterizerState = ERasterizerState::WireFrame;
        CurrentViewMode = VMI_Wireframe;
        bIsLit = false;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Unlit:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Unlit;
        //TODO : Light 안 받는 거
        bIsLit = false;
        bIsNormal = false;
        break;
    case EViewModeIndex::VMI_Depth:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Depth;
        break;
    case EViewModeIndex::VMI_Normal:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Normal;
        bIsLit = false;
        bIsNormal = true;
        break;
    default:
        CurrentRasterizerState = ERasterizerState::SolidBack;
        CurrentViewMode = VMI_Lit_Phong;
        bIsLit = true;
        bIsNormal = false;
        break;
    }
}

void FRenderer::AddRenderObjectsToRenderPass(UWorld* InWorld, const std::shared_ptr<FEditorViewportClient>& ActiveViewport) const
{
    //값을 써줄때 
    LightManager->CollectLights(InWorld);
    FMatrix View = ActiveViewport->GetViewMatrix();
    FMatrix Proj = ActiveViewport->GetProjectionMatrix();
    FFrustum Frustum = FFrustum::ExtractFrustum(View * Proj);
    LightManager->CullLights(Frustum);
    
    ComputeTileLightCulling->AddRenderObjectsToRenderPass(InWorld);

    DirectionalShadowMapRenderPass->AddRenderObjectsToRenderPass(InWorld);

    if (CurrentViewMode == VMI_Lit_Goroud)
    {
        GoroudRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    else if (CurrentViewMode == VMI_Lit_Lambert)
    {
        LambertRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    else
    {
        PhongRenderPass->AddRenderObjectsToRenderPass(InWorld);
    }
    
    GizmoRenderPass->AddRenderObjectsToRenderPass(InWorld);
    EditorIconRenderPass->AddRenderObjectsToRenderPass(InWorld);
}

void FRenderer::MappingVSPSInputLayout(const FName InShaderProgramName, FName VSName, FName PSName, FName InInputLayoutName)
{
    ShaderPrograms.Add(InShaderProgramName, std::make_shared<FShaderProgram>(VSName, PSName, TEXT(""), TEXT(""), InInputLayoutName));
}


void FRenderer::MappingCS(const FName InShaderProgramName, FName InCSName)
{
    ShaderPrograms.Add(InShaderProgramName, std::make_shared<FShaderProgram>(TEXT(""), TEXT(""), InCSName, TEXT(""), TEXT("")));
}

void FRenderer::MappingGS(const FName InShaderProgramName, FName InGS)
{
    if (ShaderPrograms.Contains(InShaderProgramName))
    {
        ShaderPrograms[InShaderProgramName]->SetGSName(InGS);
    }
    else
    {
        ShaderPrograms.Add(InShaderProgramName, std::make_shared<FShaderProgram>(TEXT(""), TEXT(""), TEXT(""), InGS, TEXT("")));
    }
}

void FRenderer::MappingVSPSCBSlot(const FName InShaderName, const TMap<FShaderConstantKey, uint32>& MappedConstants)
{
    ShaderConstantNameAndSlots.Add(InShaderName, MappedConstants);
}

void FRenderer::MappingVBTopology(const FName InObjectName, const FName InVBName, const uint32 InStride, const uint32 InNumVertices, const D3D11_PRIMITIVE_TOPOLOGY InTopology)
{
    if (VBIBTopologyMappings.Contains(InObjectName) == false)
    {
        VBIBTopologyMappings[InObjectName] = std::make_shared<FVBIBTopologyMapping>();
    }
    VBIBTopologyMappings[InObjectName]->MappingVertexBuffer(InVBName, InStride, InNumVertices, InTopology);
}

void FRenderer::MappingIB(const FName InObjectName, const FName InIBName, const uint32 InNumIndices)
{
    if (VBIBTopologyMappings.Contains(InObjectName) == false)
    {
        VBIBTopologyMappings[InObjectName] = std::make_shared<FVBIBTopologyMapping>();
    }
    VBIBTopologyMappings[InObjectName]->MappingIndexBuffer(InIBName, InNumIndices);
}

