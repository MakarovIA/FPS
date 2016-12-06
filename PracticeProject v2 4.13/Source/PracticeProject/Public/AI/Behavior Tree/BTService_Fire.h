// By Polyakov Pavel

#pragma once

#include "BehaviorTree/BTService.h"
#include "BTService_Fire.generated.h"

/** Fire enemy down */
UCLASS(HideCategories=(Service))
class PRACTICEPROJECT_API UBTService_Fire : public UBTService
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

public:
    UBTService_Fire();

    /** Tick */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;
	
    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
