#include "GraphicDevice.h"

#include <d3dcompiler.h>
#include <filesystem>

#include "Define.h"

void FGraphicsDevice::Initialize(const HWND hWindow)
{
    CreateDeviceAndSwapChain(hWindow);
    CreateFrameBuffer();
    CreateDepthStencilBuffer(hWindow);
    CreateSceneColorResources();

    //CreateDepthStencilState();
    //CreateDepthStencilSRV();
    //CreateDepthCopyTexture();
    //CreateRasterizerState();
    //CurrentRasterizer = RasterizerStateSOLID;
}

void FGraphicsDevice::CreateDeviceAndSwapChain(HWND hWindow)
{
    // 지원하는 Direct3D 기능 레벨을 정의
    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // 스왑 체인 설정 구조체 초기화
    SwapchainDesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
    SwapchainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
    SwapchainDesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
    SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
    SwapchainDesc.BufferCount = 2; // 더블 버퍼링
    SwapchainDesc.OutputWindow = hWindow; // 렌더링할 창 핸들
    SwapchainDesc.Windowed = TRUE; // 창 모드
    SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

    // 디바이스와 스왑 체인 생성
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
        featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
        &SwapchainDesc, &SwapChain, &Device, nullptr, &DeviceContext);

    if (FAILED(hr))
    {
        MessageBox(hWindow, L"CreateDeviceAndSwapChain failed!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // 스왑 체인 정보 가져오기 (이후에 사용을 위해)
    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;
}

void FGraphicsDevice::CreateDepthStencilBuffer(HWND hWindow)
{
    RECT clientRect;
    GetClientRect(hWindow, &clientRect);
    const uint32 width = clientRect.right - clientRect.left;
    const uint32 height = clientRect.bottom - clientRect.top;

    // 깊이/스텐실 텍스처 생성
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = width; // 텍스처 너비 설정
    descDepth.Height = height; // 텍스처 높이 설정
    descDepth.MipLevels = 1; // 미맵 레벨 수 (1로 설정하여 미맵 없음)
    descDepth.ArraySize = 1; // 텍스처 배열의 크기 (1로 단일 텍스처)
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24비트 깊이와 8비트 스텐실을 위한 포맷, Typeless -> SRV와 DSV 모두 사용 가능
    descDepth.SampleDesc.Count = 1; // 멀티샘플링 설정 (1로 단일 샘플)
    descDepth.SampleDesc.Quality = 0; // 샘플 퀄리티 설정
    descDepth.Usage = D3D11_USAGE_DEFAULT; // 텍스처 사용 방식
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // 깊이 스텐실 뷰로 바인딩 설정
    descDepth.CPUAccessFlags = 0; // CPU 접근 방식 설정
    descDepth.MiscFlags = 0; // 기타 플래그 설정

    HRESULT hr = Device->CreateTexture2D(&descDepth, nullptr, &DepthStencilBuffer);

    if (FAILED(hr))
    {
        MessageBox(hWindow, L"Failed to create depth stencilBuffer!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }


    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(descDSV));
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 깊이 스텐실 포맷
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // 뷰 타입 설정 (2D 텍스처)
    descDSV.Texture2D.MipSlice = 0; // 사용할 미맵 슬라이스 설정

    hr = Device->CreateDepthStencilView(DepthStencilBuffer, // Depth stencil texture
        &descDSV, // Depth stencil desc
        &DepthStencilView);  // [out] Depth stencil view

    if (FAILED(hr))
    {
        wchar_t errorMsg[256];
        swprintf_s(errorMsg, L"Failed to create depth stencil view! HRESULT: 0x%08X", hr);
        MessageBox(hWindow, errorMsg, L"Error", MB_ICONERROR | MB_OK);
        return;
    }
}

bool FGraphicsDevice::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState) const
{
    if (FAILED(Device->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState)))
    {
        return false;
    }

    return true;
}

bool FGraphicsDevice::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState) const
{
    if (FAILED(Device->CreateRasterizerState(pRasterizerDesc, ppRasterizerState)))
    {
        return false;
    }

    return true;
}

bool FGraphicsDevice::CreateBlendState(const D3D11_BLEND_DESC* pBlendState, ID3D11BlendState** ppBlendState) const
{
    if (FAILED(Device->CreateBlendState(pBlendState, ppBlendState)))
    {
        return false;
    }

    return true;
}

void FGraphicsDevice::CreateDepthCopyTexture()
{
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory(&descDepth, sizeof(descDepth));
    descDepth.Width = screenWidth; // 텍스처 너비 설정
    descDepth.Height = screenHeight; // 텍스처 높이 설정
    descDepth.MipLevels = 1; // 미맵 레벨 수 (1로 설정하여 미맵 없음)
    descDepth.ArraySize = 1; // 텍스처 배열의 크기 (1로 단일 텍스처)
    descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS; // 24비트 깊이와 8비트 스텐실을 위한 포맷, Typeless -> SRV와 DSV 모두 사용 가능
    descDepth.SampleDesc.Count = 1; // 멀티샘플링 설정 (1로 단일 샘플)
    descDepth.SampleDesc.Quality = 0; // 샘플 퀄리티 설정
    descDepth.Usage = D3D11_USAGE_DEFAULT; // 텍스처 사용 방식
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // 깊이 스텐실 뷰로 바인딩 설정
    descDepth.CPUAccessFlags = 0; // CPU 접근 방식 설정
    descDepth.MiscFlags = 0; // 기타 플래그 설정

    HRESULT Result = Device->CreateTexture2D(&descDepth, nullptr, &DepthCopyTexture);
    if (FAILED(Result))
    {
        int i = 1;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // 깊이 데이터만 읽기 위한 포맷
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    Result = Device->CreateShaderResourceView(DepthCopyTexture, &srvDesc, &DepthCopySRV);
	if (FAILED(Result))
	{
		int i = 1;
	}
}

void FGraphicsDevice::ReleaseDeviceAndSwapChain()
{
    if (DeviceContext)
    {
        DeviceContext->Flush(); // 남아있는 GPU 명령 실행
    }

    SAFE_RELEASE(SwapChain);
    SAFE_RELEASE(DeviceContext);
    SAFE_RELEASE(Device);
}

void FGraphicsDevice::CreateFrameBuffer()
{
    // 스왑 체인으로부터 백 버퍼 텍스처 가져오기
    SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&FrameBuffer));

    // 렌더 타겟 뷰 생성
    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
    framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
    
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth;
    textureDesc.Height = screenHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Device->CreateTexture2D(&textureDesc, nullptr, &UUIDFrameBuffer);

    D3D11_RENDER_TARGET_VIEW_DESC UUIDFrameBufferRTVDesc = {};
    UUIDFrameBufferRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    UUIDFrameBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    Device->CreateRenderTargetView(UUIDFrameBuffer, &UUIDFrameBufferRTVDesc, &UUIDFrameBufferRTV);

    RTVs[0] = FrameBufferRTV;
    RTVs[1] = UUIDFrameBufferRTV;
}

void FGraphicsDevice::CreateSceneColorResources()
{
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = screenWidth;
    textureDesc.Height = screenHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = Device->CreateTexture2D(&textureDesc, nullptr, &SceneColorBuffer);
    if (FAILED(hr))
    {
        assert(TEXT("SceneColorBuffer creation failed"));
        return;
    }

    D3D11_RENDER_TARGET_VIEW_DESC SceneColorRTVDesc = {};
    SceneColorRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 색상 포맷
    SceneColorRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

    hr = Device->CreateRenderTargetView(SceneColorBuffer, &SceneColorRTVDesc, &SceneColorRTV);
    if (FAILED(hr))
    {
        assert(TEXT("SceneColorBuffer creation failed"));
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = Device->CreateShaderResourceView(SceneColorBuffer, &srvDesc, &SceneColorSRV);
    if (FAILED(hr))
    {
        assert(TEXT("SceneColorBuffer creation failed"));
        return;
    }
}

void FGraphicsDevice::SwitchRTV()
{
    RTVs[0] = SceneColorRTV;
}

void FGraphicsDevice::ReturnRTV()
{
    RTVs[0] = FrameBufferRTV;
}

void FGraphicsDevice::ReleaseFrameBuffer()
{
    SAFE_RELEASE(FrameBufferRTV);
    SAFE_RELEASE(FrameBuffer);

    SAFE_RELEASE(UUIDFrameBufferRTV);
    SAFE_RELEASE(UUIDFrameBuffer);
}

void FGraphicsDevice::ReleaseDepthStencilResources()
{
    SAFE_RELEASE(DepthStencilView);
    SAFE_RELEASE(DepthStencilBuffer);
}

void FGraphicsDevice::ReleaseSceneColorResources()
{
    SAFE_RELEASE(SceneColorBuffer)
    SAFE_RELEASE(SceneColorRTV)
    SAFE_RELEASE(SceneColorSRV)
}

void FGraphicsDevice::Release() 
{
    DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ReleaseFrameBuffer();
    ReleaseDepthStencilResources();
    ReleaseDeviceAndSwapChain();
    ReleaseSceneColorResources();
}

void FGraphicsDevice::SwapBuffer()
{
    SwapChain->Present(0, 0);
}
void FGraphicsDevice::Prepare()
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor); // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    DeviceContext->ClearRenderTargetView(UUIDFrameBufferRTV, ClearColor); // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    DeviceContext->ClearRenderTargetView(SceneColorRTV, ClearColor);
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정

    //DeviceContext->RSSetViewports(1, &ViewportInfo); // GPU가 화면을 렌더링할 영역 설정
    //DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정

    //DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    DeviceContext->OMSetRenderTargets(2, RTVs, DepthStencilView); // 렌더 타겟 설정(백버퍼를 가르킴)
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // 블렌뎅 상태 설정, 기본블렌딩 상태임
}

void FGraphicsDevice::Prepare(D3D11_VIEWPORT* viewport)
{
    DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor); // 렌더 타겟 뷰에 저장된 이전 프레임 데이터를 삭제
    DeviceContext->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 깊이 버퍼 초기화 추가

    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정정 연결 방식 설정

    DeviceContext->RSSetViewports(1, viewport); // GPU가 화면을 렌더링할 영역 설정
    //DeviceContext->RSSetState(CurrentRasterizer); //레스터 라이저 상태 설정

    //DeviceContext->OMSetDepthStencilState(DepthStencilState, 0);
    DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, DepthStencilView); // 렌더 타겟 설정(백버퍼를 가르킴)
    DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // 블렌뎅 상태 설정, 기본블렌딩 상태임
}


void FGraphicsDevice::OnResize(HWND hWindow)
{
    DeviceContext->OMSetRenderTargets(0, RTVs, 0);
    
    SAFE_RELEASE(DepthCopySRV);
    SAFE_RELEASE(DepthCopyTexture);
    SAFE_RELEASE(DepthStencilView);
    
    ReleaseFrameBuffer();

    ReleaseSceneColorResources();

    if (screenWidth == 0 || screenHeight == 0)
    {
        MessageBox(hWindow, L"Invalid width or height for ResizeBuffers!", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    // SwapChain 크기 조정
    const HRESULT hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);  // DXGI_FORMAT_B8G8R8A8_UNORM으로 시도
    if (FAILED(hr))
    {
        MessageBox(hWindow, L"failed", L"ResizeBuffers failed ", MB_ICONERROR | MB_OK);
        return;
    }
    
    SwapChain->GetDesc(&SwapchainDesc);
    screenWidth = SwapchainDesc.BufferDesc.Width;
    screenHeight = SwapchainDesc.BufferDesc.Height;

    CreateFrameBuffer();
    CreateDepthStencilBuffer(hWindow);
    //CreateDepthStencilSRV();
    CreateDepthCopyTexture();
    CreateSceneColorResources();
}

void FGraphicsDevice::BindSampler(EShaderStage stage, uint32 StartSlot, uint32 NumSamplers, ID3D11SamplerState* const* ppSamplers) const
{
    if (EShaderStage::VS == stage)
        DeviceContext->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);

    if (EShaderStage::HS == stage)
        DeviceContext->HSSetSamplers(StartSlot, NumSamplers, ppSamplers);

    if (EShaderStage::DS == stage)
        DeviceContext->DSSetSamplers(StartSlot, NumSamplers, ppSamplers);

    if (EShaderStage::GS == stage)
        DeviceContext->GSSetSamplers(StartSlot, NumSamplers, ppSamplers);

    if (EShaderStage::PS == stage)
        DeviceContext->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);

}

void FGraphicsDevice::BindSamplers(uint32 StartSlot, uint32 NumSamplers, ID3D11SamplerState* const* ppSamplers) const
{
    BindSampler(EShaderStage::VS, StartSlot, NumSamplers, ppSamplers);
    //BindSampler(EShaderStage::HS, StartSlot, NumSamplers, ppSamplers);
    //BindSampler(EShaderStage::DS, StartSlot, NumSamplers, ppSamplers);
    //BindSampler(EShaderStage::GS, StartSlot, NumSamplers, ppSamplers);
    BindSampler(EShaderStage::PS, StartSlot, NumSamplers, ppSamplers);
}

ID3D11ShaderResourceView* FGraphicsDevice::GetCopiedShaderResourceView() const
{
    ID3D11Resource* DepthResource = nullptr;
    DepthStencilView->GetResource(&DepthResource);

    ID3D11ShaderResourceView* DepthSRV = nullptr;
    Device->CreateShaderResourceView(DepthResource, nullptr, &DepthSRV);

    DepthResource->Release();

    return DepthSRV;
}

uint32 FGraphicsDevice::GetPixelUUID(POINT pt)
{
    // pt.x 값 제한하기
    if (pt.x < 0) {
        pt.x = 0;
    }
    else if (pt.x > screenWidth) {
        pt.x = screenWidth;
    }

    // pt.y 값 제한하기
    if (pt.y < 0) {
        pt.y = 0;
    }
    else if (pt.y > screenHeight) {
        pt.y = screenHeight;
    }

    // 1. Staging 텍스처 생성 (1x1 픽셀)
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = 1; // 픽셀 1개만 복사
    stagingDesc.Height = 1;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 원본 텍스처 포맷과 동일
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Texture2D* stagingTexture = nullptr;
    Device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);

    // 2. 복사할 영역 정의 (D3D11_BOX)
    D3D11_BOX srcBox = {};
    srcBox.left = static_cast<UINT>(pt.x);
    srcBox.right = srcBox.left + 1; // 1픽셀 너비
    srcBox.top = static_cast<UINT>(pt.y);
    srcBox.bottom = srcBox.top + 1; // 1픽셀 높이
    srcBox.front = 0;
    srcBox.back = 1;
    FVector4 UUIDColor{ 1, 1, 1, 1 }; 

    if (stagingTexture == nullptr)
        return DecodeUUIDColor(UUIDColor);

    // 3. 특정 좌표만 복사
    DeviceContext->CopySubresourceRegion(
        stagingTexture, // 대상 텍스처
        0,              // 대상 서브리소스
        0, 0, 0,        // 대상 좌표 (x, y, z)
        UUIDFrameBuffer, // 원본 텍스처
        0,              // 원본 서브리소스
        &srcBox         // 복사 영역
    );

    // 4. 데이터 매핑
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    DeviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mapped);

    // 5. 픽셀 데이터 추출 (1x1 텍스처이므로 offset = 0)
    const BYTE* pixelData = static_cast<const BYTE*>(mapped.pData);

    if (pixelData)
    {
        UUIDColor.x = static_cast<float>(pixelData[0]); // R
        UUIDColor.y = static_cast<float>(pixelData[1]); // G
        UUIDColor.z = static_cast<float>(pixelData[2]) ; // B
        UUIDColor.w = static_cast<float>(pixelData[3]); // A
    }

    // 6. 매핑 해제 및 정리
    DeviceContext->Unmap(stagingTexture, 0);
    if (stagingTexture) stagingTexture->Release(); stagingTexture = nullptr;

    return DecodeUUIDColor(UUIDColor);
}

uint32 FGraphicsDevice::DecodeUUIDColor(FVector4 UUIDColor) {
    uint32_t W = static_cast<uint32_t>(UUIDColor.w) << 24;
    uint32_t Z = static_cast<uint32_t>(UUIDColor.z) << 16;
    uint32_t Y = static_cast<uint32_t>(UUIDColor.y) << 8;
    uint32_t X = static_cast<uint32_t>(UUIDColor.x);

    return W | Z | Y | X;
}

bool FGraphicsDevice::CompileVertexShader(const std::filesystem::path& InFilePath, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;
    
    const std::wstring shaderFilePath = InFilePath.wstring();

    const HRESULT hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", shaderFlags, 0, ppCode, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return false;
    }

    return true;
}

bool FGraphicsDevice::CompilePixelShader(const std::filesystem::path& InFilePath, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;
    
    const std::wstring shaderFilePath = InFilePath.wstring();
    const HRESULT hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", shaderFlags, 0, ppCode, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return false;
    }
 
    return true;
}

bool FGraphicsDevice::CompileComputeShader(const FString& InFileName, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode)
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;

    const std::filesystem::path current = std::filesystem::current_path();
    const std::filesystem::path fullpath = current / TEXT("Shaders") / *InFileName;
    const std::wstring shaderFilePath = fullpath.wstring();

    const HRESULT hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainCS", "cs_5_0", shaderFlags, 0, ppCode, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        return false;
    }

    return true;
}

bool FGraphicsDevice::CreateVertexShader(const std::filesystem::path& InFilePath, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode, ID3D11VertexShader** ppVShader) const
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;
    
    const std::wstring shaderFilePath = InFilePath.wstring();
    
    HRESULT hr;
    if (pDefines)
        hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", shaderFlags, 0, ppCode, &errorBlob);
    else
        hr = D3DCompileFromFile(shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainVS", "vs_5_0", shaderFlags, 0, ppCode, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        abort();
    }

    if(Device == nullptr)
        return false;

    if (FAILED(Device->CreateVertexShader((*ppCode)->GetBufferPointer(), (*ppCode)->GetBufferSize(), nullptr, ppVShader)))
        return false;

    return true;
}

bool FGraphicsDevice::CreatePixelShader(const std::filesystem::path& InFilePath, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode, ID3D11PixelShader** ppPS) const
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;
    
    const std::wstring shaderFilePath = InFilePath.wstring();    

    HRESULT hr;
    if (pDefines)
        hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", shaderFlags, 0, ppCode, &errorBlob);
    else
        hr = D3DCompileFromFile(shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainPS", "ps_5_0", shaderFlags, 0, ppCode, &errorBlob);
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        abort();
    }
    
    if (Device == nullptr)
        return false;

    if (FAILED(Device->CreatePixelShader((*ppCode)->GetBufferPointer(), (*ppCode)->GetBufferSize(), nullptr, ppPS)))
        return false;

    return true;
}

bool FGraphicsDevice::CreateComputeShader(const FString& InFileName, const D3D_SHADER_MACRO* pDefines, ID3DBlob** ppCode, ID3D11ComputeShader** ppComputeShader) const
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef  _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;

    ID3DBlob* errorBlob = nullptr;

    const std::filesystem::path current = std::filesystem::current_path();
    const std::filesystem::path fullpath = current / TEXT("Shaders") / *InFileName;
    const std::wstring shaderFilePath = fullpath.wstring();

    const HRESULT hr = D3DCompileFromFile(shaderFilePath.c_str(), pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, "mainCS", "cs_5_0", shaderFlags, 0, ppCode, &errorBlob);
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            // errorBlob에 저장된 메시지를 출력 (디버그 출력이나 콘솔 등)
            OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        abort();
    }
    
    if (Device == nullptr)
        return false;

    if (FAILED(Device->CreateComputeShader((*ppCode)->GetBufferPointer(), (*ppCode)->GetBufferSize(), nullptr, ppComputeShader)))
        return false;

    return true;
}

void FGraphicsDevice::ExtractVertexShaderInfo(ID3DBlob* shaderBlob, TArray<FConstantBufferInfo>& OutCBInfos, ID3D11InputLayout*& OutInputLayout) const
{
    ID3D11ShaderReflection* pReflector = nullptr;
    HRESULT hr = D3DReflect(shaderBlob->GetBufferPointer(),
                            shaderBlob->GetBufferSize(),
                            IID_ID3D11ShaderReflection,
                            reinterpret_cast<void**>(&pReflector));
    
    if (FAILED(hr) || !pReflector)
    {
        return;
    }

    D3D11_SHADER_DESC shaderDesc = {};
    hr = pReflector->GetDesc(&shaderDesc);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pReflector);
        return;
    }

    OutCBInfos = ExtractConstantBufferInfos(pReflector, shaderDesc);
    OutInputLayout = ExtractInputLayout(shaderBlob, pReflector, shaderDesc);
    
    SAFE_RELEASE(pReflector);
}

void FGraphicsDevice::ExtractPixelShaderInfo(ID3DBlob* shaderBlob, TArray<FConstantBufferInfo>& OutCBInfos)
{
    ID3D11ShaderReflection* pReflector = nullptr;
    HRESULT hr = D3DReflect(shaderBlob->GetBufferPointer(),
                            shaderBlob->GetBufferSize(),
                            IID_ID3D11ShaderReflection,
                            reinterpret_cast<void**>(&pReflector));
    
    if (FAILED(hr) || !pReflector)
    {
        return;
    }

    D3D11_SHADER_DESC shaderDesc = {};
    hr = pReflector->GetDesc(&shaderDesc);
    if (FAILED(hr))
    {
        SAFE_RELEASE(pReflector);
        return;
    }

    OutCBInfos = ExtractConstantBufferInfos(pReflector, shaderDesc);
}

TArray<FConstantBufferInfo> FGraphicsDevice::ExtractConstantBufferInfos(ID3D11ShaderReflection* InReflector, const D3D11_SHADER_DESC& InShaderDecs)
{
    TArray<FConstantBufferInfo> CBInfos;
    
    // 모든 상수 버퍼에 대해 이름을 추출
    for (UINT i = 0; i < InShaderDecs.ConstantBuffers; ++i)
    {
        ID3D11ShaderReflectionConstantBuffer* pCB = InReflector->GetConstantBufferByIndex(i);
        if (pCB)
        {
            D3D11_SHADER_BUFFER_DESC cbDesc = {};
            const HRESULT hr = pCB->GetDesc(&cbDesc);
            if(cbDesc.Type != D3D_CT_CBUFFER)
            {
                continue;
            }
            
            const FString CBName = cbDesc.Name;
            uint32 BindingSlot = 0;
            
            for (UINT j = 0; j < InShaderDecs.BoundResources; ++j)
            {
                D3D11_SHADER_INPUT_BIND_DESC bindDesc = {};
                if (SUCCEEDED(InReflector->GetResourceBindingDesc(j, &bindDesc)))
                {
                    if (bindDesc.Type != D3D_SIT_CBUFFER)
                    {
                        continue;
                    }
                    
                    if (_stricmp(bindDesc.Name, cbDesc.Name) == 0)  // 이름 비교, 대소문자 무시
                    {
                        BindingSlot = bindDesc.BindPoint;
                        break;
                    }
                }
            }
            CBInfos.Add(FConstantBufferInfo(CBName, cbDesc.Size, BindingSlot));
        }
    }
    
    return CBInfos;
}

ID3D11InputLayout* FGraphicsDevice::ExtractInputLayout(ID3DBlob* InShaderBlob, ID3D11ShaderReflection* InReflector, const D3D11_SHADER_DESC& InShaderDecs) const
{
    // 입력 레이아웃 기술자들을 저장할 배열
    std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDescs;

    // 각 입력 파라미터에 대해 정보를 가져와서 레이아웃 기술자 배열에 추가합니다.
    for (UINT i = 0; i < InShaderDecs.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
        const HRESULT hr = InReflector->GetInputParameterDesc(i, &paramDesc);
        if (FAILED(hr))
            continue;

        D3D11_INPUT_ELEMENT_DESC elementDesc = {};
        elementDesc.SemanticName = paramDesc.SemanticName;
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = 0;
        if (i == 0)
        {
            elementDesc.AlignedByteOffset = 0;
        }
        else
        {
            elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
        }
        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        elementDesc.InstanceDataStepRate = 0;

        // 파라미터의 Mask 값에 따라 구성 요소 수를 결정하고, ComponentType를 기반으로 DXGI_FORMAT을 정합니다.
        // Mask 값은 해당 파라미터의 몇 개 요소가 사용되는지 나타냅니다.
        if (paramDesc.Mask == 1)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D10_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32_UINT;
        }
        else if (paramDesc.Mask <= 3)  // 두 구성 요소
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
        }
        else if (paramDesc.Mask <= 7)  // 세 구성 요소
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D10_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
        }
        else if (paramDesc.Mask <= 15)  // 네 구성 요소
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D10_REGISTER_COMPONENT_UINT32)
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
        }
        else
        {
            // 기본값 또는 지원되지 않는 구성의 경우 기본적인 형식으로 설정할 수 있습니다.
            elementDesc.Format = DXGI_FORMAT_UNKNOWN;
        }
        layoutDescs.push_back(elementDesc);
    }

    // 입력 레이아웃 생성
    ID3D11InputLayout* inputLayout = nullptr;
    const HRESULT hr = Device->CreateInputLayout(layoutDescs.data(),
                                   static_cast<UINT>(layoutDescs.size()),
                                   InShaderBlob->GetBufferPointer(),
                                   InShaderBlob->GetBufferSize(),
                                   &inputLayout);
    
    return inputLayout;
}
