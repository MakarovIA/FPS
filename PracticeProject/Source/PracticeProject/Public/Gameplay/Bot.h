#pragma once

#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Weapon.h"
#include "Bot.generated.h"


/** Variables that don't change during gameplay */
USTRUCT()
struct FBotData
{
    GENERATED_USTRUCT_BODY()

    /** A socket for weapon */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bot)
    FName WeaponSocket;

    /** A bone used for headshots */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bot)
    FName HeadBone;

    /** An inventory to get at the game start */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bot)
    TArray<TSubclassOf<AWeapon>> StartInventory;

    /** Max health */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bot, meta = (ClampMin = "0.0"))
    float MaxHealth;

    /** Initial health */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bot, meta = (ClampMin = "0.0"))
    float InitialHealth;

    FBotData() : WeaponSocket(FName(TEXT("Weapon"))), HeadBone(FName(TEXT("Head"))), MaxHealth(100), InitialHealth(100) {}
};

/** Variables describing current bot state */
USTRUCT()
struct FBotState
{
    GENERATED_USTRUCT_BODY()

    /** Current inventory */
    UPROPERTY(BlueprintReadOnly, Category = Bot)
    TArray<AWeapon*> Inventory;

    /** Current weapon index */
    UPROPERTY(BlueprintReadOnly, Category = Bot, meta = (ClampMin = "0.0"))
    int32 WeaponIndex;

    /** Is a bot reloading a weapon */
    UPROPERTY(BlueprintReadOnly, Category = Bot)
    bool bIsReloading;

    /** Is a bot "aiming" */
    UPROPERTY(BlueprintReadOnly, Category = Bot)
    bool bIsAiming;

    /** Current health */
    UPROPERTY(BlueprintReadOnly, Category = Bot, meta = (ClampMin = "0.0"))
    float Health;

    /** Is a bot dead */
    UPROPERTY(BlueprintReadOnly, Category = Bot)
    bool bDead;

    FBotState() : WeaponIndex(-1), bIsReloading(false), bIsAiming(false), Health(0), bDead(false) {}
};

/** Collection of montages used */
USTRUCT()
struct FBotAnimationData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bot)
    UAnimMontage* ReloadMontage;

    FBotAnimationData() : ReloadMontage(nullptr) {}
};

UCLASS()
class PRACTICEPROJECT_API ABot : public ACharacter
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bot, meta = (AllowPrivateAccess = "true"))
    FBotData BotData;

    UPROPERTY(BlueprintReadOnly, Category = Bot, meta = (AllowPrivateAccess = "true"))
    FBotState BotState;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Bot, meta = (AllowPrivateAccess = "true"))
    FBotAnimationData BotAnimationData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    FTimerHandle ReloadTimerHandle;
    void OnReloadCompleted();

    void Die();

protected:
    void LookUp(float Value);
    void MoveForward(float Value);
    void MoveRight(float Value);

    void CrouchNoParam();
    void UnCrouchNoParam();

    void NextWeapon();
    void PreviousWeapon();

public:
    ABot();

    void EquipWeapon(int32 Index);

	UFUNCTION(BlueprintCallable, Category = Bot)
    void EquipWeapon(EWeaponType InWeaponType);

    UFUNCTION(BlueprintCallable, Category = Bot)
    AWeapon* GetCurrentWeapon() const;

    void DropWeapon();

	UFUNCTION(BlueprintCallable, Category = Weapon)
    void StartFire();

    void StopFire();

    void StartAiming();
    void StopAiming();

    bool NeedReload() const;

	UFUNCTION(BlueprintCallable, Category = Bot)
	bool ReloadNecessary();

	UFUNCTION(BlueprintCallable, Category = Bot)
    void Reload();

    UFUNCTION(BlueprintCallable, Category = Bot)
    void GetAimOffset(float &Pitch) const;

    virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* InInputComponent) override;

	float getHealth();
	float getMaxHealth();

    FORCEINLINE const FBotData&  GetBotData()  const { return BotData; }
    FORCEINLINE const FBotState& GetBotState() const { return BotState; }

    FORCEINLINE const FBotAnimationData& GetBotAnimationData() const { return BotAnimationData; }

    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent*  GetFollowCamera() const { return FollowCamera; }
};
