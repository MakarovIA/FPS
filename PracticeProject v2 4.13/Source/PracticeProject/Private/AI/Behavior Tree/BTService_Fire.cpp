// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTService_Fire.h"

UBTService_Fire::UBTService_Fire()
{
    NodeName = "Fire";

    bNotifyBecomeRelevant = false;
    bNotifyCeaseRelevant = false;

    bTickIntervals = false;

    BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_Fire, BotData), UBotBlackBoardData::StaticClass());
}

void UBTService_Fire::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
}

void UBTService_Fire::InitializeFromAsset(UBehaviorTree &Asset)
{
    Super::InitializeFromAsset(Asset);

    if (Asset.BlackboardAsset)
        BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTService_Fire::GetStaticDescription() const
{
    return FString("Kill them all");
}
