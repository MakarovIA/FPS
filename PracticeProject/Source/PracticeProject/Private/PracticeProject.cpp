#include "PracticeProject.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, PracticeProject, "PracticeProject");

bool SmartCollisionCheck(const FVector& Start, const FVector& End, float NormalOffset, const UWorld* World, ECollisionChannel CollisionChannel)
{
    static const FCollisionQueryParams TraceParams;
    const FVector Normal = FVector(Start.Y - End.Y, End.X - Start.X, 0).GetSafeNormal() * NormalOffset * 1.05f;

    return World->LineTraceTestByObjectType(Start, End, CollisionChannel, TraceParams) ||
        World->LineTraceTestByObjectType(Start + Normal, End + Normal, CollisionChannel, TraceParams) ||
        World->LineTraceTestByObjectType(Start - Normal, End - Normal, CollisionChannel, TraceParams);
}

bool SimpleCollisionCheck(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel)
{
    static const FCollisionQueryParams TraceParams;
    return World->LineTraceTestByObjectType(Start, End, CollisionChannel, TraceParams);
}

FVector ProjectPoint(const FVector& Point, const UWorld* World)
{
    static const FCollisionQueryParams TraceParams;
    static const FVector ProjectionDirection(0, 0, -100000);

    FHitResult Hit(ForceInit);
    if (!World->LineTraceSingleByObjectType(Hit, Point, Point + ProjectionDirection, ECC_WorldStatic, TraceParams))
        return Point;
    
    const FVector Answer = Hit.Location;

    int32 HitsNumber = 1;
    while (World->LineTraceSingleByObjectType(Hit, Hit.Location + FVector(0, 0, -1), Point + FVector(0, 0, -1) + ProjectionDirection, ECC_WorldStatic, TraceParams))
        ++HitsNumber;

    return (HitsNumber & 1) ? Point : Answer;
}
