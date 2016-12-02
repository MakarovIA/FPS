// By Polyakov Pavel

#pragma once

#include "Engine.h"

/** Check if it is IMPOSSIBLE for agent with given properties to go from Start to End */
bool SmartCollisionCheck(const FVector& Start, const FVector& End, const FNavAgentProperties& AgentProperties,
    const UWorld* World, ECollisionChannel CollisionChannel = ECC_WorldStatic);

/** Perform a simple collison check */
bool SimpleCollisionCheck(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel = ECC_WorldStatic);

/** Perform a simple collison check */
FVector FindCollisionPlace(const FVector& Start, const FVector& End, const UWorld* World, ECollisionChannel CollisionChannel = ECC_WorldStatic);

/** Projects a point on ground */
FVector ProjectPoint(const FVector& Point, const UWorld* World);
