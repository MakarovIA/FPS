// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiNavigationPath.h"

#include "VoronoiNavData.h"

/** FVoronoiNavigationPath */

const FNavPathType FVoronoiNavigationPath::Type;

void FVoronoiNavigationPath::AddPathPoint(const FVoronoiFace *InFace, const FVector& InLocation, bool bIsJumpRequired)
{
    if (bIsJumpRequired)
        JumpPositions.Add(PathFaces.Last());

    PathPoints.Emplace(InLocation);
    PathFaces.Add(InFace);
}

float FVoronoiNavigationPath::GetCostFromIndex(int32 PathPointIndex) const
{
    const AVoronoiNavData* VoronoiNavData = static_cast<AVoronoiNavData*>(NavigationDataUsed.Get());
    const IVoronoiQuerier *VoronoiQuerier = Cast<const IVoronoiQuerier>(GetQuerier());

    if (VoronoiQuerier == nullptr)
        VoronoiQuerier = VoronoiNavData;

    float Cost = 0;
    for (int32 i = PathPointIndex + 1, sz = PathPoints.Num(); i < sz; ++i)
        Cost += VoronoiNavData->Distance(VoronoiQuerier, i >= 2 ? PathFaces[i - 2] : nullptr, PathFaces[i - 1], PathFaces[i], JumpPositions.Contains(PathFaces[i]));

    return Cost;
}
