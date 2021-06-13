#pragma once

#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HPCheck.generated.h"

/** Checks if Health is below a HealthThreshold */
UCLASS()
class PRACTICEPROJECT_API UBTDecorator_HPCheck : public UBTDecorator
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(EditAnywhere, Category = Blackboard)
    FBlackboardKeySelector BotData;

public:
    UBTDecorator_HPCheck();

    /** Calculates value of decorator's condition. Should not include calling IsInversed */
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
