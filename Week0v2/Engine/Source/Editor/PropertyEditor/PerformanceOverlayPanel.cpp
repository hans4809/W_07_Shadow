#include "PerformanceOverlayPanel.h"
#include "EditorEngine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Profiling/PlatformTime.h"
#include "Profiling/StatRegistry.h"


void PerformanceOverlayPanel::Render()
{
    /* Pre Setup */
    ImGuiIO& io = ImGui::GetIO();

    float PanelWidth = Width * 0.2f - 6.0f;
    float PanelHeight = Height * 0.25f;

    float PanelPosX = 0 * 0.8f + 5.0f;
    float PanelPosY = Height * 0.35f; // Outliner 아래에 위치하도록

    ImVec2 MinSize(140, 100);
    ImVec2 MaxSize(FLT_MAX, 400);

    /* Size Constraints */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Position & Size */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Window Flags */
    ImGuiWindowFlags PanelFlags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse;

    /* Background Alpha (for performance panel transparency) */
    ImGui::SetNextWindowBgAlpha(0.35f);

    /* Begin Panel */
    ImGui::Begin("Performance", nullptr, PanelFlags);

    // FPS 표시
    static TStatId Stat_Frame("MainFrame");
    float fps = static_cast<float>(FStatRegistry::GetFPS(Stat_Frame));
    ImGui::Text("FPS: %.2f", fps);
    auto Stats = FStatRegistry::GetFPSStats(Stat_Frame);
    ImGui::Text("FPS (1s): %.2f", Stats.FPS_1Sec);
    ImGui::Text("FPS (5s): %.2f", Stats.FPS_5Sec);

    // Stat 타이밍 표시
    if (ImGui::CollapsingHeader("Stat Timings (ms)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const auto& StatMap = FStatRegistry::GetStatMap();
        for (const auto& Pair : StatMap)
        {
            const uint32 StatKey = Pair.Key;
            double Ms = Pair.Value;

            FName StatName(StatKey); // FName 생성자 필요
            FString NameString = StatName.ToString();

            ImGui::Text("%s: %.3f ms", GetData(NameString), Ms);
        }
    }

    ImGui::End();
}


void PerformanceOverlayPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}
