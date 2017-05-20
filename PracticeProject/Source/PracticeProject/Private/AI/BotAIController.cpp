// By Polyakov Pavel

#include "PracticeProject.h"
#include "AIPlayerState.h"

#include "BotAIController.h"
#include "Bot.h"
#include "BotPathFollowingComponent.h"
#include "BotBlackBoardData.h"

/** ABotAIController */

ABotAIController::ABotAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBotPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
	FalconComponent = ObjectInitializer.CreateDefaultSubobject<UFalconComponent>(this, TEXT("FALCON"));

    BlackboardComponent = ObjectInitializer.CreateDefaultSubobject<UBlackboardComponent>(this, TEXT("AIBlackboard"));
    BehaviorTreeComponent = ObjectInitializer.CreateDefaultSubobject<UBehaviorTreeComponent>(this, TEXT("AIBehaviorTree"));

    bWantsPlayerState = true;
}

UFalconComponent* ABotAIController::getFalconComponent()
{
	return FalconComponent;
}

void ABotAIController::setFocusActor(AActor* actor)
{
	SetFocus(actor);
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
		FRotator NewControlRotation;
		if (GetFocusActor())
		{
			NewControlRotation = (GetFocusActor()->GetActorLocation() - MyPawn->GetActorLocation()).Rotation().GetNormalized();
			if (FocalPoint == GetFocalPointForPriority(EAIFocusPriority::Move))
				NewControlRotation = FMath::RInterpTo(ControlRotation, NewControlRotation, DeltaTime, 5);
			else
				NewControlRotation = FMath::RInterpConstantTo(ControlRotation, NewControlRotation, DeltaTime, 270);

			SetControlRotation(NewControlRotation);
		}
		else
		{
			NewControlRotation = (FocalPoint - MyPawn->GetActorLocation()).Rotation().GetNormalized();
			if (FocalPoint == GetFocalPointForPriority(EAIFocusPriority::Move))
				NewControlRotation = FMath::RInterpTo(ControlRotation, NewControlRotation, DeltaTime, 5);
			else
				NewControlRotation = FMath::RInterpConstantTo(ControlRotation, NewControlRotation, DeltaTime, 270);

			SetControlRotation(NewControlRotation);
		}

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

void ABotAIController::InstigatedAnyDamage(float Damage, const UDamageType *DamageType, AActor *DamagedActor, AActor *DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("Instigated damage: %f"), Damage);
	if (Cast<ABot>(DamagedActor))
	{
		FalconComponent->IncreaseReward(Damage);
	}
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

const FVoronoiQuerierParameters& ABotAIController::GetVoronoiQuerierParameters() const
{
    return VoronoiQuerierParams;
}

float ABotAIController::GetAdditionalPenalty(const FVoronoiFace *From, const FVoronoiFace *To) const
{
	const double kills = To->getKills(EStatisticKey::SKT_Bots_1);
	const double deaths = To->getDeaths(EStatisticKey::SKT_Bots_1);
	const double probability = (deaths + 1) / (deaths + kills + 2);
	const double dist = FVector::Dist(To->Location, From->Location) / 2;
	return 100 * dist * probability;
}

FVector2D ABotAIController::GetInitialRotation() const
{ 
    return FVector2D(GetControlRotation().Vector());
}

bool ABotAIController::IsVoronoiQuerier() const
{
    return true;
}
