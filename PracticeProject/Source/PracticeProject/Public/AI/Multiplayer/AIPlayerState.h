#pragma once

#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"
#include "AIPlayerState.generated.h"

class APracticeProjectGameMode;

/** Player (or AI) state */
UCLASS()
class PRACTICEPROJECT_API AAIPlayerState : public APlayerState
{
    GENERATED_BODY()

    /** Indicates team id */
    UPROPERTY(Replicated)
    FGenericTeamId TeamId;

    friend APracticeProjectGameMode;
	
public:
    AAIPlayerState() : TeamId(FGenericTeamId::NoTeam) {}

    /** Setting up replication */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Get team id */
    FORCEINLINE FGenericTeamId GetGenericTeamId() const { return TeamId; }
};

/** Team solver that supports all vs all mode */
ETeamAttitude::Type AITeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B);
