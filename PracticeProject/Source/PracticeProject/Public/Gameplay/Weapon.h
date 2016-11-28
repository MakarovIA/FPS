// By Polyakov Pavel

#pragma once

#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

#define COLLISION_WEAPON ECC_GameTraceChannel1

class ABot;

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    WT_Melee          UMETA(DisplayName = "Melee"),
    WT_Pistol         UMETA(DisplayName = "Pistol"),
    WT_Rifle          UMETA(DisplayName = "Rifle"),
    WT_SniperRifle    UMETA(DisplayName = "Sniper Rifle"),
    WT_MachineGun     UMETA(DisplayName = "Machine Gun"),
    WT_Shotgun        UMETA(DisplayName = "Shotgun"),
    WT_RocketLauncher UMETA(DisplayName = "Rocket Launcher")
};

USTRUCT()
struct FWeaponData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    FName FireSocket;

    /** Use 0 for infinite ammo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 MaxAmmo;

    /** Use 0 for infinite ammo per cliip */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 AmmoPerClip;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 InitialAmmo;

    /** Delay if a weapn is automatic */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0.0"))
    float TimeBetweenShots;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    bool bIsAutomatic;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    EWeaponType WeaponType;

    FWeaponData()
        : FireSocket(FName(TEXT("FireSocket")))
        , MaxAmmo(400)
        , AmmoPerClip(40)
        , InitialAmmo(200)
        , TimeBetweenShots(0.1f)
        , bIsAutomatic(true)
        , WeaponType(EWeaponType::WT_Rifle) {}
};

USTRUCT()
struct FWeaponState
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 CurrentAmmo;

    UPROPERTY(BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 CurrentAmmoInClip;

    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    bool bIsFiring;

    FWeaponState() : CurrentAmmo(0), CurrentAmmoInClip(0), bIsFiring(false) {}
};

UCLASS(HideCategories = (Replication, Input, Rendering, Actor))
class PRACTICEPROJECT_API AWeapon : public AActor
{
    GENERATED_BODY()

    UPROPERTY(VisibleDefaultsOnly, Category = "Mesh")
    USkeletalMeshComponent* Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    FWeaponData WeaponData;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    FWeaponState WeaponState;

    UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    ABot *OwningBot;

    FTimerHandle FireTimerHandle;
    void Fire();

protected:
    UFUNCTION(BlueprintCallable, Category = Weapon)
    bool GetHitData(const FRotator &Direction, FHitResult& OutHit) const;

    UFUNCTION(BlueprintCallable, Category = Weapon)
    void GetAimRotation(FRotator &Direction) const;

    UFUNCTION(BlueprintImplementableEvent, Category = Weapon)
    void OnFire();

public:
    AWeapon();

    void StartFire();
    void StopFire();
    void Reload();

    void SetOwningBot(ABot *NewOwningBot);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    FORCEINLINE const FWeaponData&  GetWeaponData()  const { return WeaponData; }
    FORCEINLINE const FWeaponState& GetWeaponState() const { return WeaponState; }
    FORCEINLINE const ABot*         GetOwningBot()   const { return OwningBot; }
};
