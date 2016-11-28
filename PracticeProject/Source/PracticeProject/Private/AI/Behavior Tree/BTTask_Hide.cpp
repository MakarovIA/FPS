// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTTask_Hide.h"

UBTTask_Hide::UBTTask_Hide()
{
    NodeName = "Hide";
    BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_Hide, BotData), UBotBlackBoardData::StaticClass());
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
