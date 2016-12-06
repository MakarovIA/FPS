// By Polyakov Pavel

#include "PracticeProject.h"
#include "AIPlayerState.h"
#include "TeamPlayerController.h"

void ATeamPlayerController::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
    // TODO:
}

FGenericTeamId ATeamPlayerController::GetGenericTeamId() const
{
    if (AAIPlayerState *PS = Cast<AAIPlayerState>(PlayerState))
        return PS->GetGenericTeamId();

    return FGenericTeamId::NoTeam;
}
