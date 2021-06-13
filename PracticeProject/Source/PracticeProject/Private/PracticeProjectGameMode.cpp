#include "PracticeProject.h"

#include "AIPlayerState.h"
#include "TeamPlayerController.h"
#include "HUDD.h"
#include "PracticeProjectGameMode.h"

APracticeProjectGameMode::APracticeProjectGameMode()
{
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Characters/Soldier/Blueprints/SoldierBP"));
    if (PlayerPawnBPClass.Class != nullptr)
        DefaultPawnClass = PlayerPawnBPClass.Class;

    PlayerControllerClass = ATeamPlayerController::StaticClass();
    PlayerStateClass = AAIPlayerState::StaticClass();
	HUDClass = AHUDD::StaticClass();
}

void APracticeProjectGameMode::InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    FGenericTeamId::SetAttitudeSolver(AITeamAttitudeSolver);
}
