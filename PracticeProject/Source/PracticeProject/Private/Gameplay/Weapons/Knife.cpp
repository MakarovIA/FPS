// By Polyakov Pavel

#include "PracticeProject.h"
#include "Knife.h"

void AKnife::OnFire()
{
	const FHitResult TraceHit = SingleShot();
	PlayEffects(TraceHit);

	//DrawDebugLine(GetWorld(), TraceHit.TraceStart, TraceHit.TraceEnd, FColor::Red, false, 1.5f);
}