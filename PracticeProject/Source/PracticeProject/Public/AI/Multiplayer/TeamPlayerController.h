#pragma once

#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "TeamPlayerController.generated.h"

/** Extends player controller to support teams */
UCLASS()
class PRACTICEPROJECT_API ATeamPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    /** Assigns Team Agent to given TeamID */
    virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;

    /** Retrieve team identifier in form of FGenericTeamId */
    virtual FGenericTeamId GetGenericTeamId() const override;
};
