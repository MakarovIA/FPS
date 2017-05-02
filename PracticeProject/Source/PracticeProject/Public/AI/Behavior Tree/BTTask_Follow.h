// By Polyakov Pavel

#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Follow.generated.h"

/** Follow enemy */
UCLASS()
class PRACTICEPROJECT_API UBTTask_Follow : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

	const float MAXRadius = 300.;
	const float Dist = 350.;
	const float DistForCover = 700.;

public:
    UBTTask_Follow();

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds);

	FVector ChooseNewCover(FVector CurrentLocation, ABot* CurrentBot, UBotBlackBoardData* Data);
};
