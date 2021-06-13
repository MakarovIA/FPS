#include "PracticeProject.h"
#include "Machinegun.h"

void AMachinegun::OnFire()
{
    const FHitResult TraceHit = SingleShot();
	PlayEffects(TraceHit);

    /*DrawDebugLine(GetWorld(), TraceHit.TraceStart, TraceHit.TraceEnd, FColor::Red, false, 1.5f);*/
}