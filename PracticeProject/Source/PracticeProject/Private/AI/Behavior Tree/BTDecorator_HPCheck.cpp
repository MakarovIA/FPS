// By Polyakov Pavel

#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTDecorator_HPCheck.h"

UBTDecorator_HPCheck::UBTDecorator_HPCheck()
{
    NodeName = "Health Check";
    BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HPCheck, BotData), UBotBlackBoardData::StaticClass());
}

bool UBTDecorator_HPCheck::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    const UBotBlackBoardData* Data = static_cast<UBotBlackBoardData*>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID()));

    const ABot* Pawn = Cast<ABot>(OwnerComp.GetAIOwner()->GetPawn());
    return Data && Pawn && (Pawn->GetBotState().Health < Data->HealthThreshold);
}

void UBTDecorator_HPCheck::InitializeFromAsset(UBehaviorTree &Asset)
{
    Super::InitializeFromAsset(Asset);

    if (Asset.BlackboardAsset)
        BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTDecorator_HPCheck::GetStaticDescription() const
{
    if (IsInversed())
        return FString("Health >= BotData.HealthThreshold");

    return FString("Health < BotData.HealthThreshold");
}
