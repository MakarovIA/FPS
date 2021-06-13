#include "PracticeProject.h"
#include "BotBlackBoardData.h"
#include "BTDecorator_IsEnemySeen.h"

UBTDecorator_IsEnemySeen::UBTDecorator_IsEnemySeen()
{
    NodeName = "Is Enemy Seen";
    BotData.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsEnemySeen, BotData), UBotBlackBoardData::StaticClass());
}

bool UBTDecorator_IsEnemySeen::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    const UBotBlackBoardData* Data = static_cast<UBotBlackBoardData*>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(BotData.GetSelectedKeyID()));
	// .....
	if (Data) {
		ABot* CurrentBot = Cast<ABot>(OwnerComp.GetAIOwner()->GetPawn());
		if (((*CurrentBot).GetBotState().Health > Data->HealthThreshold) && (Data->CurrentFollowTarget.Num() > 0)) {
			return true;
		}
	}
	// ...........
    return Data && (Data->GetVisibleEnemies().Num() > 0);
}

void UBTDecorator_IsEnemySeen::InitializeFromAsset(UBehaviorTree &Asset)
{
    Super::InitializeFromAsset(Asset);

    if (Asset.BlackboardAsset)
        BotData.ResolveSelectedKey(*Asset.BlackboardAsset);
}

FString UBTDecorator_IsEnemySeen::GetStaticDescription() const
{
    if (IsInversed())
        return FString("BotData.VisibleEnemies.Num() == 0");

    return FString("BotData.VisibleEnemies.Num() > 0");
}
