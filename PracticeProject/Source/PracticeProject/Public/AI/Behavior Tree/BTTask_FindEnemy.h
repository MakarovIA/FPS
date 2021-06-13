#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindEnemy.generated.h"

/** Find enemies based on influence map data */
UCLASS()
class PRACTICEPROJECT_API UBTTask_FindEnemy : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

public:
    UBTTask_FindEnemy();
	
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
