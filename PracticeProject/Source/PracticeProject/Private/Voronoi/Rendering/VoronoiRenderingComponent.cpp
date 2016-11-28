// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiSceneProxy.h"
#include "VoronoiRenderingComponent.h"
#include "AI/Navigation/NavMeshRenderingComponent.h"

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

        const auto& DrawingOptions = VoronoiNavData->GetDrawingOptions();
        if (!DrawingOptions.bDrawEdges && DrawingOptions.FaceDrawingMode == EVoronoiFaceDrawingMode::VFDM_None && !DrawingOptions.bDrawLinks && !DrawingOptions.bDrawSurfaces)
            return nullptr;

        const FVoronoiGraph *VoronoiGraph = FVoronoiNavDataAttorney::GetVoronoiGraphUnchecked(VoronoiNavData);
        if (VoronoiGraph == nullptr)
            return nullptr;

        // ------------------------------------------------------------------------------------------
        // DATA GATHERING
        // ------------------------------------------------------------------------------------------

        FVoronoiSceneProxyData Data;

        for (const auto& Surface : FVoronoiRenderAttorney::GetSurfacesToRender(*VoronoiGraph))
        {
            if (DrawingOptions.bDrawSurfaces)
            {
                const FVector A = Surface->GetLocation(), B = A + FVector(Surface->GetSize().X, 0, 0), C = A + FVector(Surface->GetSize().X, Surface->GetSize().Y, 0), D = A + FVector(0, Surface->GetSize().Y, 0);

                Data.Lines.Emplace(A, B, FColor(0, 0, 255));
                Data.Lines.Emplace(B, C, FColor(0, 0, 255));
                Data.Lines.Emplace(C, D, FColor(0, 0, 255));
                Data.Lines.Emplace(A, D, FColor(0, 0, 255));
            }

            if (DrawingOptions.bDrawQuadTrees)
            {         
                TArray<const FVoronoiQuadTree::FQuadTreeNode*> Stack;
                Stack.Push(Surface->GetQuadTree()->GetRoot());

                while (Stack.Num() > 0)
                {
                    const auto Top = Stack.Last();
                    Stack.Pop();

                    if (Top->IsSubdivided())
                    {
                        Stack.Push(Top->GetLU());
                        Stack.Push(Top->GetLD());
                        Stack.Push(Top->GetRU());
                        Stack.Push(Top->GetRD());
                    }
                    else
                    {
                        const FBox2D& Rect = Top->GetRect();
                        const FVector A(Rect.Min, Surface->GetLocation().Z), B(Rect.Min.X, Rect.Max.Y, Surface->GetLocation().Z), C(Rect.Max, Surface->GetLocation().Z), D(Rect.Max.X, Rect.Min.Y, Surface->GetLocation().Z);

                        Data.Lines.Emplace(A, B, FColor(128, 0, 128));
                        Data.Lines.Emplace(B, C, FColor(128, 0, 128));
                        Data.Lines.Emplace(C, D, FColor(128, 0, 128));
                        Data.Lines.Emplace(A, D, FColor(128, 0, 128));
                    }
                }
            }
            
            if (DrawingOptions.bDrawEdges)
            {
                for (const auto& Edge : Surface->GetEdges())
                {
                    if (Edge->GetLeftFace()->GetFlags().bInvalid && Edge->GetRightFace()->GetFlags().bInvalid)
                        continue;

                    const FVector& EdgeBeginLocation = Edge->GetFirstVertex()->GetLocation();
                    const FVector& EdgeEndLocation = Edge->GetLastVertex()->GetLocation();

                    Data.Lines.Emplace(EdgeBeginLocation, EdgeEndLocation, (Edge->GetLeftFace()->IsNavigable() || Edge->GetRightFace()->IsNavigable()) ? FColor(0, 255, 0) : FColor(255, 0, 0));
                }
            }

            for (const auto& Face : Surface->GetFaces())
            {
                if (Face->GetFlags().bInvalid)
                    continue;

                if (DrawingOptions.FaceDrawingMode != EVoronoiFaceDrawingMode::VFDM_None)
                {
                    const FColor FaceColor = SelectFaceColor(Face.Get(), DrawingOptions);

                    const int32 FirstVertexIndex = Data.Vertices.Num();
                    for (const FVoronoiVertex *j : FVoronoiHelper::GetAdjacentVertexes(Face.Get()))
                        Data.Vertices.Emplace(j->GetLocation(), FaceColor);

                    for (int32 Index = FirstVertexIndex + 2; Index < Data.Vertices.Num(); ++Index)
                    {
                        Data.Indices.Add(FirstVertexIndex);
                        Data.Indices.Add(Index);
                        Data.Indices.Add(Index - 1);
                    }
                }

                if (DrawingOptions.bDrawLinks)
                    for (const auto& Link : Face->GetLinks())
                        Data.Lines.Emplace(Face->GetLocation(), Link.GetFace()->GetLocation(), Link.IsJumpRequired() ? FColor(255, 0, 160) : FColor(255, 160, 0));
            }
        }

        return new FVoronoiSceneProxy(this, Data);
    }
#endif

    return nullptr;
}

FColor UVoronoiRenderingComponent::SelectFaceColor(FVoronoiFace *Face, const FVoronoiDrawingOptions &Options) const
{
    if (!Face->IsNavigable())
        return FColor(127, 0, 0, 200);

    const FVoronoiTacticalProperties& TacticalProperties = Face->GetTacticalProperties();
    switch (Options.FaceDrawingMode)
    {
        case EVoronoiFaceDrawingMode::VFDM_Standard:             return FColor(0, 120, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_Visibility:           return FColor(0, TacticalProperties.Visibility           * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_CloseRangeVisibility: return FColor(0, TacticalProperties.CloseRangeVisibility * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_FarRangeVisibility:   return FColor(0, TacticalProperties.FarRangeVisibility   * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_SouthVisibility:      return FColor(0, TacticalProperties.SouthVisibility      * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_WestVisibility:       return FColor(0, TacticalProperties.WestVisibility       * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_EastVisibility:       return FColor(0, TacticalProperties.EastVisibility       * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_NorthVisibility:      return FColor(0, TacticalProperties.NorthVisibility      * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_SEVisibility:         return FColor(0, TacticalProperties.SEVisibility         * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_SWVisibility:         return FColor(0, TacticalProperties.SWVisibility         * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_NEVisibility:         return FColor(0, TacticalProperties.NEVisibility         * 255, 0, 200);
        case EVoronoiFaceDrawingMode::VFDM_NWVisibility:         return FColor(0, TacticalProperties.NWVisibility         * 255, 0, 200);

        default:                                                 return FColor();
    }
}

FBoxSphereBounds UVoronoiRenderingComponent::CalcBounds(const FTransform &LocalToWorld) const
{
    FBox BoundingBox(0);
    const AVoronoiNavData *VoronoiNavData = static_cast<const AVoronoiNavData*>(GetOwner());
    if (const FVoronoiGraph *VoronoiGraph = FVoronoiNavDataAttorney::GetVoronoiGraphUnchecked(VoronoiNavData))
    {
        for (const auto& i : FVoronoiRenderAttorney::GetSurfacesToRender(*VoronoiGraph))
        {
            BoundingBox += i->GetLocation();
            BoundingBox += i->GetLocation() + FVector(i->GetSize().X, i->GetSize().Y, 0);;

            for (const auto& j : i->GetEdges())
            {
                if (j->GetLeftFace()->GetFlags().bInvalid && j->GetRightFace()->GetFlags().bInvalid)
                    continue;

                BoundingBox += j->GetFirstVertex()->GetLocation();
                BoundingBox += j->GetLastVertex()->GetLocation();
            }
        }
    }
    
    return BoundingBox;
}

bool UVoronoiRenderingComponent::IsNavigationShowFlagSet(const UWorld* World)
{
    return UNavMeshRenderingComponent::IsNavigationShowFlagSet(World);
}
