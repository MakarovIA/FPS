// By Polyakov Pavel

#include "PracticeProject.h"
#include "AIPlayerState.h"

#include "BotAIController.h"
#include "BotPathFollowingComponent.h"

/** ABotAIController */

ABotAIController::ABotAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBotPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
    BlackboardComponent = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("AIBlackboard"));
    BehaviorTreeComponent = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("AIBehaviorTree"));

    bWantsPlayerState = true;
}

void ABotAIController::Possess(APawn* InPawn)
{
    Super::Possess(InPawn);

    if (BehaviorTree)
    {
        BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        BehaviorTreeComponent->StartTree(*BehaviorTree, EBTExecutionMode::Looped);
    }
}

void ABotAIController::UnPossess()
{
    Super::UnPossess();

    if (BehaviorTree)
        BehaviorTreeComponent->StopTree(EBTStopMode::Safe);
}

void ABotAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    const FVector FocalPoint = GetFocalPoint();
    APawn* const MyPawn = GetPawn();

    if (!FocalPoint.IsZero() && MyPawn)
    {
        FRotator NewControlRotation = (FocalPoint - MyPawn->GetActorLocation()).Rotation().GetNormalized();
        if (FocalPoint == GetFocalPointForPriority(EAIFocusPriority::Move))
            NewControlRotation = FMath::RInterpTo(ControlRotation, NewControlRotation, DeltaTime, 5);
        else
            NewControlRotation = FMath::RInterpConstantTo(ControlRotation, NewControlRotation, DeltaTime, 270);

        SetControlRotation(NewControlRotation);

        if (bUpdatePawn)
            MyPawn->FaceRotation(NewControlRotation, DeltaTime);
    }
}

bool ABotAIController::LineOfSightTo(const AActor* Other, FVector ViewPoint, bool bAlternateChecks) const
{
    if (GetPawn() == nullptr || FMath::Abs((GetControlRotation() - (Other->GetActorLocation() - GetPawn()->GetActorLocation()).Rotation()).GetNormalized().Yaw) > 60)
        return false;

    return Super::LineOfSightTo(Other, ViewPoint, bAlternateChecks);
}

void ABotAIController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
    // TODO:
}

FGenericTeamId ABotAIController::GetGenericTeamId() const
{
    if (AAIPlayerState *PS = Cast<AAIPlayerState>(PlayerState))
        return PS->GetGenericTeamId();

    return FGenericTeamId::NoTeam;
}

float ABotAIController::GetPenaltyForRotation() const
{ 
    return VoronoiQuerierParams.PenaltyForRotation;
}

float ABotAIController::GetPenaltyMultiplierForCrouch() const
{ 
    return VoronoiQuerierParams.PenaltyMultiplierForCrouch;
}

float ABotAIController::GetPenaltyForJump() const
{ 
    return VoronoiQuerierParams.PenaltyForJump;
}

float ABotAIController::GetAdditionalPenalty(FVoronoiFace *From, FVoronoiFace *To) const
{
    return 0;
}

FVector2D ABotAIController::GetInitialRotation() const
{ 
    return FVector2D(GetControlRotation().Vector());
}

bool ABotAIController::IsVoronoiQuerier() const
{
    return VoronoiQuerierParams.bIsVoronoiQuerier;
}
