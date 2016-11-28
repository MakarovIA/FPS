// By Polyakov Pavel

#include "PracticeProject.h"
#include "Bot.h"

ABot::ABot()
{
    PrimaryActorTick.bCanEverTick = false;

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
    EquipWeapon(GetWorld()->SpawnActor<AWeapon>(BotData.DefaultWeapon));
}

void ABot::EquipWeapon(AWeapon *Weapon)
{
    if (BotState.bDead)
        return;

    UnEquipWeapon();
    if (Weapon == nullptr)
        return;

    if (Weapon->GetOwningBot() != this)
        if (Weapon->GetOwningBot() == nullptr)
            Weapon->SetOwningBot(this);
        else
            return;

    Weapon->SetActorHiddenInGame(false);
    BotState.Weapon = Weapon;
}

void ABot::UnEquipWeapon()
{
    if (BotState.bDead)
        return;

    if (BotState.Weapon != nullptr)
        BotState.Weapon->SetActorHiddenInGame(true);

    BotState.Weapon = nullptr;
}

void ABot::DropWeapon()
{
    if (BotState.Weapon != nullptr)
    {
        GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
        StopAiming();

        BotState.Weapon->StopFire();
        BotState.Weapon->SetOwningBot(nullptr);
        BotState.Weapon = nullptr;
    }
}

void ABot::StartFire()
{
    if (BotState.bDead)
        return;

    if (BotState.Weapon != nullptr && !BotState.bIsReloading)
        BotState.Weapon->StartFire();
}

void ABot::StopFire()
{
    if (BotState.Weapon != nullptr)
        BotState.Weapon->StopFire();
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

bool ABot::NeedReload() const
{
    if (BotState.bDead || BotState.Weapon == nullptr)
        return false;

    const FWeaponState& WeaponState = BotState.Weapon->GetWeaponState();
    const FWeaponData& WeaponData = BotState.Weapon->GetWeaponData();

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

        float ReloadTime = BotAnimationData.ReloadMontage->CalculateSequenceLength();
        GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ABot::OnReloadCompleted, ReloadTime, false);
    }
    else
        OnReloadCompleted();
}

void ABot::OnReloadCompleted() 
{
    BotState.bIsReloading = false;

    if (BotState.Weapon != nullptr)
        BotState.Weapon->Reload();
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

float ABot::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (BotState.bDead)
        return false;

    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
        if (PointDamageEvent.HitInfo.BoneName == BotData.HeadBone)
            DamageAmount = BotState.Health;
    }

    if (DamageAmount > BotState.Health)
        DamageAmount = BotState.Health;

    BotState.Health -= DamageAmount;

    if (BotState.Health == 0)
        Die();
    
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

    InInputComponent->BindAction("Reload", IE_Pressed, this, &ABot::Reload);

    InInputComponent->BindAxis("MoveForward", this, &ABot::MoveForward);
    InInputComponent->BindAxis("MoveRight", this, &ABot::MoveRight);

    InInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    InInputComponent->BindAxis("LookUp", this, &ABot::LookUp);
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
