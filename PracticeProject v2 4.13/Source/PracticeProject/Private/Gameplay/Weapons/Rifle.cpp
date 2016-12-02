// By Polyakov Pavel

#include "PracticeProject.h"
#include "Rifle.h"

void ARifle::OnFire()
{
    const FHitResult TraceHit = SingleShot();
	PlayEffects(TraceHit);

    //DrawDebugLine(GetWorld(), TraceHit.TraceStart, TraceHit.TraceEnd, FColor::Red, false, 1.5f);
}