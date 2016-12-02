// By Polyakov Pavel

#include "PracticeProject.h"
#include "Shotgun.h"

void AShotgun::OnFire()
{
	for (int32 i = 0; i < 10; ++i)
	{
		const FHitResult TraceHit = SingleShot();
		PlayEffects(TraceHit);

		// DrawDebugLine(GetWorld(), TraceHit.TraceStart, TraceHit.TraceEnd, FColor::Red, false, 1.5f);
	}
}