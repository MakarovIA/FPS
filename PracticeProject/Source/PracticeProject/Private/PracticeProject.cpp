// By Polyakov Pavel

#include "PracticeProject.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, PracticeProject, "PracticeProject");

bool SmartCollisionCheck(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProperties,
    const UWorld* World, ECollisionChannel CollisionChannel)
{
    static const FCollisionQueryParams TraceParams(FName(TEXT("ObstacleTrace")));
    const FVector Normal = FVector(Start.Y - End.Y, End.X - Start.X, 0).GetSafeNormal() * AgentProperties.AgentRadius * 1.05f;

    return World->LineTraceTestByObjectType(Start, End, CollisionChannel, TraceParams) ||
        World->LineTraceTestByObjectType(Start + Normal, End + Normal, CollisionChannel, TraceParams) ||
        World->LineTraceTestByObjectType(Start - Normal, End - Normal, CollisionChannel, TraceParams);
}

bool SimpleCollisionCheck(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel)
{
    static const FCollisionQueryParams TraceParams(FName(TEXT("ObstacleTrace")));
    return World->LineTraceTestByObjectType(Start, End, CollisionChannel, TraceParams);
}

FVector FindCollisionPlace(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel)
{
    static const FCollisionQueryParams TraceParams(FName(TEXT("ObstacleTrace")));
    FHitResult Hit(ForceInit);

    return World->LineTraceSingleByObjectType(Hit, Start, End, CollisionChannel, TraceParams) ? Hit.Location : End;
}

FVector ProjectPoint(const FVector& Point, const UWorld* World)
{
    static const FCollisionQueryParams TraceParams(FName(TEXT("ObstacleTrace")));
    static const FVector ProjectionDirection(0, 0, -100000);

    FHitResult Hit(ForceInit);
    if (!World->LineTraceSingleByObjectType(Hit, Point, Point + ProjectionDirection, ECC_WorldStatic, TraceParams))
        return Point;
    
    FVector Answer = Hit.Location;

    /** Determine if a given point is inside an obstacle and do not project it in this case.
        A line trace should intersect even number of mesh triangles. */

    int32 HitsNumber = 1;
    while (World->LineTraceSingleByObjectType(Hit, Hit.Location + FVector(0, 0, -1), Point + FVector(0, 0, -1) + ProjectionDirection, ECC_WorldStatic, TraceParams))
        ++HitsNumber;

    if (HitsNumber & 1)
        return Point;
    else
        return Answer;
}
