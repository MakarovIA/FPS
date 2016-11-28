// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTTask_Follow.h"

UBTTask_Follow::UBTTask_Follow()
{
    NodeName = "Follow Enemy";
    BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_Follow, BotData), UBotBlackBoardData::StaticClass());
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
