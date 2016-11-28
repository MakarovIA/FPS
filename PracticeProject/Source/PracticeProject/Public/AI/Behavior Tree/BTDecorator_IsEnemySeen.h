// By Polyakov Pavel

#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsEnemySeen.generated.h"

/** Checks if enemies are present */
UCLASS()
class PRACTICEPROJECT_API UBTDecorator_IsEnemySeen : public UBTDecorator
{
	GENERATED_BODY()

protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

public:
    UBTDecorator_IsEnemySeen();

    /** Calculates value of decorator's condition. Should not include calling IsInversed */
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;
	
    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
