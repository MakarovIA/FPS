// By Polyakov Pavel

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
    const AVoronoiNavData* VoronoiNavData = UVoronoiPropertiesManager::GetVoronoiNavData(OwnerComp.GetWorld());
    
    if (!Data || !Pawn) 
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    
    while (FVector::DistSquared(Data->Target, Pawn->GetActorLocation()) < 200 * 200)
    {
        FNavLocation Temp;
        VoronoiNavData->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), 10000, Temp);

        Data->Target = Temp.Location;
    }

    if (!OwnerComp.GetAIOwner()->IsFollowingAPath())
        OwnerComp.GetAIOwner()->MoveToLocation(Data->Target, 5.f, false, true, true);

    if (Data->GetVisibleEnemies().Num() > 0)
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
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
