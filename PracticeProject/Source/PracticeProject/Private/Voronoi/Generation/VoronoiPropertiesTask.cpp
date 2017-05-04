// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiPropertiesTask.h"

/** Types */

struct FLinkCandidate
{
    FVoronoiFace *First, *Second;
    FORCEINLINE FLinkCandidate(FVoronoiFace *InFirst, FVoronoiFace *InSecond)
        : First(InFirst), Second(InSecond) {}
};

/** FVoronoiPropertiesTask */

void FVoronoiPropertiesTask::DoWork()
{
    static const float JumpZVelocity = 450;
    static const float MaxWalkVelocity = 600;

    const float Gravity = FMath::Abs(World->GetGravityZ());

    const FVector AgentCrouchedHeight(0, 0, AgentProperties.AgentHeight * 2 / 3);
    const FVector AgentFullHeight(0, 0, AgentProperties.AgentHeight);

    const FVector AgentJumpHeight(0, 0, JumpZVelocity * JumpZVelocity / 2 / Gravity);
    const FVector AgentJumpFullHeight = AgentFullHeight + AgentJumpHeight;

    // -------------------------------------------------------------------------------------------------------------
    // Building links
    // -------------------------------------------------------------------------------------------------------------

    TArray<FLinkCandidate> Candidates;
    for (TPreserveConstUniquePtr<FVoronoiSurface>& Surface : *GeneratedSurfaces)
    {
        for (TPreserveConstUniquePtr<FVoronoiFace>& BorderFace : Surface->Faces)
        {
            if (!BorderFace->Flags.bBorder)
                continue;

            const TArray<FVoronoiEdge*> AdjacentEdges = FVoronoiHelper::GetAdjacentEdges(BorderFace);
            const TArray<FVoronoiEdge*> BorderEdges = AdjacentEdges.FilterByPredicate([](const FVoronoiEdge* Edge) { return !Edge->LeftFace; });

            for (TPreserveConstUniquePtr<FVoronoiSurface>& TempSurface : *GeneratedSurfaces)
            {
                if (TempSurface == Surface)
                    continue;

                for (FVoronoiFace *Face : TempSurface->QuadTree->GetFacesInCircle(FVector2D(BorderFace->Location), GenerationOptions.LinksSearchRadius))
                {
                    if (ShouldCancelBuild())
                        return;

                    if ((Face->Flags.bBorder && Face < BorderFace) || FMath::Abs(BorderFace->Location.Z - Face->Location.Z) > AgentProperties.AgentHeight * 4)
                        continue;

                    FVector Dummy;
                    for (const FVoronoiEdge* Edge : BorderEdges)
                        if (FMath::SegmentIntersection2D(Face->Location, BorderFace->Location, Edge->FirstVertex->Location, Edge->LastVertex->Location, Dummy))
                            Candidates.Emplace(BorderFace, Face);
                }
            }
        }
    }

    for (FLinkCandidate& i : Candidates)
    {
        if (ShouldCancelBuild())
            return;

        if (SmartCollisionCheck(i.First->Location + (i.First->Flags.bCrouchedOnly ? AgentCrouchedHeight : AgentFullHeight),
            i.Second->Location + (i.Second->Flags.bCrouchedOnly ? AgentCrouchedHeight : AgentFullHeight), AgentProperties.AgentRadius, World))
            continue;

        for (int32 j = 0; j < 2; ++j, Swap(i.First, i.Second))
        {
            bool bNoJumpVariant = !SmartCollisionCheck(i.First->Location + (i.First->Flags.bCrouchedOnly ? AgentCrouchedHeight : AgentFullHeight), i.Second->Location + FVector(0, 0, 50), AgentProperties.AgentRadius, World);
            bool bJumpVariant = !i.First->Flags.bNoJump && !SmartCollisionCheck(i.First->Location + AgentJumpFullHeight, i.Second->Location + AgentFullHeight / 10, AgentProperties.AgentRadius, World);

            if (bNoJumpVariant)
            {
                const FVector StartingPoint = i.First->Location + AgentFullHeight;
                const FVector Line = i.Second->Location - i.First->Location;

                const float Distance = Line.Size();
                const int32 RaycastCount = Distance / 20;
                const FVector Step = Line / RaycastCount;

                float PreviousZ = ProjectPoint(StartingPoint, World).Z;
                for (int32 RaycastNumber = 0; RaycastNumber < RaycastCount; ++RaycastNumber)
                {
                    const float ProjectionZ = ProjectPoint(StartingPoint + (RaycastNumber + 1) * Step, World).Z;
                    if (PreviousZ < ProjectionZ - 30)
                    {
                        bNoJumpVariant = false;
                        break;
                    }

                    PreviousZ = ProjectionZ;
                }
            }

            if (!i.First->Flags.bBorder && bNoJumpVariant)
                continue;

            if (!bNoJumpVariant && bJumpVariant)
            {
                const float ZDifference = i.First->Location.Z + AgentJumpHeight.Z - i.Second->Location.Z;
                if (ZDifference < 0)
                    continue;

                const float Time = FMath::Sqrt(2 * ZDifference / Gravity) + FMath::Sqrt(2 * AgentJumpHeight.Z / Gravity);
                if (MaxWalkVelocity * Time * MaxWalkVelocity * Time < FVector2D::DistSquared(FVector2D(i.First->Location), FVector2D(i.Second->Location)))
                    continue;
            }

            if (bJumpVariant || bNoJumpVariant)
                i.First->Links.Emplace(i.Second, !bNoJumpVariant);
        }
    }
    
    // -------------------------------------------------------------------------------------------------------------
    // Calculating properties (Area)
    // -------------------------------------------------------------------------------------------------------------

    for (TPreserveConstUniquePtr<FVoronoiSurface>& Surface : *GeneratedSurfaces)
    {
        for (TPreserveConstUniquePtr<FVoronoiFace>& Face : Surface->Faces)
        {
            Face->TacticalProperties.Area = 0;

            TArray<FVoronoiVertex*> Vertexes = FVoronoiHelper::GetAdjacentVertexes(Face);
            for (int32 Index = 2, sz = Vertexes.Num(); Index < sz; ++Index)
                Face->TacticalProperties.Area += FMathExtended::GetTriangleArea(Vertexes[0]->Location, Vertexes[Index - 1]->Location, Vertexes[Index]->Location);
        }
    }

    // -------------------------------------------------------------------------------------------------------------
    // Calculating properties (Visibility)
    // -------------------------------------------------------------------------------------------------------------

    for (TPreserveConstUniquePtr<FVoronoiSurface>& Surface : *GeneratedSurfaces)
    {
        for (TPreserveConstUniquePtr<FVoronoiFace>& Face : Surface->Faces)
        {
            if (ShouldCancelBuild())
                return;

            float Visibility[8] = { 0.f };

            TArray<FVoronoiFace*> NeighboorFaces;
            for (TPreserveConstUniquePtr<FVoronoiSurface>& TempSurface : *GeneratedSurfaces)
                NeighboorFaces.Append(TempSurface->QuadTree->GetFacesInCircle(FVector2D(Face->Location), GenerationOptions.VisibilityRadius));

            for (FVoronoiFace* Other : NeighboorFaces)
            {
                if (ShouldCancelBuild())
                    return;

                if (Face == Other)
                    continue;

                const FVector Normal = FVector(Face->Location.Y - Other->Location.Y, Other->Location.X - Face->Location.X, 0).GetSafeNormal() * AgentProperties.AgentRadius * 0.5f;
                const int32 Hits = !SimpleCollisionCheck(Face->Location + AgentFullHeight / 3, Other->Location + AgentFullHeight / 3, World) +
                    !SimpleCollisionCheck(Face->Location + AgentFullHeight / 3 + Normal, Other->Location + AgentFullHeight / 3 + Normal, World) +
                    !SimpleCollisionCheck(Face->Location + AgentFullHeight / 3 - Normal, Other->Location + AgentFullHeight / 3 - Normal, World) +
                    !SimpleCollisionCheck(Face->Location + AgentCrouchedHeight, Other->Location + AgentCrouchedHeight, World) +
                    !SimpleCollisionCheck(Face->Location + AgentCrouchedHeight + Normal, Other->Location + AgentCrouchedHeight + Normal, World) +
                    !SimpleCollisionCheck(Face->Location + AgentCrouchedHeight - Normal, Other->Location + AgentCrouchedHeight - Normal, World);

                const float Angle = FMath::Atan2(Face->Location.Y - Other->Location.Y, Face->Location.X - Other->Location.X) * 180 / PI;
                const float VisibleArea = Other->TacticalProperties.Area * Hits / 6;

                if (Angle >= 0    && Angle < 45)    Visibility[0] += VisibleArea;
                if (Angle >= 45   && Angle < 90)    Visibility[1] += VisibleArea;
                if (Angle >= 90   && Angle < 135)   Visibility[2] += VisibleArea;
                if (Angle >= 135)                   Visibility[3] += VisibleArea;

                if (Angle < -135)                   Visibility[4] += VisibleArea;
                if (Angle >= -135 && Angle < -90)   Visibility[5] += VisibleArea;
                if (Angle >= -90  && Angle < -45)   Visibility[6] += VisibleArea;
                if (Angle >= -45  && Angle < 0)     Visibility[7] += VisibleArea;
            }

            for (int32 i = 0; i < 8; ++i)
                Visibility[i] = FMath::Min(0.125f, Visibility[i] / (1.5f * GenerationOptions.VisibilityRadius * GenerationOptions.VisibilityRadius * PI));

            FMemory::Memcpy(Face->TacticalProperties.Visibility, Visibility, 8 * sizeof(float));
        }
    }
}
