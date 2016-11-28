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

public:
    UBTTask_Hide();

    /** Initialize any asset related data */
    virtual void InitializeFromAsset(UBehaviorTree &Asset) override;

    /** Returns static description */
    virtual FString GetStaticDescription() const override;
};
