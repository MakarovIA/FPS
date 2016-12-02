// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTTask_Follow.h"

UBTTask_Follow::UBTTask_Follow()
{
	NodeName = "Follow Enemy";
	BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_Follow, BotData), UBotBlackBoardData::StaticClass());
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Follow::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_Follow::InitializeFromAsset(UBehaviorTree &Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
		BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTTask_Follow::GetStaticDescription() const
{
	return FString("Make enemies suffer");
}

void UBTTask_Follow::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) {
	ABot* CurrentBot = Cast<ABot>(OwnerComp.GetAIOwner()->GetPawn());
	if (!CurrentBot) {
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	UBotBlackBoardData* Data = static_cast<UBotBlackBoardData*>(OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID()));
	if (!Data) {
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Data->BotLastHealth = -1;

	if (CurrentBot->GetBotState().Health < Data->HealthThreshold) {
		OwnerComp.GetAIOwner()->MoveToLocation((*CurrentBot).GetActorLocation(), 5.f, false, true, true);
		Data->Target = (*CurrentBot).GetActorLocation();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	//TODO
	if (Data->GetVisibleEnemies().Num() == 0)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		Data->CurrentFollowTarget.Empty();
		OwnerComp.GetAIOwner()->MoveToLocation((*CurrentBot).GetActorLocation(), 5.f, false, true, true);
		Data->FollowTarget = nullptr;
		Data->Target = (*CurrentBot).GetActorLocation();
		Data->StartFollowing = false;
		return;
	}

	if (Data->FollowTarget) {
		if ((*Data->FollowTarget).GetBotState().Health > 0) {
			if (!Data->StartFollowing) {
				Data->Target = ChooseNewCover((*CurrentBot).GetActorLocation(), CurrentBot, Data);
				OwnerComp.GetAIOwner()->MoveToLocation(Data->Target, 5.f, false, true, true);
				Data->StartFollowing = true;
			}

			if (!OwnerComp.GetAIOwner()->IsFollowingAPath()) {
				OwnerComp.GetAIOwner()->MoveToLocation(Data->Target, 5.f, false, true, true);
			}

			if (FVector::DistSquared((*CurrentBot).GetActorLocation(), Data->Target) < 100 * 100) {
				Data->StartFollowing = false;
				Data->Target = (*CurrentBot).GetActorLocation();
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
				return;
			}
		}
		else {
			Data->CurrentFollowTarget.Remove(Data->FollowTarget);
			Data->FollowTarget = nullptr;
			Data->StartFollowing = false;
		}
	}
	else {

		if (Data->CurrentFollowTarget.Num() == 0) {
			TArray<ABot*> Enemies = Data->GetVisibleEnemies();

			TArray<bool> used;
			float TheirHealth;

			for (size_t i = 0; i < Enemies.Num(); i++)
			{
				used.Add(false);
			}

			TArray<ABot*> new_group;
			float currentValue;
			TArray<ABot*> MinValueGroup;
			float MinValue = 0;

			for (size_t enemy = 0; enemy < Enemies.Num(); enemy++) {
				if (!used[enemy]) {
					new_group.Add(Enemies[enemy]);
					TheirHealth = (*Enemies[enemy]).GetBotState().Health;
					used[enemy] = true;
					for (size_t friends = enemy + 1; friends < Enemies.Num(); friends++)
					{
						if (!used[friends]) {
							if (((*Enemies[enemy]).GetActorLocation() - (*Enemies[friends]).GetActorLocation()).Size() <= MAXRadius) {
								new_group.Add(Enemies[friends]);
								TheirHealth += (*Enemies[friends]).GetBotState().Health;
								used[friends] = true;
							}
						}
					}
					currentValue = new_group.Num()*TheirHealth*((*Enemies[enemy]).GetActorLocation() - (*CurrentBot).GetActorLocation()).Size();
					if ((MinValue == 0) || (currentValue < MinValue)) {
						MinValueGroup = new_group;
						MinValue = currentValue;
					}
				}
			}
			Data->CurrentFollowTarget = MinValueGroup;
		}

		for (ABot* enemy : Data->CurrentFollowTarget) {
			if ((!Data->FollowTarget) || ((Data->FollowTarget->GetActorLocation() - CurrentBot->GetActorLocation()).Size() >(enemy->GetActorLocation() - CurrentBot->GetActorLocation()).Size())) {
				Data->FollowTarget = enemy;
			}
		}
	}
}

FVector UBTTask_Follow::ChooseNewCover(FVector CurrentLocation, ABot* CurrentBot, UBotBlackBoardData* Data) {
	AVoronoiNavData* VoronoiNavData = UVoronoiPropertiesManager::GetVoronoiNavData(GetWorld());
	FVoronoiGraph* VoronoiGraph = VoronoiNavData->GetVoronoiGraph();
	float minValue = 0;
	float currentValue;
	FVector destination;
	for (const auto& Face : VoronoiNavData->GetReachableFacesInRadius((*Data->FollowTarget).GetActorLocation(), DistForCover)) {
		if (VoronoiNavData->GetFaceByPoint(CurrentLocation) != Face) {
			if (Data->GetMapData()[Face].EnemyOverwatch == -1) {
				currentValue = (1. - (*Face).GetTacticalProperties().Visibility) * ((*Face).GetSurface()->GetLocation() - (*CurrentBot).GetActorLocation()).Size();
			}
			else {
				currentValue = ((Data->GetMapData()[Face].EnemyOverwatch)) * ((*Face).GetSurface()->GetLocation() - (*CurrentBot).GetActorLocation()).Size();
			}
			if ((minValue == 0) || (minValue > currentValue)) {
				minValue = currentValue;
				destination = Face->GetLocation();
			}
		}
	}
	return destination;
}