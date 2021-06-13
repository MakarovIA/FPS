#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTTask_Hide.h"

UBTTask_Hide::UBTTask_Hide()
{
	NodeName = "Hide";
	BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_Hide, BotData), UBotBlackBoardData::StaticClass());
	bNotifyTick = true;
}

void UBTTask_Hide::InitializeFromAsset(UBehaviorTree &Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
		BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTTask_Hide::GetStaticDescription() const
{
	return FString("Run to a cover");
}

EBTNodeResult::Type UBTTask_Hide::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) {
	return EBTNodeResult::InProgress;
}

void UBTTask_Hide::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) {
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

	if (!Data->StartHiding) {
		Data->HideTarget = ChooseNewCover((*CurrentBot).GetActorLocation(), CurrentBot, Data);
		OwnerComp.GetAIOwner()->MoveToLocation(Data->HideTarget, 5.f, false, true, true);
		Data->StartHiding = true;
	}

	if (!OwnerComp.GetAIOwner()->IsFollowingAPath()) {
		OwnerComp.GetAIOwner()->MoveToLocation(Data->HideTarget, 5.f, false, true, true);
	}

	if (FVector::DistSquared((*CurrentBot).GetActorLocation(), Data->HideTarget) < 100 * 100) {
		Data->StartHiding = false;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
}

FVector UBTTask_Hide::ChooseNewCover(FVector CurrentLocation, ABot* CurrentBot, UBotBlackBoardData* Data) {
	AVoronoiNavData* VoronoiNavData = AVoronoiNavData::GetVoronoiNavData(GetWorld());
	FVoronoiGraph* VoronoiGraph = VoronoiNavData->GetVoronoiGraph();
	float minValue = -1;
	float currentValue;
	FVector destination;
	for (const FVoronoiFace *Face : VoronoiNavData->GetReachableFacesInRadius(CurrentLocation, Dist)) {
		if (VoronoiNavData->GetFaceByPoint(CurrentLocation) != Face) {
			if (Data->GetMapData()[Face].EnemyOverwatch == -1) {
				currentValue = (1. - Face->TacticalProperties.GetFullVisibility()) * (Face->Location - (*CurrentBot).GetActorLocation()).Size();
			}
			else {
				currentValue = (Data->GetMapData()[Face].EnemyOverwatch) * (Face->Location - (*CurrentBot).GetActorLocation()).Size();
			}
			if ((minValue == -1) || (minValue > currentValue)) {
				minValue = currentValue;
				destination = Face->Location;
			}
		}
	}
	return destination;
}