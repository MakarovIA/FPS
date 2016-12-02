// By Polyakov Pavel

#include "PracticeProject.h"
#include "UnrealNetwork.h"
#include "AIPlayerState.h"

void AAIPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAIPlayerState, TeamId);
}

ETeamAttitude::Type AITeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
{
    if (A == FGenericTeamId::NoTeam || B == FGenericTeamId::NoTeam)
        return ETeamAttitude::Hostile;

    return A != B ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
}
