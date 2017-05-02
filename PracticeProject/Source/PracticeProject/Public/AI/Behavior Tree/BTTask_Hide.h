// By Polyakov Pavel

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Hide.generated.h"

/** Hide from enemy and suvive */
UCLASS()
class PRACTICEPROJECT_API UBTTask_Hide : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

	const float Dist = 700.;

public:
    UBTTask_Hide();

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;

	EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory);

	void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

	FVector ChooseNewCover(FVector CurrentLocation, ABot* CurrentBot, UBotBlackBoardData* Data);
};