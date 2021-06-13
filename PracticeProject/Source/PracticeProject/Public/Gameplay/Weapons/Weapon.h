#pragma once

#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

#define COLLISION_WEAPON ECC_GameTraceChannel1

class ABot;

/** Weapon type information is mainly used for bot animation */
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Knife = 0          UMETA(DisplayName = "Knife"),
    Rifle = 1          UMETA(DisplayName = "Rifle"),
    MachineGun = 2     UMETA(DisplayName = "Machinegun"),
    Shotgun = 3        UMETA(DisplayName = "Shotgun"),

    Pistol = 4         UMETA(DisplayName = "Pistol"),
    SniperRifle = 5    UMETA(DisplayName = "Sniper Rifle"),
    RocketLauncher = 6 UMETA(DisplayName = "Rocket Launcher")
};

/** Describes weapon's characteristics */
USTRUCT()
struct FWeaponData
{
    GENERATED_USTRUCT_BODY()

    /** Socket of a weapon mesh to fire from */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    FName FireSocket;

    /** Sound effect */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    USoundWave *SoundEffect;

	/** Particle effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<AActor> Particle;

    /** Maximum amount of ammo allowed for this weapon */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "1"))
    int32 MaxAmmo;

    /** Maximum amount of ammo per clip allowed for this weapon */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "1"))
    int32 AmmoPerClip;

    /** Initiatial amount of ammo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    int32 InitialAmmo;

    /** Indicates weapon's rate of fire. Used for both automatic and not automatic weapons */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    float TimeBetweenShots;

    /** Indicates weapon's damage per bullet */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    float DamagePerBullet;

    /** Indicates weapon's damage per bullet */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon, meta = (ClampMin = "0"))
    float Range;

    /** Minimum weapon's spread */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    float MinSpread;

    /** Maximum weapon's spread */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    float MaxSpread;

    /** Current weapon's spread */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    float SpreadIncreasingRate;

    /** Indicates whether to continue shooting after StartFire and before StopFire calls */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    bool bIsAutomatic;

    /** Indicates whether ammo for this weapon is infinite */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    bool bInfiniteAmmo;

    /** Indicates whether ammo per clip for this weapon is infinite */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    bool bInfiniteAmmoPerClip;

    /** Weapon type information is mainly used for bot animation */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
    EWeaponType WeaponType;

    FWeaponData()
        : FireSocket(FName(TEXT("FireSocket")))
        , MaxAmmo(400)
        , AmmoPerClip(40)
        , InitialAmmo(200)
        , TimeBetweenShots(0.1f)
        , DamagePerBullet(7.f)
        , Range(10000.f)
        , MinSpread(0.5f)
        , MaxSpread(2.f)
        , SpreadIncreasingRate(0.1f)
        , bIsAutomatic(true)
        , bInfiniteAmmo(false)
        , bInfiniteAmmoPerClip(false)
        , WeaponType(EWeaponType::Rifle) {}
};

/** Describes weapon's current state */
USTRUCT()
struct FWeaponState
{
    GENERATED_USTRUCT_BODY()

    /** Current amount of ammo */
    UPROPERTY(BlueprintReadOnly, Category = Weapon)
    int32 CurrentAmmo;

    /** Current amount of ammo in clip */
    UPROPERTY(BlueprintReadOnly, Category = Weapon)
    int32 CurrentAmmoInClip;

    /** Current weapon's spread */
    UPROPERTY(BlueprintReadOnly, Category = Weapon)
    float CurrentSpread;

    /** Indicates whether weapon is firing at the moment */
    UPROPERTY(BlueprintReadOnly, Category = Weapon)
    bool bIsFiring;

    FWeaponState() : CurrentAmmo(0), CurrentAmmoInClip(0), CurrentSpread(0), bIsFiring(false) {}
};

/** Base class for all weapons used in the game */
UCLASS(HideCategories = (Replication, Input, Rendering, Actor))
class PRACTICEPROJECT_API AWeapon : public AActor
{
    GENERATED_BODY()

    /** Weapon's mesh */
    UPROPERTY(VisibleDefaultsOnly, Category = "Mesh")
    USkeletalMeshComponent* Mesh;

    /** Weapon's characteristics */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    FWeaponData WeaponData;

    /** Current weapon state */
    UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    FWeaponState WeaponState;

    /** Weapon's owner */
    UPROPERTY(BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    ABot *OwningBot;

    FTimerHandle FireTimerHandle;
    void Fire(int NumShots);

protected:
    /** Overrided in child classes */
    virtual void OnFire() { check(0); }

    /** Fire once and return info about hit */
    FHitResult SingleShot();
	void PlayEffects(FHitResult TraceHit);

public:
    AWeapon();

    void StartFire(int numShots = 3);
    void StopFire();
    void Reload();

    void SetOwningBot(ABot *NewOwningBot);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    FORCEINLINE const FWeaponData&  GetWeaponData()  const { return WeaponData; }
    FORCEINLINE const FWeaponState& GetWeaponState() const { return WeaponState; }
    FORCEINLINE const ABot*         GetOwningBot()   const { return OwningBot; }
};