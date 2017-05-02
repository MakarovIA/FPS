// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSceneProxy.h"
#include "VoronoiRenderingComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

UVoronoiRenderingComponent::UVoronoiRenderingComponent()
{
    SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    bSelectable = false;
}

bool UVoronoiRenderingComponent::NeedsLoadForServer() const
{
    return false;
}

FPrimitiveSceneProxy* UVoronoiRenderingComponent::CreateSceneProxy()
{
#ifdef WITH_EDITOR
    if (IsNavigationShowFlagSet(GetWorld()) && IsVisible())
    {
        const AVoronoiNavData* VoronoiNavData = static_cast<const AVoronoiNavData*>(GetOwner());
        if (!VoronoiNavData->IsDrawingEnabled())
            return nullptr;

        const FVoronoiDrawingOptions& DrawingOptions = VoronoiNavData->GetDrawingOptions();
        if (!DrawingOptions.bDrawEdges && DrawingOptions.FaceDrawingMode == EVoronoiFaceDrawingMode::None && !DrawingOptions.bDrawLinks && !DrawingOptions.bDrawSurfaces)
            return nullptr;

        const FVoronoiGraph *VoronoiGraph = FVoronoiNavDataAttorney::GetVoronoiGraphUnchecked(VoronoiNavData);
        if (VoronoiGraph == nullptr)
            return nullptr;

        // ------------------------------------------------------------------------------------------
        // DATA GATHERING
        // ------------------------------------------------------------------------------------------

        FVoronoiSceneProxyData Data;
        for (const FVoronoiSurface* Surface : FVoronoiRenderAttorney::GetSurfacesToRender(*VoronoiGraph))
        {
            if (DrawingOptions.bDrawSurfaces)
                for (const TArray<FVector> &Border : Surface->Borders)
                    for (int32 i = 0, sz = Border.Num(); i < sz; ++i)
                        Data.Lines.Emplace(Border[i == 0 ? Border.Num() - 1 : i - 1], Border[i], FColor(15, 55, 40), 5.f);

			if (DrawingOptions.bDrawEdges)
				for (const FVoronoiEdge* Edge : Surface->Edges)
					Data.Lines.Emplace(Edge->FirstVertex->Location, Edge->LastVertex->Location, FColor(0, 255, 0));

            for (const TPreserveConstUniquePtr<FVoronoiFace>& Face : Surface->Faces)
            {
                if (DrawingOptions.FaceDrawingMode != EVoronoiFaceDrawingMode::None)
                {
                    const FColor FaceColor = SelectFaceColor(Face, DrawingOptions);

                    const int32 FirstVertexIndex = Data.Vertices.Num();
                    for (const FVoronoiVertex* Vertex : FVoronoiHelper::GetAdjacentVertexes(Face))
                        Data.Vertices.Emplace(Vertex->Location, FaceColor);

                    for (int32 Index = FirstVertexIndex + 2; Index < Data.Vertices.Num(); ++Index)
                    {
                        Data.Indices.Add(FirstVertexIndex);
                        Data.Indices.Add(Index);
                        Data.Indices.Add(Index - 1);
                    }
                }

                if (DrawingOptions.bDrawLinks)
                    for (const FVoronoiLink& Link : Face->Links)
                        Data.Arcs.Emplace(Face->Location, Link.Face->Location, Link.bJumpRequired ? FColor(255, 0, 160) : FColor(255, 160, 0));
            }
        }

        return new FVoronoiSceneProxy(this, MoveTemp(Data));
    }
#endif

    return nullptr;
}

FColor UVoronoiRenderingComponent::SelectFaceColor(const FVoronoiFace *Face, const FVoronoiDrawingOptions &Options) const
{
    const FVoronoiTacticalProperties& TacticalProperties = Face->TacticalProperties;
    const uint32 SurfaceId = *(const uint32*)(const FVoronoiSurface*)Face->Surface;

    switch (Options.FaceDrawingMode)
    {
        case EVoronoiFaceDrawingMode::Standard:             return FColor((100 + SurfaceId) % 255, (200 + SurfaceId * 3) % 255, (60 + SurfaceId * 7) % 255, 200);
        case EVoronoiFaceDrawingMode::Visibility:           return FColor(0, TacticalProperties.GetFullVisibility() * 255, 0, 200);

        case EVoronoiFaceDrawingMode::SVisibility:          return FColor(0, TacticalProperties.GetSVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::WVisibility:          return FColor(0, TacticalProperties.GetWVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::EVisibility:          return FColor(0, TacticalProperties.GetEVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::NVisibility:          return FColor(0, TacticalProperties.GetNVisibility() * 4 * 255, 0, 200);

        case EVoronoiFaceDrawingMode::SEVisibility:         return FColor(0, TacticalProperties.GetSEVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::SWVisibility:         return FColor(0, TacticalProperties.GetSWVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::NEVisibility:         return FColor(0, TacticalProperties.GetNEVisibility() * 4 * 255, 0, 200);
        case EVoronoiFaceDrawingMode::NWVisibility:         return FColor(0, TacticalProperties.GetNWVisibility() * 4 * 255, 0, 200);

        default:                                            return FColor();
    }
}

FBoxSphereBounds UVoronoiRenderingComponent::CalcBounds(const FTransform &LocalToWorld) const
{
    const UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(GetWorld());
    const TSet<FNavigationBounds>& NavigationBoundsSet = NavSys->GetNavigationBounds();

    if (NavSys->ShouldGenerateNavigationEverywhere())
        return NavSys->GetWorldBounds();
    
    FBox BoundingBox(0);
    for (const FNavigationBounds& NavigationBounds : NavigationBoundsSet)
        BoundingBox += NavigationBounds.AreaBox;

    return BoundingBox;
}

void UVoronoiRenderingComponent::GetUsedMaterials(TArray<UMaterialInterface*> &OutMaterials, bool bGetDebugMaterials) const
{
    const AVoronoiNavData* VoronoiNavData = static_cast<const AVoronoiNavData*>(GetOwner());
    if (VoronoiNavData->GetDrawingOptions().VoronoiMaterial)
        OutMaterials.Add(VoronoiNavData->GetDrawingOptions().VoronoiMaterial);
}

bool UVoronoiRenderingComponent::IsNavigationShowFlagSet(const UWorld* World)
{
    const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World);
    bool bShowNavigation = WorldContext && WorldContext->GameViewport && WorldContext->GameViewport->EngineShowFlags.Navigation;

#if WITH_EDITOR
    if (GEditor && WorldContext && WorldContext->WorldType != EWorldType::Game)
        for (FEditorViewportClient* CurrentViewport : GEditor->AllViewportClients)
            if (CurrentViewport && CurrentViewport->IsVisible() && CurrentViewport->EngineShowFlags.Navigation)
                return true;
#endif //WITH_EDITOR

    return bShowNavigation;
}
