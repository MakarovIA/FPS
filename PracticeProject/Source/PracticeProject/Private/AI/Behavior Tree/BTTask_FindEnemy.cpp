#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTTask_FindEnemy.h"

UBTTask_FindEnemy::UBTTask_FindEnemy()
{
	NodeName = "Find Enemy";
	bNotifyTick = true;

	BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindEnemy, BotData), UBotBlackBoardData::StaticClass());
}

EBTNodeResult::Type UBTTask_FindEnemy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

void UBTTask_FindEnemy::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	UBotBlackBoardData* Data = static_cast<UBotBlackBoardData*>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID()));

	const ABot* Pawn = Cast<ABot>(OwnerComp.GetAIOwner()->GetPawn());
	const AVoronoiNavData* VoronoiNavData = AVoronoiNavData::GetVoronoiNavData(OwnerComp.GetWorld());

	if (!Data || !Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Data->StartHiding = false;

	if (FVector::DistSquared(Data->Target, Pawn->GetActorLocation()) < 200 * 200)
	{
		FNavLocation Temp;
		VoronoiNavData->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), 10000, Temp);

		Data->Target = Temp.Location;
		OwnerComp.GetAIOwner()->MoveToLocation(Data->Target, 5.f, false, true, true);
	}

	if (!OwnerComp.GetAIOwner()->IsFollowingAPath())
		OwnerComp.GetAIOwner()->MoveToLocation(Data->Target, 5.f, false, true, true);

	if (Data->BotLastHealth == -1) {
		Data->BotLastHealth = Pawn->GetBotState().Health;
		Data->BotLastPosition = Pawn->GetActorLocation();
	}
	else {
		if (Data->BotLastHealth > Pawn->GetBotState().Health) {
			OwnerComp.GetAIOwner()->SetFocalPoint(Data->BotLastPosition);
			Data->BotLastHealth = Pawn->GetBotState().Health;
			Data->BotLastPosition = Pawn->GetActorLocation();
		}
		else {
			Data->BotLastPosition = Pawn->GetActorLocation();
		}
	}

	if (Data->GetVisibleEnemies().Num() > 0) {
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		Data->Target = Pawn->GetActorLocation();
		OwnerComp.GetAIOwner()->MoveToLocation(OwnerComp.GetAIOwner()->GetPawn()->GetActorLocation(), 5.f, false, true, true);
	}
}

void UBTTask_FindEnemy::InitializeFromAsset(UBehaviorTree &Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
		BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTTask_FindEnemy::GetStaticDescription() const
{
	return FString("Go to influence maximum");
}
