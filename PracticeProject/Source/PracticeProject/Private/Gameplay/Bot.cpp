// By Polyakov Pavel

#include "PracticeProject.h"
#include "Bot.h"
#include "VoronoiNavData.h"
#include "SpawnPoint.h"

ABot::ABot()
{
    PrimaryActorTick.bCanEverTick = false;

	statKey = EStatisticKey::SKT_Player;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -96));

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 450.f;
    GetCharacterMovement()->AirControl = 0.4f;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->AttachToComponent(RootComponent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->SetRelativeLocation(FVector(0, 0, 80));

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false), USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ABot::BeginPlay()
{
    Super::BeginPlay();

    BotState.Health = BotData.InitialHealth;
    for (auto i : BotData.StartInventory)
        BotState.Inventory.Add(GetWorld()->SpawnActor<AWeapon>(i));

    for (auto i : BotState.Inventory)
    {
        i->SetActorHiddenInGame(true);
        i->SetOwningBot(this);
    }

    if (BotState.Inventory.Num() > 0)
        EquipWeapon(0);
}

void ABot::setStatKey(EStatisticKey key)
{
	this->statKey = key;
}


void ABot::EquipWeapon(int32 Index)
{
    if (BotState.bDead || BotState.bIsReloading)
        return;

    if (BotState.WeaponIndex != -1)
        GetCurrentWeapon()->SetActorHiddenInGame(true);

    if (Index >= BotState.Inventory.Num())
        Index = -1;

    BotState.WeaponIndex = Index;
    if (Index == -1)
        return;

    BotState.Inventory[Index]->SetActorHiddenInGame(false);
}

void ABot::EquipWeapon(EWeaponType InWeaponType)
{
    for (int32 i = 0, sz = BotState.Inventory.Num(); i < sz; ++i)
    {
        if (BotState.Inventory[i]->GetWeaponData().WeaponType == InWeaponType) 
        {
            EquipWeapon(i);
            return;
        }
    }

    EquipWeapon(-1);
}

AWeapon* ABot::GetCurrentWeapon() const
{
    if (BotState.WeaponIndex == -1)
        return nullptr;

    return BotState.Inventory[BotState.WeaponIndex];
}

void ABot::DropWeapon()
{
    if (BotState.WeaponIndex != -1)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
        StopAiming();

        BotState.Inventory[BotState.WeaponIndex]->StopFire();
        BotState.Inventory[BotState.WeaponIndex]->SetOwningBot(nullptr);
        BotState.Inventory.RemoveAtSwap(BotState.WeaponIndex);
        BotState.WeaponIndex = -1;
    }
}

void ABot::StartFire()
{
    if (BotState.bDead)
        return;

    if (BotState.WeaponIndex != -1 && !BotState.bIsReloading)
        BotState.Inventory[BotState.WeaponIndex]->StartFire();
}

void ABot::StopFire()
{
    if (BotState.WeaponIndex != -1)
        BotState.Inventory[BotState.WeaponIndex]->StopFire();
}

void ABot::StartAiming()
{
    if (BotState.bDead)
        return;

    BotState.bIsAiming = true;
}

void ABot::StopAiming()
{
    BotState.bIsAiming = false;
}

bool ABot::ReloadNecessary()
{
	if (BotState.bDead || BotState.WeaponIndex == -1)
		return false;

	const FWeaponState& WeaponState = BotState.Inventory[BotState.WeaponIndex]->GetWeaponState();
	const FWeaponData& WeaponData = BotState.Inventory[BotState.WeaponIndex]->GetWeaponData();

	return (WeaponData.bInfiniteAmmo || WeaponState.CurrentAmmo != 0) && !WeaponData.bInfiniteAmmoPerClip && WeaponState.CurrentAmmoInClip == 0;
}

bool ABot::NeedReload() const
{
    if (BotState.bDead || BotState.WeaponIndex == -1)
        return false;

    const FWeaponState& WeaponState = BotState.Inventory[BotState.WeaponIndex]->GetWeaponState();
    const FWeaponData& WeaponData = BotState.Inventory[BotState.WeaponIndex]->GetWeaponData();

    return WeaponState.CurrentAmmo != 0 && WeaponState.CurrentAmmoInClip != WeaponData.AmmoPerClip;
}

void ABot::Reload()
{
    if (!NeedReload() || BotState.bIsReloading)
        return;

    BotState.bIsReloading = true;
    StopFire();

    if (BotAnimationData.ReloadMontage != nullptr)
    {
        GetMesh()->GetAnimInstance()->Montage_Play(BotAnimationData.ReloadMontage);

        const float ReloadTime = BotAnimationData.ReloadMontage->CalculateSequenceLength();
        GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ABot::OnReloadCompleted, ReloadTime, false);
    }
    else
        OnReloadCompleted();
}

void ABot::OnReloadCompleted() 
{
    BotState.bIsReloading = false;

    if (BotState.WeaponIndex != -1)
        BotState.Inventory[BotState.WeaponIndex]->Reload();
}

void ABot::GetAimOffset(float &Pitch) const
{
    Pitch = GetControlRotation().GetNormalized().Pitch;
}

void ABot::Die()
{
    if (BotState.bDead)
        return;

    BotState.Health = 0;
    DropWeapon();

    // Detach controller
    if (GetController())
        GetController()->UnPossess();

    // Turn off collision
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

    BotState.bDead = true;
}

void ABot::UpdateStatistic(AActor* cause)
{
	FVoronoiGraph* VoronoiGraph = (TActorIterator<AVoronoiNavData>(GetWorld()))->GetVoronoiGraph();

	VoronoiGraph->addDeath(this->statKey, this->GetActorLocation());
	AWeapon * weapon = dynamic_cast<AWeapon*>(cause);
	if (weapon != nullptr) {
		const ABot * killer = weapon->GetOwningBot();
		VoronoiGraph->addKill(killer->statKey, killer->GetActorLocation());
	}

	TActorIterator<AVoronoiNavData>(GetWorld())->UpdateVoronoiGraphDrawing();
}

void ABot::informBotSpawn()
{
	
	if (this->BotSpawn)
	{
		this->BotSpawn->OnBotDeath();
	}
	
}

float ABot::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (BotState.bDead)
        return false;

	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        //const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
        //if (PointDamageEvent.HitInfo.BoneName == BotData.HeadBone)
        //    DamageAmount = BotState.Health;
    }

    if (DamageAmount > BotState.Health)
        DamageAmount = BotState.Health;

    BotState.Health -= DamageAmount;

	if (BotState.Health == 0)
	{
		UpdateStatistic(DamageCauser);
		Die();
		informBotSpawn();
	}
    
	// Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    return DamageAmount;
}

void ABot::SetupPlayerInputComponent(class UInputComponent* InInputComponent)
{
    Super::SetupPlayerInputComponent(InInputComponent);

    check(InInputComponent);
    InInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
    InInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

    InInputComponent->BindAction("Fire", IE_Pressed, this, &ABot::StartFire);
    InInputComponent->BindAction("Fire", IE_Released, this, &ABot::StopFire);

    InInputComponent->BindAction("Aim", IE_Pressed, this, &ABot::StartAiming);
    InInputComponent->BindAction("Aim", IE_Released, this, &ABot::StopAiming);

    InInputComponent->BindAction("Crouch", IE_Pressed, this, &ABot::CrouchNoParam);
    InInputComponent->BindAction("Crouch", IE_Released, this, &ABot::UnCrouchNoParam);

    InInputComponent->BindAction("NextWeapon", IE_Pressed, this, &ABot::NextWeapon);
    InInputComponent->BindAction("PreviousWeapon", IE_Pressed, this, &ABot::PreviousWeapon);

    InInputComponent->BindAction("Reload", IE_Pressed, this, &ABot::Reload);

    InInputComponent->BindAxis("MoveForward", this, &ABot::MoveForward);
    InInputComponent->BindAxis("MoveRight", this, &ABot::MoveRight);

    InInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    InInputComponent->BindAxis("LookUp", this, &ABot::LookUp);
}

void ABot::NextWeapon()
{
    EquipWeapon(BotState.WeaponIndex + 1 == BotState.Inventory.Num() ? 0 : BotState.WeaponIndex + 1);
}

void ABot::PreviousWeapon()
{
    EquipWeapon(BotState.WeaponIndex == 0 ? BotState.Inventory.Num() - 1 : BotState.WeaponIndex - 1);
}

void ABot::CrouchNoParam()
{
    Super::Crouch();
}

void ABot::UnCrouchNoParam() 
{
    Super::UnCrouch();
}

void ABot::MoveForward(float Value)
{
    if (Controller != nullptr && Value != 0.f)
    {
        const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        AddMovementInput(Direction, Value);
    }
}

void ABot::MoveRight(float Value)
{
    if (Controller != nullptr && Value != 0.f)
    {
        const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
        const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(Direction, Value);
    }
}

void ABot::LookUp(float Value)
{
    FRotator ControlRotation = GetControlRotation();
    ControlRotation.Pitch = FMath::ClampAngle(ControlRotation.Pitch - Value, -40, 40);
    GetController()->SetControlRotation(ControlRotation);
}

void ABot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    DropWeapon();
}

float ABot::getHealth()
{
	return BotData.InitialHealth;
}

float ABot::getMaxHealth()
{
	return BotData.MaxHealth;
}
