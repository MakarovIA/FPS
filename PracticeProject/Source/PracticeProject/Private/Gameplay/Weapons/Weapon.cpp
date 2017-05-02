// By Polyakov Pavel

#include "PracticeProject.h"
#include "Weapon.h"
#include "Bot.h"

AWeapon::AWeapon()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
    Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
    Mesh->bReceivesDecals = false;
    Mesh->CastShadow = true;
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

    PrimaryActorTick.bCanEverTick = false;
}

void AWeapon::Fire(int NumShots)
{
    if ((WeaponState.CurrentAmmoInClip == 0 && !WeaponData.bInfiniteAmmoPerClip) || (WeaponData.bIsAutomatic && NumShots == 0))
        WeaponState.bIsFiring = false;

    if (!WeaponState.bIsFiring)
        return;

	if (!WeaponData.bInfiniteAmmoPerClip)
		--WeaponState.CurrentAmmoInClip;

    OnFire();

    WeaponState.CurrentSpread += (WeaponData.MaxSpread - WeaponState.CurrentSpread) * WeaponData.SpreadIncreasingRate;

    if (WeaponData.bIsAutomatic)
		GetWorldTimerManager().SetTimer(FireTimerHandle, [this, NumShots]() { this->Fire(NumShots - 1); }, WeaponData.TimeBetweenShots, false);
    else
        WeaponState.bIsFiring = false;
}

FHitResult AWeapon::SingleShot()
{
    static FRandomStream RandStream(42);

    FVector ForwardVector = GetActorForwardVector();
    if (OwningBot != nullptr && OwningBot->GetController() != nullptr)
        ForwardVector = OwningBot->GetController()->GetActorForwardVector();

    const FVector StartTrace = Mesh->GetSocketLocation(WeaponData.FireSocket);
    const FVector EndTrace = StartTrace + RandStream.VRandCone(ForwardVector, WeaponState.CurrentSpread * PI / 180.f) * WeaponData.Range;

    FCollisionQueryParams TraceParams("Weapon Trace", true, this);
    TraceParams.AddIgnoredActor(GetOwningBot());

    FHitResult TraceHit;
    GetWorld()->LineTraceSingleByChannel(TraceHit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

    if (ABot *HitBot = Cast<ABot>(TraceHit.GetActor()))
        UGameplayStatics::ApplyPointDamage(HitBot, WeaponData.DamagePerBullet, StartTrace, TraceHit, GetOwningBot()->GetController(), this, UDamageType::StaticClass());

    return TraceHit;
}

void AWeapon::PlayEffects(FHitResult TraceHit)
{
	if (GetWeaponData().SoundEffect)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), GetWeaponData().SoundEffect, TraceHit.TraceStart);

	if (TraceHit.bBlockingHit) {
		UWorld* const World = GetWorld();
		if (World) {
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FTransform NewTranform(TraceHit.Location);
			World->SpawnActor<AActor>(GetWeaponData().Particle, NewTranform, SpawnParams);
		}
	}
}

void AWeapon::Reload()
{
    if (WeaponData.bInfiniteAmmoPerClip)
        return;

    if (WeaponData.bInfiniteAmmo)
        WeaponState.CurrentAmmoInClip = WeaponData.AmmoPerClip;
    else
    {
        const int32 reload_count = FMath::Min(WeaponData.AmmoPerClip - WeaponState.CurrentAmmoInClip, WeaponState.CurrentAmmo);

        WeaponState.CurrentAmmo -= reload_count;
        WeaponState.CurrentAmmoInClip += reload_count;
    }
}

void AWeapon::StartFire(int numShots)
{
    WeaponState.CurrentSpread = WeaponData.MinSpread;
    WeaponState.bIsFiring = true;
    Fire(numShots);
}

void AWeapon::StopFire()
{
    WeaponState.bIsFiring = false;
}

void AWeapon::SetOwningBot(ABot *NewOwningBot)
{
    if (OwningBot != nullptr)
        RootComponent->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, false));

    OwningBot = NewOwningBot;
    if (OwningBot != nullptr)
    {
        AttachToComponent(OwningBot->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false), OwningBot->GetBotData().WeaponSocket);

        Mesh->SetSimulatePhysics(false);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    else
    {
        Mesh->SetSimulatePhysics(true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
        Mesh->SetAllPhysicsAngularVelocity(FVector(FMath::RandRange(-30, 30), FMath::RandRange(-30, 30), FMath::RandRange(-30, 30)));
    }
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

    WeaponState.CurrentAmmo = WeaponData.InitialAmmo;
    Reload();
}

void AWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}