#pragma once

#include "Engine.h"

/** Performs three line collision checks: along a given line and along its offsets */
bool SmartCollisionCheck(const FVector& Start, const FVector& End, float NormalOffset, const UWorld* World, ECollisionChannel CollisionChannel = ECC_WorldStatic);

/** Performs a simple collison check along a given line */
bool SimpleCollisionCheck(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel = ECC_WorldStatic);

/** Projects a point on ground */
FVector ProjectPoint(const FVector& Point, const UWorld* World);
