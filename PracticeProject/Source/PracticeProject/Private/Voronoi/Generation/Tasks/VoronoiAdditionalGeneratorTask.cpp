// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiAdditionalGeneratorTask.h"

/** FVoronoiAdditionalGeneratorTask */

void FVoronoiAdditionalGeneratorTask::DoWork()
{
    // Get prerequisites
    const UWorld* World = VoronoiNavData->GetWorld();
    const FNavDataConfig& AgentProperties = VoronoiNavData->GetConfig();

    TArray<TUniquePtr<FVoronoiSurface>>& Surfaces = VoronoiNavData->GetVoronoiGraph()->GeneratedSurfaces;

    const float JumpZVelocity = 450;
    const float MaxWalkVelocity = 600;

    const float Gravity = FMath::Abs(World->GetGravityZ());

    const FVector AgentCrouchedHeight(0, 0, AgentProperties.AgentHeight * 2 / 3);
    const FVector AgentFullHeight(0, 0, AgentProperties.AgentHeight);

    const FVector AgentJumpHeight(0, 0, JumpZVelocity * JumpZVelocity / 2 / Gravity);
    const FVector AgentJumpFullHeight = AgentFullHeight + AgentJumpHeight;

    // -------------------------------------------------------------------------------------------------------------
    // Building links
    // -------------------------------------------------------------------------------------------------------------

    if (TaskType == EVoronoiTaskType::VTT_Links)
    {
        /** Link Candidate */
        struct FLinkCandidate
        {
            FVoronoiFace *First, *Second;
            FLinkCandidate(FVoronoiFace *InFirst, FVoronoiFace *InSecond) : First(InFirst), Second(InSecond) {}
        };

        // Choose candidates for link building
        TArray<FLinkCandidate> Candidates;
        Candidates.Reserve(1000);

        for (const TUniquePtr<FVoronoiSurface>& Surface : Surfaces)
        {
            for (const TUniquePtr<FVoronoiFace>& BorderFace : Surface->GetFaces())
            {
                // Build was canceled
                if (ShouldCancelBuild())
                    return;

                if (!BorderFace->IsNavigable())
                    continue;

                if (BorderFace->GetFlags().bBorder)
                {
                    // Get faces that lie not far than 500
                    TArray<FVoronoiFace*> NeighboorFaces;
                    for (const TUniquePtr<FVoronoiSurface>& TempSurface : Surfaces)
                        if (TempSurface != Surface)
                            NeighboorFaces.Append(TempSurface->GetQuadTree()->GetFacesInCircle(FVector2D(BorderFace->GetLocation()), 500));

                    // Cache edges
                    TArray<FVoronoiEdge*> Edges = FVoronoiHelper::GetAdjacentEdges(BorderFace.Get());

                    // Iterate through neighboor faces and choose link candidates
                    for (const auto& Face : NeighboorFaces)
                    {
                        // Build was canceled
                        if (ShouldCancelBuild())
                            return;

                        // It is a duplicate candidate (yes, pointer addresses are compared)
                        if (Face->Flags.bBorder && Face < BorderFace.Get())
                            continue;

                        // No links in not navigable faces
                        if (!Face->IsNavigable())
                            continue;

                        // Check if height difference is too high
                        if (FMath::Abs(BorderFace->GetLocation().Z - Face->GetLocation().Z) < AgentProperties.AgentHeight * 5)
                        {
                            const double A1 = BorderFace->GetLocation().Y - Face->GetLocation().Y;
                            const double B1 = Face->GetLocation().X - BorderFace->GetLocation().X;
                            const double C1 = BorderFace->GetLocation().X * Face->GetLocation().Y - BorderFace->GetLocation().Y * Face->GetLocation().X;

                            for (FVoronoiEdge* Edge : Edges)
                            {
                                if (Edge->GetLeftFace()->GetFlags().bInvalid || Edge->GetRightFace()->GetFlags().bInvalid)
                                {
                                    const double A2 = Edge->GetFirstVertex()->GetLocation().Y - Edge->GetLastVertex()->GetLocation().Y;
                                    const double B2 = Edge->GetLastVertex()->GetLocation().X - Edge->GetFirstVertex()->GetLocation().X;
                                    const double C2 = Edge->GetFirstVertex()->GetLocation().X * Edge->GetLastVertex()->GetLocation().Y -
                                        Edge->GetFirstVertex()->GetLocation().Y * Edge->GetLastVertex()->GetLocation().X;

                                    const double Det = A1 * B2 - A2 * B1;
                                    if (FMath::IsNearlyZero(Det))
                                        continue;

                                    const double X = (B1 * C2 - B2 * C1) / Det;
                                    if ((X - Face->GetLocation().X) * (X - BorderFace->GetLocation().X) <= 0.01 &&
                                        (X - Edge->GetFirstVertex()->GetLocation().X) * (X - Edge->GetLastVertex()->GetLocation().X) <= 0.01)
                                        Candidates.Emplace(FLinkCandidate(BorderFace.Get(), Face));
                                }
                            }
                        }
                    }
                }
            }
        }

        // Process candidates
        for (FLinkCandidate& i : Candidates)
        {
            // Build was canceled
            if (ShouldCancelBuild())
                return;

            // First and simple raycast check: if a bot can't see a destination he won't jump
            if (SmartCollisionCheck(i.First->GetLocation() + AgentFullHeight, i.Second->GetLocation() + AgentFullHeight, AgentProperties, World))
                continue;

            // Try build First->Second link, then Second->First
            for (int32 j = 0; j < 2; ++j, Swap(i.First, i.Second))
            {
                bool bJumpVariant = !i.First->GetFlags().bNoJump;
                bool bNoJumpVariant = true;

                // Bot should see the ground to jump
                if (SmartCollisionCheck(i.First->GetLocation() + AgentFullHeight, i.Second->GetLocation() + AgentFullHeight / 10, AgentProperties, World))
                    bNoJumpVariant = false;

                if (bJumpVariant && SmartCollisionCheck(i.First->GetLocation() + AgentJumpFullHeight, i.Second->GetLocation() + AgentFullHeight / 10, AgentProperties, World))
                    bJumpVariant = false;

                // Maybe that is enough
                if (!(bJumpVariant || bNoJumpVariant))
                    continue;

                // Raycast to check for possibility of no jumping
                if (bNoJumpVariant)
                {
                    const FVector StartingPoint = i.First->GetLocation() + AgentFullHeight;
                    const FVector EndingPoint = i.Second->GetLocation() + AgentFullHeight;
                    const FVector Line = EndingPoint - StartingPoint;

                    const float Distance = Line.Size();
                    const int32 RaycastCount = Distance / 25;
                    const FVector Step = Line / RaycastCount;

                    float PreviousZ = ProjectPoint(StartingPoint, World).Z;

                    for (int32 RaycastNumber = 0; RaycastNumber < RaycastCount; ++RaycastNumber)
                    {
                        float ProjectionZ = ProjectPoint(StartingPoint + (RaycastNumber + 1) * Step, World).Z;
                        if (PreviousZ < ProjectionZ - 40)
                        {
                            bNoJumpVariant = false;
                            break;
                        }

                        PreviousZ = ProjectionZ;
                    }
                }

                // Check physics
                if (!bNoJumpVariant && bJumpVariant)
                {
                    const float ZDifference = i.First->GetLocation().Z + AgentJumpHeight.Z - i.Second->GetLocation().Z;

                    // Destination is very high
                    if (ZDifference < 0)
                        continue;

                    const float Time = FMath::Sqrt(2 * ZDifference / Gravity) + FMath::Sqrt(2 * AgentJumpHeight.Z / Gravity);

                    // Not enough time
                    if (MaxWalkVelocity * Time * MaxWalkVelocity * Time < FVector2D::DistSquared(FVector2D(i.First->GetLocation()), FVector2D(i.Second->GetLocation())))
                        continue;
                }

                // Finally build a link. Variant without jump is preferable
                if (bJumpVariant || bNoJumpVariant)
                    i.First->Links.Add(FVoronoiLink(i.Second, !bNoJumpVariant));
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------------
    // Calculating properties
    // -------------------------------------------------------------------------------------------------------------

    if (TaskType == EVoronoiTaskType::VTT_Properties)
    {
        int32 N = 0;
        for (const auto& Surface : Surfaces)
            N += Surface->GetFaces().Num();

        // Calculate areas of faces
        TMap<FVoronoiFace*, double> Areas;
        Areas.Reserve(N);

        for (const auto& Surface : Surfaces)
        {
            for (const auto& Face : Surface->GetFaces())
            {
                double Area = 0;

                TArray<FVoronoiVertex*> Vertexes = FVoronoiHelper::GetAdjacentVertexes(Face.Get());
                for (int32 Index = 2, sz = Vertexes.Num(); Index < sz; ++Index)
                {
                    const FVector2D AB = FVector2D(Vertexes[Index]->GetLocation() - Vertexes[0]->GetLocation());
                    const FVector2D AC = FVector2D(Vertexes[Index - 1]->GetLocation() - Vertexes[0]->GetLocation());

                    Area += FMath::Abs(AB.X * AC.Y - AC.X * AB.Y) / 2;
                }

                Areas.Add(Face.Get(), Area);
            }
        }

        // Calculate visibility
        for (const auto& Surface : Surfaces)
        {
            for (const auto& Face : Surface->GetFaces())
            {
                // Build was canceled
                if (ShouldCancelBuild())
                    return;

                if (!Face->IsNavigable())
                    continue;

                // Get faces that lie NOT FAR THEN 20 METRES
                // For performance reasons a greater radius can not be chosen

                const float RADIUS = 2000;

                TArray<FVoronoiFace*> NeighboorFaces;
                for (const TUniquePtr<FVoronoiSurface>& TempSurface : Surfaces)
                    NeighboorFaces.Append(TempSurface->GetQuadTree()->GetFacesInCircle(FVector2D(Face->GetLocation()), RADIUS));

                Face->TacticalProperties = {};

                for (const auto& Other : NeighboorFaces)
                {
                    // Build was canceled
                    if (ShouldCancelBuild())
                        return;

                    if (!Other->IsNavigable() || Face.Get() == Other)
                        continue;

                    const float Distance = FVector::Dist(Face->GetLocation(), Other->GetLocation());
                    const float Angle = FMath::Atan2(Face->GetLocation().Y - Other->GetLocation().Y, Face->GetLocation().X - Other->GetLocation().X) * 180 / PI;

                    const FVector FaceLocation = Face->GetLocation() - 30;
                    const FVector OtherLocation = Other->GetLocation() - 30;

                    const int32 Hits = !SmartCollisionCheck(FaceLocation + AgentFullHeight, OtherLocation + AgentFullHeight, AgentProperties, World) +
                        !SmartCollisionCheck(FaceLocation + AgentCrouchedHeight, OtherLocation + AgentCrouchedHeight, AgentProperties, World);

                    const float VisibleArea = Areas[Other] * Hits / 2;

                    Face->TacticalProperties.Visibility += VisibleArea;

                    if (Distance < RADIUS / 2) Face->TacticalProperties.CloseRangeVisibility += VisibleArea;
                    else                       Face->TacticalProperties.FarRangeVisibility   += VisibleArea;

                    if (Angle < -45 && Angle > -135) Face->TacticalProperties.SouthVisibility += VisibleArea;
                    if (Angle > 135 || Angle < -135) Face->TacticalProperties.WestVisibility  += VisibleArea;
                    if (Angle > -45 && Angle < 45)   Face->TacticalProperties.EastVisibility  += VisibleArea;
                    if (Angle > 45 && Angle < 135)   Face->TacticalProperties.NorthVisibility += VisibleArea;

                    if (Angle < -90)                 Face->TacticalProperties.SWVisibility += VisibleArea;
                    if (Angle > -90 && Angle < 0)    Face->TacticalProperties.SEVisibility += VisibleArea;
                    if (Angle > 90)                  Face->TacticalProperties.NWVisibility += VisibleArea;
                    if (Angle < 90 && Angle > 0)     Face->TacticalProperties.NEVisibility += VisibleArea;
                }

                Face->TacticalProperties.Visibility           = FMath::Min(1.f, Face->TacticalProperties.Visibility           / (RADIUS * RADIUS * PI));
                Face->TacticalProperties.CloseRangeVisibility = FMath::Min(1.f, Face->TacticalProperties.CloseRangeVisibility / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.FarRangeVisibility   = FMath::Min(1.f, Face->TacticalProperties.FarRangeVisibility   / (RADIUS * RADIUS * PI * 3 / 4));
                Face->TacticalProperties.SouthVisibility      = FMath::Min(1.f, Face->TacticalProperties.SouthVisibility      / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.WestVisibility       = FMath::Min(1.f, Face->TacticalProperties.WestVisibility       / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.EastVisibility       = FMath::Min(1.f, Face->TacticalProperties.EastVisibility       / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.NorthVisibility      = FMath::Min(1.f, Face->TacticalProperties.NorthVisibility      / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.SWVisibility         = FMath::Min(1.f, Face->TacticalProperties.SWVisibility         / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.SEVisibility         = FMath::Min(1.f, Face->TacticalProperties.SEVisibility         / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.NWVisibility         = FMath::Min(1.f, Face->TacticalProperties.NWVisibility         / (RADIUS * RADIUS * PI / 4));
                Face->TacticalProperties.NEVisibility         = FMath::Min(1.f, Face->TacticalProperties.NEVisibility         / (RADIUS * RADIUS * PI / 4));
            }
        }
    }
}
