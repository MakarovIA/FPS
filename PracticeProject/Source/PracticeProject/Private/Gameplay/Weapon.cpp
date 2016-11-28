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

void AWeapon::Fire()
{
    if (WeaponState.CurrentAmmoInClip == 0)
        WeaponState.bIsFiring = false;

    if (WeaponState.bIsFiring == false)
        return;

    --WeaponState.CurrentAmmoInClip;
    OnFire();

    if (WeaponData.bIsAutomatic)
        GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AWeapon::Fire, WeaponData.TimeBetweenShots, false);
    else
        WeaponState.bIsFiring = false;
}

void AWeapon::Reload()
{
    int32 reload_count = WeaponData.AmmoPerClip - WeaponState.CurrentAmmoInClip;
    if (WeaponData.MaxAmmo != 0)
        reload_count = FMath::Min(reload_count, WeaponState.CurrentAmmo);

    WeaponState.CurrentAmmo -= reload_count;
    WeaponState.CurrentAmmoInClip += reload_count;
}

void AWeapon::StartFire()
{
    WeaponState.bIsFiring = true;
    Fire();
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

bool AWeapon::GetHitData(const FRotator &Direction, FHitResult &OutHit) const
{
    static const FCollisionQueryParams TraceParams(FName(TEXT("WeaponTrace")), true, this);

    const FVector FireStart = Mesh->GetSocketLocation(WeaponData.FireSocket);
    const FVector FireEnd = FireStart + Direction.Vector() * 10000;

    OutHit = FHitResult(ForceInit);
    return GetWorld()->LineTraceSingleByChannel(OutHit, FireStart, FireEnd, COLLISION_WEAPON, TraceParams);
}

void AWeapon::GetAimRotation(FRotator &Direction) const
{
    Direction = (OwningBot == nullptr ? GetActorRotation().GetNormalized() : OwningBot->GetControlRotation().GetNormalized());
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
