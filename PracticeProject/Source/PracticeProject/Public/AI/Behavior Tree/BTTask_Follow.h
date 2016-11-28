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

public:
    UBTTask_Follow();

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
